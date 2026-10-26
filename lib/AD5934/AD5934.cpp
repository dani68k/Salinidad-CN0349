#include "AD5934.h"


namespace {
const AD5934::RegisterGroup groups[] = {
    {0x80, 2, "Control", true}, {0x82, 3, "Start Frequency", true},
    {0x85, 3, "Frequency Increment", true}, {0x88, 2, "Number of Increments", true},
    {0x8A, 2, "Settling Cycles", true}, {0x8F, 1, "Status", false},
    {0x94, 2, "Real Data (raw)", false}, {0x96, 2, "Imaginary Data (raw)", false},
};
}

AD5934::AD5934(TwoWire& wire, uint8_t address) : wire_(wire), address_(address) {}

const AD5934::RegisterGroup* AD5934::groupAt(size_t index) {
  return index < sizeof(groups) / sizeof(groups[0]) ? &groups[index] : nullptr;
}

const AD5934::RegisterGroup* AD5934::groupFor(uint8_t reg) {
  for (const RegisterGroup& group : groups) {
    if (reg >= group.first && reg < group.first + group.size) return &group;
  }
  return nullptr;
}

void AD5934::clearError() { error_ = Error{}; }

bool AD5934::fail(ErrorCode code, uint8_t reg, Phase phase, uint8_t wireCode) {
  error_.code = code;
  error_.reg = reg;
  error_.phase = phase;
  error_.wireCode = wireCode;
  return false;
}

bool AD5934::transmit(const uint8_t* bytes, size_t length, uint8_t reg, Phase phase) {
  wire_.beginTransmission(address_);
  const size_t queued = length ? wire_.write(bytes, length) : 0;
  // Siempre terminar para liberar el bloqueo interno de Wire, incluso si falta buffer.
  const uint8_t code = wire_.endTransmission(true);
  if (queued != length) return fail(ErrorCode::TxBuffer, reg, phase, code);
  if (code == 0) return true;
  const ErrorCode error = code == 1 ? ErrorCode::TxBuffer :
                          code == 2 ? ErrorCode::AddressNack :
                          code == 3 ? ErrorCode::DataNack :
                          code == 5 ? ErrorCode::Timeout : ErrorCode::Bus;
  return fail(error, reg, phase, code);
}

bool AD5934::begin() {
  clearError();
  if (address_ < 0x08 || address_ > 0x77)
    return fail(ErrorCode::InvalidAddress, 0, Phase::Validation);
  return transmit(nullptr, 0, 0, Phase::Probe);
}

bool AD5934::validateSpan(uint8_t reg, uint8_t width, bool writing) {
  clearError();
  if (address_ < 0x08 || address_ > 0x77)
    return fail(ErrorCode::InvalidAddress, reg, Phase::Validation);
  const RegisterGroup* group = groupFor(reg);
  if (!group) return fail(ErrorCode::InvalidRegister, reg, Phase::Validation);
  if (width < 1 || width > 3 || static_cast<unsigned int>(reg) + width > group->first + group->size)
    return fail(ErrorCode::InvalidSpan, reg, Phase::Validation);
  if (writing && !group->writable)
    return fail(ErrorCode::ReadOnly, reg, Phase::Validation);
  // Politica conservadora 1B: ni siquiera la API de bajo nivel permite activar DDS.
  if (writing && group->first == CONTROL_HIGH)
    return fail(ErrorCode::ControlProtected, reg, Phase::Validation);
  return true;
}

bool AD5934::validateRead(uint8_t reg, uint8_t width) {
  return validateSpan(reg, width, false);
}

uint8_t AD5934::meaningfulMask(uint8_t reg) {
  if (reg == NUMBER_INCREMENTS_MSB) return 0x01;
  if (reg == SETTLING_CYCLES_MSB) return 0x07;
  return 0xFF;
}

bool AD5934::validateWrite(uint8_t reg, uint8_t width, uint32_t value) {
  if (!validateSpan(reg, width, true)) return false;
  if (value >> (8 * width)) return fail(ErrorCode::InvalidValue, reg, Phase::Validation);
  // Validar todos los bytes antes de enviar el primero: sin escrituras parciales por argumentos.
  for (uint8_t index = 0; index < width; ++index) {
    const uint8_t target = reg + index;
    const uint8_t byte = static_cast<uint8_t>(value >> (8 * (width - index - 1)));
    if ((byte & static_cast<uint8_t>(~meaningfulMask(target))) != 0)
      return fail(ErrorCode::InvalidValue, target, Phase::Validation);
    // Tabla 13: D10:D9 = 10 es reservado; 00, 01 y 11 estan definidos.
    if (target == SETTLING_CYCLES_MSB && (byte & 0x06) == 0x04)
      return fail(ErrorCode::InvalidValue, target, Phase::Validation);
  }
  return true;
}

bool AD5934::readBytes(uint8_t reg, uint8_t width, uint32_t& value) {
  if (!validateRead(reg, width)) return false;
  uint32_t result = 0;
  for (uint8_t index = 0; index < width; ++index) {
    const uint8_t target = reg + index;
    const uint8_t pointer[] = {ADDRESS_POINTER, target};
    if (!transmit(pointer, sizeof(pointer), target, Phase::Pointer)) return false;
    // Receive Byte con STOP previo y posterior (figuras 27 y 29), sin autoincremento.
    const size_t received = wire_.requestFrom(address_, static_cast<size_t>(1), true);
    if (received != 1 || wire_.available() != 1)
      return fail(ErrorCode::ShortRead, target, Phase::Receive);
    const int byte = wire_.read();
    if (byte < 0) return fail(ErrorCode::ShortRead, target, Phase::Receive);
    result = (result << 8) | static_cast<uint8_t>(byte);
    ++error_.completedBytes;
  }
  value = result;  // El argumento del llamante no cambia si falla algun byte.
  return true;
}

bool AD5934::writeBytes(uint8_t reg, uint8_t width, uint32_t value) {
  if (!validateWrite(reg, width, value)) return false;
  for (uint8_t index = 0; index < width; ++index) {
    const uint8_t target = reg + index;
    const uint8_t bytes[] = {target, static_cast<uint8_t>(value >> (8 * (width - index - 1)))};
    // Write Byte (figura 26). No utilizar Address Pointer como prefijo de escritura.
    if (!transmit(bytes, sizeof(bytes), target, Phase::Write)) return false;
    ++error_.completedBytes;
  }
  return true;
}

bool AD5934::readRegister8(uint8_t reg, uint8_t& value) {
  uint32_t result;
  if (!readBytes(reg, 1, result)) return false;
  value = static_cast<uint8_t>(result);
  return true;
}
bool AD5934::readRegister16(uint8_t reg, uint16_t& value) {
  uint32_t result;
  if (!readBytes(reg, 2, result)) return false;
  value = static_cast<uint16_t>(result);
  return true;
}
bool AD5934::readRegister24(uint8_t reg, uint32_t& value) { return readBytes(reg, 3, value); }
bool AD5934::writeRegister8(uint8_t reg, uint8_t value) { return writeBytes(reg, 1, value); }
bool AD5934::writeRegister16(uint8_t reg, uint16_t value) { return writeBytes(reg, 2, value); }
bool AD5934::writeRegister24(uint8_t reg, uint32_t value) { return writeBytes(reg, 3, value); }
bool AD5934::readStatus(uint8_t& value) { return readRegister8(STATUS, value); }

bool AD5934::setStartFrequencyCode(uint32_t code) {
  if (code > 0xFFFFFF) return fail(ErrorCode::InvalidValue, START_FREQUENCY_MSB, Phase::Validation);
  return writeRegister24(START_FREQUENCY_MSB, code);
}

bool AD5934::setFrequencyIncrementCode(uint32_t code) {
  if (code > 0xFFFFFF) return fail(ErrorCode::InvalidValue, FREQUENCY_INCREMENT_MSB, Phase::Validation);
  return writeRegister24(FREQUENCY_INCREMENT_MSB, code);
}

bool AD5934::setNumberOfIncrements(uint16_t count) {
  if (count > 511) return fail(ErrorCode::InvalidValue, NUMBER_INCREMENTS_MSB, Phase::Validation);
  return writeRegister16(NUMBER_INCREMENTS_MSB, count);
}

bool AD5934::setSettlingCycles(uint16_t cycles) {
  if (cycles > 511) return fail(ErrorCode::InvalidValue, SETTLING_CYCLES_MSB, Phase::Validation);
  return writeRegister16(SETTLING_CYCLES_MSB, cycles);
}

bool AD5934::configureSweep(uint32_t startFrequencyCode, uint16_t settlingWord) {
  // Validate el perfil completo antes de modificar cualquier byte.
  if (startFrequencyCode > 0xFFFFFF || !AD5934Math::validSettlingWord(settlingWord)) {
    return fail(ErrorCode::InvalidValue, START_FREQUENCY_MSB, Phase::Validation);
  }
  if (!setStartFrequencyCode(startFrequencyCode)) return false;
  if (!setFrequencyIncrementCode(0)) return false;
  if (!setNumberOfIncrements(0)) return false;
  return writeRegister16(SETTLING_CYCLES_MSB, settlingWord);
}

bool AD5934::configureRange1PgaX1() {
  uint16_t current;
  if (!readRegister16(CONTROL_HIGH, current)) return false;
  if (!AD5934Math::validInactiveControl(current))
    return fail(ErrorCode::InvalidValue, CONTROL_HIGH, Phase::Validation);
  // Power-down + Range 1 (D10:D9=00) + PGA x1 (D8=1); D4 reset stays zero.
  // Control Low remains untouched at 0x08.
  const uint8_t high = AD5934Math::controlHighRange1PgaX1();
  const uint8_t command[] = {CONTROL_HIGH, high};
  if (!transmit(command, sizeof(command), CONTROL_HIGH, Phase::Write)) return false;
  uint16_t verified;
  if (!readRegister16(CONTROL_HIGH, verified)) return false;
  const uint16_t expected = static_cast<uint16_t>((static_cast<uint16_t>(high) << 8) | 0x08);
  if (verified != expected) return fail(ErrorCode::InvalidValue, CONTROL_HIGH, Phase::Receive);
  return true;
}

bool AD5934::wakeToStandby(uint16_t& previousControl, uint16_t& verifiedControl) {
  if (!readRegister16(CONTROL_HIGH, previousControl)) return false;
  if (!AD5934Math::validInactiveControl(previousControl))
    return fail(ErrorCode::InvalidValue, CONTROL_HIGH, Phase::Validation);
  const uint8_t mode = static_cast<uint8_t>((previousControl >> 12) & 0x0F);
  if (mode == 0x0B) {
    verifiedControl = previousControl;
    return true;
  }
  // Power-down -> standby preserves range/PGA and never asserts reset/DDS.
  const uint8_t high = AD5934Math::controlHighWithOperation(
      static_cast<uint8_t>(previousControl >> 8), 0x0B);
  const uint8_t bytes[] = {CONTROL_HIGH, high};
  if (!transmit(bytes, sizeof(bytes), CONTROL_HIGH, Phase::Write)) return false;
  if (!readRegister16(CONTROL_HIGH, verifiedControl)) return false;
  const uint16_t expected = static_cast<uint16_t>((static_cast<uint16_t>(high) << 8) | 0x08);
  if (verifiedControl != expected)
    return fail(ErrorCode::InvalidValue, CONTROL_HIGH, Phase::Receive);
  return true;
}

bool AD5934::sendCommand(ControlCommand command) {
  const uint8_t operation = static_cast<uint8_t>(command);
  if (!AD5934Math::validControlOperation(operation))
    return fail(ErrorCode::InvalidValue, CONTROL_HIGH, Phase::Validation);
  uint16_t current;
  if (!readRegister16(CONTROL_HIGH, current)) return false;
  if (operation == static_cast<uint8_t>(ControlCommand::IncrementFrequency)) {
    uint8_t status;
    if (!readStatus(status)) return false;
    if (!(status & STATUS_DATA_VALID))
      return fail(ErrorCode::InvalidValue, STATUS, Phase::Validation);
  }
  if (!AD5934Math::validControlTransition(current, operation))
    return fail(ErrorCode::InvalidValue, CONTROL_HIGH, Phase::Validation);
  // Preservar D11:D8 (rango/PGA), sustituir solo D15:D12. No tocar reset D4.
  const uint8_t high = AD5934Math::controlHighWithOperation(static_cast<uint8_t>(current >> 8), operation);
  const uint8_t bytes[] = {CONTROL_HIGH, high};
  if (!transmit(bytes, sizeof(bytes), CONTROL_HIGH, Phase::Write)) return false;
  uint16_t verified;
  if (!readRegister16(CONTROL_HIGH, verified)) return false;
  const uint16_t expected = static_cast<uint16_t>((static_cast<uint16_t>(high) << 8) | 0x08);
  if (verified != expected) return fail(ErrorCode::InvalidValue, CONTROL_HIGH, Phase::Receive);
  return true;
}

bool AD5934::readReal(int16_t& value) {
  uint16_t raw;
  if (!readRegister16(REAL_MSB, raw)) return false;
  value = AD5934Math::decodeSigned16(raw);
  return true;
}

bool AD5934::readImaginary(int16_t& value) {
  uint16_t raw;
  if (!readRegister16(IMAGINARY_MSB, raw)) return false;
  value = AD5934Math::decodeSigned16(raw);
  return true;
}

bool AD5934::frequencyToCode(uint32_t frequencyHz, uint32_t mclkHz, uint32_t& code) {
  return AD5934Math::frequencyToCode(frequencyHz, mclkHz, code);
}

double AD5934::magnitude(int16_t real, int16_t imaginary) {
  return AD5934Math::magnitude(real, imaginary);
}

bool AD5934::enterStandby(uint16_t& previousControl, uint16_t& verifiedControl) {
  if (!readRegister16(CONTROL_HIGH, previousControl)) return false;
  const uint8_t mode = static_cast<uint8_t>(previousControl >> 12);
  if (mode == 0x0A && (previousControl & 0x00FF) == 0x0008) {
    // Ya está en power-down con los bits reservados correctos.
    verifiedControl = previousControl;
    return true;
  }
  if (mode == 0x0B && (previousControl & 0x00FF) == 0x0008) {
    verifiedControl = previousControl;
    return true;
  }
  if (mode > 0x04 && mode < 0x08 || mode == 0x0E || mode == 0x0F)
    return fail(ErrorCode::InvalidValue, CONTROL_HIGH, Phase::Validation);

  if (mode == 0x0A) {
    // Corregir solo Control Low estando ya en power-down: no se despierta el DDS.
    const uint8_t lowBytes[] = {CONTROL_LOW, 0x08};
    if (!transmit(lowBytes, sizeof(lowBytes), CONTROL_LOW, Phase::Write)) return false;
    ++error_.completedBytes;
    if (!readRegister16(CONTROL_HIGH, verifiedControl)) return false;
    const uint16_t expected = static_cast<uint16_t>((previousControl & 0xFF00) | 0x08);
    if (verifiedControl != expected)
      return fail(ErrorCode::InvalidValue, CONTROL_LOW, Phase::Receive);
    return true;
  }

  // Rev. E: solo el byte alto es necesario para comandos de modo. Conservar
  // D11:D8 (rango de salida/PGA); D15:D12 se cambia exclusivamente a standby.
  const uint8_t standbyHigh = static_cast<uint8_t>(0xB0 | ((previousControl >> 8) & 0x0F));
  const uint8_t highBytes[] = {CONTROL_HIGH, standbyHigh};
  if (!transmit(highBytes, sizeof(highBytes), CONTROL_HIGH, Phase::Write)) return false;
  ++error_.completedBytes;

  // La tabla 9 prescribe D3=1 y los demás bits bajos reservados=0.
  const uint8_t lowBytes[] = {CONTROL_LOW, 0x08};
  if (!transmit(lowBytes, sizeof(lowBytes), CONTROL_LOW, Phase::Write)) return false;
  ++error_.completedBytes;

  if (!readRegister16(CONTROL_HIGH, verifiedControl)) return false;
  if (verifiedControl != static_cast<uint16_t>((static_cast<uint16_t>(standbyHigh) << 8) | 0x08))
    return fail(ErrorCode::InvalidValue, CONTROL_HIGH, Phase::Receive);
  return true;
}

const char* AD5934::phaseText(Phase phase) {
  switch (phase) {
    case Phase::Validation: return "validacion";
    case Phase::Probe: return "ping";
    case Phase::Pointer: return "address pointer";
    case Phase::Receive: return "recepcion";
    case Phase::Write: return "escritura";
  }
  return "desconocida";
}

const char* AD5934::errorText(ErrorCode code) {
  switch (code) {
    case ErrorCode::None: return "sin error";
    case ErrorCode::InvalidAddress: return "direccion I2C no valida";
    case ErrorCode::InvalidRegister: return "registro reservado o no documentado";
    case ErrorCode::InvalidSpan: return "acceso fuera del grupo de registros";
    case ErrorCode::ReadOnly: return "registro de solo lectura";
    case ErrorCode::ControlProtected: return "control protegido; use las operaciones acotadas";
    case ErrorCode::InvalidValue: return "valor fuera de rango o bits no permitidos";
    case ErrorCode::TxBuffer: return "buffer de transmision insuficiente";
    case ErrorCode::AddressNack: return "NACK de direccion";
    case ErrorCode::DataNack: return "NACK de datos";
    case ErrorCode::Timeout: return "timeout de transmision";
    case ErrorCode::Bus: return "error de bus";
    case ErrorCode::ShortRead: return "lectura incompleta (Wire no expone la causa de recepcion)";
  }
  return "error desconocido";
}
