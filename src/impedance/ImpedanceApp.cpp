#include "ImpedanceApp.h"

#include <string.h>
#include <math.h>
#include "../terminal/Terminal.h"

namespace {
constexpr uint32_t ACQUISITION_TIMEOUT_MS = 500;
constexpr uint32_t APPLICATION_PHASE_TIMEOUT_MS = 1500;
constexpr uint32_t STATUS_POLL_INTERVAL_MS = 5;
constexpr uint8_t CONFIG_FIELD_COUNT = 11;
}

ImpedanceApp::ImpedanceApp(AD5934& driver, MuxApp& mux, Stream& output)
    : driver_(driver), mux_(mux), output_(output) {}

void ImpedanceApp::begin(Terminal& terminal) {
  dumpActive_ = false;
  measurementState_ = MeasurementState::Idle;
  terminal.attachModule(this, &ImpedanceApp::dispatch, &ImpedanceApp::busy,
                        "  ad5934 measure <help|config|start|status|result|stop|set> - Adquisicion DFT");
}

void ImpedanceApp::reportConnection(bool ready) {
  publicStatus_.ad5934Communication = ready;
  if (ready) {
    output_.print("ACK en direccion configurada para AD5934: ");
    printHex(driver_.address(), 2);
    output_.println(" (no identifica el chip).");
  } else error();
}

bool ImpedanceApp::dispatch(void* context, char* line, bool busReady, bool busBusy) {
  return static_cast<ImpedanceApp*>(context)->process(line, busReady, busBusy);
}

bool ImpedanceApp::busy(void* context) {
  const ImpedanceApp* app = static_cast<ImpedanceApp*>(context);
  return app->dumpActive_ || (app->measurementState_ != MeasurementState::Idle &&
      app->measurementState_ != MeasurementState::Complete &&
      app->measurementState_ != MeasurementState::Error &&
      app->measurementState_ != MeasurementState::Cancelled);
}

void ImpedanceApp::printHex(uint32_t value, uint8_t digits) {
  static const char hex[] = "0123456789ABCDEF";
  output_.print("0x");
  for (uint8_t digit = digits; digit > 0; --digit)
    output_.print(hex[(value >> (4 * (digit - 1))) & 0x0F]);
}

void ImpedanceApp::printValue(uint8_t reg, uint8_t width, uint32_t value) {
  output_.print("AD5934 [");
  printHex(reg, 2);
  output_.print("] = ");
  printHex(value, width * 2);
  output_.println();
}

void ImpedanceApp::error() {
  const AD5934::Error& e = driver_.lastError();
  using namespace ImpedanceDiagnostics;
  const ErrorCode code = e.code == AD5934::ErrorCode::AddressNack || e.code == AD5934::ErrorCode::DataNack ? ErrorCode::I2cNack :
      e.code == AD5934::ErrorCode::Timeout ? ErrorCode::I2cTimeout :
      e.code == AD5934::ErrorCode::None ? ErrorCode::InvalidState : ErrorCode::I2cError;
  recordError(code, Subsystem::Ad5934, AD5934::errorText(e.code), code != ErrorCode::InvalidState);
  output_.print("Error AD5934: ");
  output_.print(AD5934::errorText(e.code));
  output_.print("; fase=");
  output_.print(AD5934::phaseText(e.phase));
  output_.print("; registro=");
  printHex(e.reg, 2);
  output_.print("; bytes completados=");
  output_.print(e.completedBytes);
  if (e.phase == AD5934::Phase::Pointer || e.phase == AD5934::Phase::Probe ||
      e.phase == AD5934::Phase::Write) {
    output_.print("; Wire code=");
    output_.print(e.wireCode);
  }
  output_.println();
}

void ImpedanceApp::help() {
  output_.println("Comandos AD5934 (registros y valores en hexadecimal, prefijo 0x opcional):");
  output_.println("  ad5934 help                  - Mostrar ayuda");
  output_.println("  ad5934 ping                  - Sondear la direccion configurada");
  output_.println("  ad5934 standby               - Detener modo activo y verificar standby");
  output_.println("  ad5934 read <reg>            - Leer 8 bits");
  output_.println("  ad5934 read16 <reg>          - Leer 16 bits, MSB primero");
  output_.println("  ad5934 read24 <reg>          - Leer 24 bits, MSB primero");
  output_.println("  ad5934 write <reg> <value>   - Escribir/verificar 8 bits permitidos");
  output_.println("  ad5934 write16 <reg> <value> - Escribir/verificar 16 bits permitidos");
  output_.println("  ad5934 write24 <reg> <value> - Escribir/verificar 24 bits permitidos");
  output_.println("  ad5934 status                - Estado integrado del subsistema AD5934/ADG715");
  output_.println("  ad5934 diag                  - Diagnostico integrado no destructivo");
  output_.println("  ad5934 errors [clear]        - Historial tipado y contadores de errores");
  output_.println("  ad5934 recover               - Recuperacion controlada sin cambiar preset");
  output_.println("  ad5934 dump                  - Leer mapa documentado por grupos");
  output_.println("  ad5934 measure help          - Ayuda de adquisicion DFT");
  output_.println("Sin escritura RAW de control; medida/standby usan operaciones acotadas.");
  output_.println("Escrituras genericas de configuracion: 0x82..0x8B en modo inactivo seguro.");
}

void ImpedanceApp::measureHelp() {
  output_.println("Comandos de medida DFT de un unico punto:");
  output_.println("  ad5934 measure help");
  output_.println("  ad5934 measure config");
  output_.println("  ad5934 measure start");
  output_.println("  ad5934 measure status");
  output_.println("  ad5934 measure result");
  output_.println("  ad5934 measure stop");
  output_.println("  ad5934 measure set freq <Hz>      (1..MCLK/16)");
  output_.println("  ad5934 measure set settling <cycles> (0..511)");
  output_.println("  ad5934 measure set preset <NAME>  (4 presets CAL_*)");
  output_.println("  ad5934 measure sweep <help|config|set|start|status|result|stop>");
  output_.println("No calcula impedancia ni calibra el sistema.");
}

void ImpedanceApp::sweepHelp() {
  output_.println("Barrido AD5934 (maximo 64 puntos; increments = puntos - 1):");
  output_.println("  ad5934 measure sweep help");
  output_.println("  ad5934 measure sweep config");
  output_.println("  ad5934 measure sweep set start <Hz>");
  output_.println("  ad5934 measure sweep set step <Hz>");
  output_.println("  ad5934 measure sweep set increments <n> (0..63)");
  output_.println("  ad5934 measure sweep start|status|result|stop");
  output_.println("El preset es el configurado con ad5934 measure set preset.");
  output_.println("Magnitud/fase son del codigo DFT crudo, no impedancia calibrada.");
}

bool ImpedanceApp::validateSweep(uint32_t& finalHz, uint32_t& startCode,
                                 uint32_t& incrementCode) const {
  uint32_t end = 0;
  if (!AD5934Math::validateSweep(sweepConfig_.startHz, sweepConfig_.incrementHz,
          sweepConfig_.increments, MAX_SWEEP_POINTS,
          profile_.mclkHz / 16 < AD5934::MAX_OUTPUT_FREQUENCY_HZ ? profile_.mclkHz / 16 : AD5934::MAX_OUTPUT_FREQUENCY_HZ,
          end)) return false;
  uint32_t start = 0, step = 0;
  if (!AD5934::frequencyToCode(sweepConfig_.startHz, profile_.mclkHz, start)) return false;
  if (sweepConfig_.increments == 0 && sweepConfig_.incrementHz == 0) step = 0;
  else if (!AD5934::frequencyToCode(sweepConfig_.incrementHz, profile_.mclkHz, step)) return false;
  if (!AD5934Math::validSweepDdsCodes(start, step, sweepConfig_.increments)) return false;
  finalHz = end;
  startCode = start;
  incrementCode = step;
  return true;
}

void ImpedanceApp::sweepConfig() {
  uint32_t end = 0, startCode = 0, incrementCode = 0;
  const bool valid = validateSweep(end, startCode, incrementCode);
  MuxApp::CalibrationInfo cal{};
  output_.println("Configuracion de barrido AD5934:");
  if (mux_.resolveCalibrationPreset(profile_.calibrationPreset, cal)) printCalibration(cal);
  output_.print("MCLK         : "); output_.print(profile_.mclkHz); output_.println(" Hz externo");
  output_.print("Start        : "); output_.print(sweepConfig_.startHz); output_.println(" Hz");
  output_.print("Step         : "); output_.print(sweepConfig_.incrementHz); output_.println(" Hz");
  output_.print("Increments   : "); output_.println(sweepConfig_.increments);
  output_.print("Points       : "); output_.println(sweepConfig_.increments + 1);
  output_.print("End          : "); output_.print(valid ? end : 0); output_.println(valid ? " Hz" : " Hz (config invalida)");
  output_.print("Start DDS    : "); if (valid) printHex(startCode, 6); else output_.print("invalido"); output_.println();
  output_.print("Step DDS     : "); if (valid) printHex(incrementCode, 6); else output_.print("invalido"); output_.println();
  output_.print("Settling     : "); output_.print(profile_.settlingCycles); output_.println(" cycles x1");
}

void ImpedanceApp::sweepStatus() {
  const char* state = (sweepMode_ && busy(this)) ? measurementStateName() :
      sweepComplete_ ? "COMPLETE" : sweepError_ ? (strstr(sweepError_, "cancel") ? "CANCELLED" : "ERROR") : "INCOMPLETE";
  output_.print("Sweep state  : "); output_.println(state);
  output_.print("Points       : "); output_.print(sweepPointCount_); output_.print("/"); output_.println(sweepTargetPoints_);
  if (sweepPointCount_ < sweepTargetPoints_ && sweepTargetPoints_ > 0) {
    const double freq = static_cast<double>(activeSweepConfig_.startHz) +
        static_cast<double>(activeSweepConfig_.incrementHz) * sweepPointCount_;
    output_.print("Current Hz   : "); output_.println(freq, 3);
  }
  output_.print("Elapsed      : "); output_.print((sweepMode_ && busy(this)) ? millis() - measurementStartedMs_ : sweepElapsedMs_); output_.println(" ms");
  output_.print("Last error   : "); output_.println(sweepError_ ? sweepError_ : "ninguno");
}

void ImpedanceApp::printSweepPoint(const SweepPoint& p) {
  output_.print(p.index); output_.print("  "); output_.print(p.frequencyHz, 3); output_.print("  ");
  output_.print(p.real); output_.print("  "); output_.print(p.imaginary); output_.print("  ");
  output_.print(p.magnitude, 3); output_.print("  "); output_.print(p.phaseDegrees, 3);
  output_.print("  "); output_.println(p.valid ? "VALID" : "INVALID");
}

void ImpedanceApp::sweepResult() {
  if (!hasSweep_) { output_.println("No hay resultados de barrido disponibles."); return; }
  MuxApp::CalibrationInfo cal{};
  output_.println("ATLANTES - AD5934 Frequency Sweep (DFT cruda)");
  if (mux_.resolveCalibrationPreset(activeSweepCalibration_.id, cal)) printCalibration(cal);
  output_.print("MCLK: "); output_.print(activeSweepMclkHz_); output_.println(" Hz externo");
  output_.print("Start/step/increments: "); output_.print(activeSweepConfig_.startHz); output_.print(" / ");
  output_.print(activeSweepConfig_.incrementHz); output_.print(" / "); output_.println(sweepTargetPoints_ ? sweepTargetPoints_ - 1 : 0);
  output_.print("Points configured: "); output_.println(sweepTargetPoints_);
  output_.print("Settling: "); output_.print(activeSweepSettling_); output_.println(" cycles x1");
  output_.print("Points valid: "); output_.print(sweepPointCount_); output_.print("/"); output_.println(sweepTargetPoints_);
  output_.print("Status: "); output_.println(sweepComplete_ ? "COMPLETE" : sweepError_ ?
      (strstr(sweepError_, "cancel") ? "CANCELLED (partial)" : "ERROR (partial)") : "INCOMPLETE (partial)");
  output_.println("Idx  Freq(Hz)  REAL  IMAG  Magnitude  Phase(deg)  Validity");
  for (uint16_t i = 0; i < sweepPointCount_; ++i) printSweepPoint(sweepPoints_[i]);
}

void ImpedanceApp::printCalibration(const MuxApp::CalibrationInfo& calibration) {
  output_.print("ADG715 preset : "); output_.print(calibration.name); output_.print(" (");
  printHex(calibration.mask, 2); output_.println(")");
  output_.print("RFB nominal   : "); output_.print(calibration.rfbOhms); output_.println(" ohm");
  output_.print("RCAL nominal  : "); output_.print(calibration.rcalOhms); output_.println(" ohm");
}

void ImpedanceApp::measureConfig() {
  uint32_t code = 0;
  const bool codeValid = AD5934::frequencyToCode(profile_.frequencyHz, profile_.mclkHz, code);
  output_.println("Perfil de adquisicion AD5934:");
  MuxApp::CalibrationInfo calibration{};
  if (mux_.resolveCalibrationPreset(profile_.calibrationPreset, calibration)) printCalibration(calibration);
  else output_.println("ADG715 preset : invalido");
  output_.print("MCLK          : "); output_.print(profile_.mclkHz);
  output_.println(profile_.externalClock ? " Hz, externo" : " Hz, interno");
  output_.print("Frequency     : "); output_.print(profile_.frequencyHz); output_.println(" Hz solicitado");
  output_.print("DDS code      : "); if (codeValid) printHex(code, 6); else output_.print("invalido"); output_.println();
  output_.print("Excitation    : Range "); output_.print(profile_.excitationRange);
  output_.println(profile_.excitationRange == 1 ? " / 2 Vpp nominal" : "");
  output_.print("PGA           : x"); output_.println(profile_.pgaGain);
  output_.print("Increments    : "); output_.print(profile_.numberOfIncrements);
  output_.print("; increment Hz: "); output_.println(profile_.frequencyIncrementHz);
  output_.print("Settling      : "); output_.print(profile_.settlingCycles);
  output_.println(" cycles, multiplicador x1");
  output_.print("Initialize wait: "); output_.print(profile_.startSettlingCycles);
  output_.println(" ciclos nominales (temporizados por firmware)");
}

void ImpedanceApp::measureStatus() {
  output_.print("Measure state : "); output_.println(measurementStateName());
  uint32_t elapsed = measurementElapsedMs_;
  if (busy(this)) elapsed = millis() - measurementStartedMs_;
  output_.print("Elapsed       : "); output_.print(elapsed); output_.println(" ms");
  output_.print("Last error    : "); output_.println(measurementError_ ? measurementError_ : "ninguno");
  if (hasResult_) printCalibration(result_.calibration);
  if (measurementError_) printMeasurementError();
}

void ImpedanceApp::measureResult() {
  if (!hasResult_) { output_.println("No hay resultado DFT valido disponible."); return; }
  output_.println("Resultado AD5934 (DFT cruda; no es impedancia):");
  output_.print("Frequency     : "); output_.print(result_.frequencyHz); output_.println(" Hz");
  printCalibration(result_.calibration);
  output_.print("REAL          : "); output_.println(result_.real);
  output_.print("IMAG          : "); output_.println(result_.imaginary);
  output_.print("Magnitude     : "); output_.println(result_.magnitude, 3);
  output_.print("Elapsed       : "); output_.print(result_.elapsedMs); output_.println(" ms");
  output_.println("Status        : VALID");
}

const char* ImpedanceApp::measurementStateName() const {
  switch (measurementState_) {
    case MeasurementState::Idle: return "IDLE";
    case MeasurementState::Configuring: return "CONFIGURING";
    case MeasurementState::PreparingStandby: return "PREPARING_STANDBY";
    case MeasurementState::Initializing: return "INITIALIZING";
    case MeasurementState::Settling: return "SETTLING";
    case MeasurementState::Acquiring: return "ACQUIRING";
    case MeasurementState::ReadingReal: return "READING_REAL";
    case MeasurementState::ReadingImaginary: return "READING_IMAGINARY";
    case MeasurementState::Incrementing: return "INCREMENTING";
    case MeasurementState::Stopping: return "STOPPING";
    case MeasurementState::Complete: return "COMPLETE";
    case MeasurementState::Error: return "ERROR";
    case MeasurementState::Cancelled: return "CANCELLED";
  }
  return "UNKNOWN";
}

bool ImpedanceApp::parseHex(const char* token, uint32_t maximum, uint32_t& result) {
  if (token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) token += 2;
  if (!*token) return false;
  uint32_t value = 0;
  for (; *token; ++token) {
    uint8_t digit;
    if (*token >= '0' && *token <= '9') digit = *token - '0';
    else if (*token >= 'a' && *token <= 'f') digit = *token - 'a' + 10;
    else if (*token >= 'A' && *token <= 'F') digit = *token - 'A' + 10;
    else return false;
    if (digit > maximum || value > (maximum - digit) / 16) return false;
    value = value * 16 + digit;
  }
  result = value;
  return true;
}

bool ImpedanceApp::parseDecimal(const char* token, uint32_t maximum, uint32_t& result) {
  if (!*token) return false;
  uint32_t value = 0;
  for (; *token; ++token) {
    if (*token < '0' || *token > '9') return false;
    const uint8_t digit = static_cast<uint8_t>(*token - '0');
    if (value > (maximum - digit) / 10 || digit > maximum) return false;
    value = value * 10 + digit;
  }
  result = value;
  return true;
}

bool ImpedanceApp::read(uint8_t reg, uint8_t width, uint32_t& value) {
  if (width == 1) {
    uint8_t result;
    if (!driver_.readRegister8(reg, result)) return false;
    value = result; return true;
  }
  if (width == 2) {
    uint16_t result;
    if (!driver_.readRegister16(reg, result)) return false;
    value = result; return true;
  }
  return driver_.readRegister24(reg, value);
}

void ImpedanceApp::write(uint8_t reg, uint8_t width, uint32_t value) {
  if (!driver_.validateWrite(reg, width, value)) { error(); return; }
  uint16_t control;
  if (!driver_.readRegister16(AD5934::CONTROL_HIGH, control)) { error(); return; }
  const uint8_t mode = (control >> 12) & 0x0F;
  if ((mode != 0x0A && mode != 0x0B) || (control & 0xFF) != 0x08) {
    output_.println("Rechazado: se requiere power-down/standby y Control Low=0x08. No se cambia el modo.");
    return;
  }
  uint32_t previous;
  if (!read(reg, width, previous)) { error(); return; }
  output_.print("Valor anterior: "); printValue(reg, width, previous);
  const bool written = width == 1 ? driver_.writeRegister8(reg, static_cast<uint8_t>(value)) :
                       width == 2 ? driver_.writeRegister16(reg, static_cast<uint16_t>(value)) :
                                    driver_.writeRegister24(reg, value);
  if (!written) {
    error(); output_.println("Escritura no confirmada: puede haber cambios parciales. Releer antes de restaurar; sin reintentos automaticos.");
    return;
  }
  uint32_t actual;
  if (!read(reg, width, actual)) { error(); output_.println("Escritura enviada pero verificacion fallida; configuracion no confirmada."); return; }
  uint32_t mask = 0;
  for (uint8_t index = 0; index < width; ++index) mask = (mask << 8) | AD5934::meaningfulMask(reg + index);
  printValue(reg, width, actual);
  output_.println((actual & mask) == value ? "Escritura verificada en bits definidos." :
                  "Error: la relectura no coincide; configuracion no confirmada.");
}

void ImpedanceApp::status() {
  uint8_t value;
  if (!driver_.readStatus(value)) { error(); return; }
  printValue(AD5934::STATUS, 1, value);
  output_.print("Datos real/imaginario validos (D1): ");
  output_.println((value & AD5934::STATUS_DATA_VALID) ? "si" : "no (normal antes de convertir)");
  output_.print("Barrido completado (D2): ");
  output_.println((value & AD5934::STATUS_SWEEP_COMPLETE) ? "si" : "no");
  output_.println("Los demas bits no se interpretan. No se inicia ninguna conversion.");
}

void ImpedanceApp::recordError(ImpedanceDiagnostics::ErrorCode code,
                               ImpedanceDiagnostics::Subsystem subsystem,
                               const char* detail, bool recoverable) {
  using namespace ImpedanceDiagnostics;
  lastErrorRecord_ = ErrorRecord{code, subsystem,
      static_cast<uint8_t>(measurementState_), millis(), detail, recoverable,
      RecoveryState::NotAttempted};
  publicStatus_.lastError = code;
  publicStatus_.activeError = code;
  if (static_cast<uint8_t>(code) < static_cast<uint8_t>(ErrorCode::Count))
    ++errorCounters_[static_cast<uint8_t>(code)];
}

void ImpedanceApp::refreshPublicStatus() {
  publicStatus_.busy = busy(this);
  publicStatus_.pointsCompleted = sweepPointCount_;
  publicStatus_.pointsExpected = sweepTargetPoints_;
  publicStatus_.elapsedMs = publicStatus_.busy ? millis() - measurementStartedMs_ :
      (sweepMode_ ? sweepElapsedMs_ : measurementElapsedMs_);
  publicStatus_.lastError = lastErrorRecord_.code;
  publicStatus_.hardwareReady = publicStatus_.ad5934Communication &&
      publicStatus_.adg715Communication && publicStatus_.configurationVerified &&
      publicStatus_.operational && publicStatus_.diagnosticComplete &&
      publicStatus_.adg715MaskKnown;
}

void ImpedanceApp::integratedStatus() {
  refreshPublicStatus();
  output_.println("Estado integrado CN-0349:");
  output_.print("Busy: "); output_.println(publicStatus_.busy ? "si" : "no");
  output_.print("AD5934 comunicacion: "); output_.println(publicStatus_.ad5934Communication ? "disponible" : "no confirmada");
  output_.print("ADG715 comunicacion: "); output_.println(publicStatus_.adg715Communication ? "disponible" : "no confirmada");
  output_.print("Configuracion AD5934: "); output_.println(publicStatus_.configurationVerified ? "verificada" : "desconocida/no verificada");
  output_.print("ADG715 mask: ");
  if (publicStatus_.adg715MaskKnown) printHex(publicStatus_.adg715Mask, 2);
  else output_.print("desconocida");
  output_.println();
  output_.print("Diagnostico completo: "); output_.println(publicStatus_.diagnosticComplete ? "si" : "no");
  output_.print("Operativo: "); output_.println(publicStatus_.operational ? "si" : "no confirmado");
  output_.print("Hardware ready: "); output_.println(publicStatus_.hardwareReady ? "si" : "no");
  output_.print("Operacion: "); output_.println(measurementStateName());
  output_.print("Puntos: "); output_.print(publicStatus_.pointsCompleted); output_.print("/"); output_.println(publicStatus_.pointsExpected);
  output_.print("Ultima operacion exitosa: "); output_.println(publicStatus_.lastOperationSuccessful ? "si" : "no");
  output_.print("Error activo: "); output_.println(ImpedanceDiagnostics::errorName(publicStatus_.activeError));
  output_.print("Ultimo error: "); output_.println(ImpedanceDiagnostics::errorName(publicStatus_.lastError));
  uint8_t rawStatus = 0;
  if (!busy(this) && driver_.readStatus(rawStatus)) {
    output_.print("AD5934 Status raw: "); printHex(rawStatus, 2);
    output_.print("; D1="); output_.print((rawStatus & AD5934::STATUS_DATA_VALID) ? "valid" : "no");
    output_.print("; D2="); output_.println((rawStatus & AD5934::STATUS_SWEEP_COMPLETE) ? "complete" : "no");
  }
  output_.println("ACK solo confirma comunicacion en una direccion; no identidad ni ruta analogica.");
}

void ImpedanceApp::errors(bool clear) {
  using namespace ImpedanceDiagnostics;
  if (clear) {
    if (busy(this)) { output_.println("Rechazado: no se borra el historial durante una operacion activa."); return; }
    if (publicStatus_.activeError != ErrorCode::None) {
      output_.println("Rechazado: hay un error activo; ejecute diag/recover antes de limpiar el historial."); return;
    }
    memset(errorCounters_, 0, sizeof(errorCounters_));
    lastErrorRecord_ = ErrorRecord{};
    publicStatus_.lastError = ErrorCode::None;
    output_.println("Contadores e historial software borrados; estado hardware no modificado.");
    return;
  }
  output_.print("Error activo: "); output_.println(errorName(publicStatus_.activeError));
  output_.print("Ultimo error: "); output_.println(errorName(lastErrorRecord_.code));
  if (lastErrorRecord_.code != ErrorCode::None) {
    output_.print("Subsystem: "); output_.println(lastErrorRecord_.subsystem == Subsystem::Ad5934 ? "AD5934" :
        lastErrorRecord_.subsystem == Subsystem::Adg715 ? "ADG715" : lastErrorRecord_.subsystem == Subsystem::Bus ? "I2C" : "APP");
    output_.print("Phase: "); output_.println(lastErrorRecord_.phase);
    output_.print("Time millis: "); output_.println(lastErrorRecord_.timestampMs);
    output_.print("Detail: "); output_.println(lastErrorRecord_.detail);
    output_.print("Recoverable: "); output_.println(lastErrorRecord_.recoverable ? "si" : "no");
    output_.print("Recovery: "); output_.println(lastErrorRecord_.recovery == RecoveryState::Complete ? "complete" :
        lastErrorRecord_.recovery == RecoveryState::Partial ? "partial" : lastErrorRecord_.recovery == RecoveryState::Failed ? "failed" : "not attempted");
  }
  output_.println("Counters:");
  for (uint8_t index = 1; index < static_cast<uint8_t>(ErrorCode::Count); ++index) {
    output_.print("  "); output_.print(errorName(static_cast<ErrorCode>(index))); output_.print(": "); output_.println(errorCounters_[index]);
  }
}

void ImpedanceApp::diagnostics() {
  using namespace ImpedanceDiagnostics;
  if (busy(this)) { recordError(ErrorCode::Busy, Subsystem::Application, "diagnostic requested while busy", false); output_.println("Diagnostico rechazado: subsistema ocupado."); return; }
  output_.println("Diagnostico integrado no destructivo:");
  publicStatus_.diagnosticComplete = false;
  publicStatus_.operational = false;
  publicStatus_.configurationVerified = false;
  publicStatus_.ad5934Communication = false;
  publicStatus_.adg715MaskKnown = false;
  const bool adgAck = mux_.ping();
  publicStatus_.adg715Communication = adgAck;
  publicStatus_.ad5934Communication = driver_.begin();
  output_.print("AD5934 0x"); output_.print(driver_.address(), HEX); output_.print(": "); output_.println(publicStatus_.ad5934Communication ? "ACK" : "sin respuesta");
  output_.print("ADG715 0x"); output_.print(mux_.address(), HEX); output_.print(": "); output_.println(adgAck ? "ACK" : "sin respuesta");
  uint16_t control = 0;
  uint32_t start = 0, step = 0;
  uint16_t increments = 0, settling = 0;
  uint8_t statusByte = 0, mask = 0;
  bool registersOk = publicStatus_.ad5934Communication &&
      driver_.readRegister16(AD5934::CONTROL_HIGH, control) &&
      driver_.readRegister24(AD5934::START_FREQUENCY_MSB, start) &&
      driver_.readRegister24(AD5934::FREQUENCY_INCREMENT_MSB, step) &&
      driver_.readRegister16(AD5934::NUMBER_INCREMENTS_MSB, increments) &&
      driver_.readRegister16(AD5934::SETTLING_CYCLES_MSB, settling) && driver_.readStatus(statusByte);
  const uint8_t mode = static_cast<uint8_t>((control >> 12) & 0x0F);
  const bool safeControl = registersOk && (mode == 0x0A || mode == 0x0B) && (control & 0xFF) == 0x08;
  const bool validConfig = registersOk && driver_.validateWrite(AD5934::START_FREQUENCY_MSB, 3, start) &&
      driver_.validateWrite(AD5934::FREQUENCY_INCREMENT_MSB, 3, step) &&
      driver_.validateWrite(AD5934::NUMBER_INCREMENTS_MSB, 2, increments) &&
      driver_.validateWrite(AD5934::SETTLING_CYCLES_MSB, 2, settling);
  output_.print("AD5934 control seguro: "); output_.println(safeControl ? "si" : "no/desconocido");
  output_.print("Configuracion relevante valida: "); output_.println(validConfig ? "si" : "no/desconocida");
  output_.print("Status: "); if (registersOk) printHex(statusByte, 2); else output_.print("lectura fallida"); output_.println();
  const bool maskOk = adgAck && mux_.readMask(mask);
  publicStatus_.adg715MaskKnown = maskOk;
  publicStatus_.adg715Mask = mask;
  publicStatus_.configurationVerified = validConfig && safeControl;
  publicStatus_.operational = publicStatus_.configurationVerified && maskOk && mux_.knownPresetMask(mask);
  publicStatus_.diagnosticComplete = registersOk && maskOk && adgAck;
  publicStatus_.lastOperationSuccessful = publicStatus_.diagnosticComplete;
  if (maskOk) { output_.print("ADG715 mask readback: "); printHex(mask, 2); output_.print("; preset conocido: "); output_.println(mux_.knownPresetMask(mask) ? "si" : "no"); }
  else output_.println("ADG715 mask: lectura fallida");
  if (!publicStatus_.diagnosticComplete) {
    const ADG715::Error muxError = mux_.lastDriverError();
    const bool adgFailure = !adgAck || !maskOk;
    const ErrorCode code = muxError == ADG715::Error::AddressNack || muxError == ADG715::Error::DataNack ? ErrorCode::I2cNack :
        muxError == ADG715::Error::Timeout ? ErrorCode::I2cTimeout :
        muxError == ADG715::Error::VerifyMismatch ? ErrorCode::Adg715VerifyFailed : ErrorCode::I2cError;
    recordError(code, adgFailure ? Subsystem::Adg715 : Subsystem::Ad5934,
                adgFailure ? mux_.lastErrorText() : AD5934::errorText(driver_.lastError().code), true);
  }
  else if (!publicStatus_.operational) recordError(ErrorCode::InvalidState, Subsystem::Application, "hardware state/configuration not verified safe", true);
  else publicStatus_.activeError = ErrorCode::None;
  refreshPublicStatus();
  output_.print("Resultado diagnostico: "); output_.println(publicStatus_.hardwareReady ? "READY" : "DEGRADED/UNKNOWN");
  output_.println("La lectura del ADG715 no verifica electricamente la ruta analogica.");
}

void ImpedanceApp::recover() {
  using namespace ImpedanceDiagnostics;
  if (busy(this)) { recordError(ErrorCode::Busy, Subsystem::Application, "recovery requested while busy", false); output_.println("Recuperacion rechazada: subsistema ocupado."); return; }
  recoveryAttempted_ = true;
  publicStatus_.hardwareReady = false;
  publicStatus_.operational = false;
  publicStatus_.configurationVerified = false;
  publicStatus_.diagnosticComplete = false;
  publicStatus_.adg715MaskKnown = false;
  output_.println("Recuperacion controlada; preset ADG715 conservado.");
  const bool ad5934Ack = driver_.begin();
  const bool adgAck = mux_.ping();
  publicStatus_.ad5934Communication = ad5934Ack;
  publicStatus_.adg715Communication = adgAck;
  uint8_t mask = 0;
  const bool maskRead = adgAck && mux_.readMask(mask);
  if (maskRead) { publicStatus_.adg715Mask = mask; publicStatus_.adg715MaskKnown = true; }
  uint16_t before = 0, after = 0;
  const bool standby = ad5934Ack && driver_.enterStandby(before, after);
  uint8_t statusByte = 0;
  const bool statusRead = standby && driver_.readStatus(statusByte);
  uint32_t startCode = 0, incrementCode = 0;
  uint16_t incrementCount = 0, settlingWord = 0;
  const bool configRead = statusRead &&
      driver_.readRegister24(AD5934::START_FREQUENCY_MSB, startCode) &&
      driver_.readRegister24(AD5934::FREQUENCY_INCREMENT_MSB, incrementCode) &&
      driver_.readRegister16(AD5934::NUMBER_INCREMENTS_MSB, incrementCount) &&
      driver_.readRegister16(AD5934::SETTLING_CYCLES_MSB, settlingWord);
  const bool configValid = configRead &&
      driver_.validateWrite(AD5934::START_FREQUENCY_MSB, 3, startCode) &&
      driver_.validateWrite(AD5934::FREQUENCY_INCREMENT_MSB, 3, incrementCode) &&
      driver_.validateWrite(AD5934::NUMBER_INCREMENTS_MSB, 2, incrementCount) &&
      driver_.validateWrite(AD5934::SETTLING_CYCLES_MSB, 2, settlingWord);
  const bool knownPreset = maskRead && mux_.knownPresetMask(mask);
  publicStatus_.configurationVerified = standby && configValid;
  publicStatus_.operational = publicStatus_.configurationVerified && knownPreset;
  publicStatus_.diagnosticComplete = ad5934Ack && adgAck && maskRead && configRead;
  publicStatus_.lastOperationSuccessful = publicStatus_.operational;
  if (publicStatus_.operational) {
    publicStatus_.activeError = ErrorCode::None;
    if (lastErrorRecord_.code != ErrorCode::None) lastErrorRecord_.recovery = RecoveryState::Complete;
    output_.println("Recuperacion completa: AD5934 en standby verificado; mascara ADG715 conservada y leida.");
  } else {
    if (lastErrorRecord_.code == ErrorCode::None)
      recordError(ErrorCode::RecoveryFailed, Subsystem::Application, "recovery verification incomplete", true);
    else {
      publicStatus_.activeError = ErrorCode::RecoveryFailed;
      ++errorCounters_[static_cast<uint8_t>(ErrorCode::RecoveryFailed)];
    }
    lastErrorRecord_.recovery = (ad5934Ack || adgAck) ? RecoveryState::Partial : RecoveryState::Failed;
    output_.println((ad5934Ack || adgAck) ? "Recuperacion parcial: estado no completamente verificado." : "Recuperacion fallida: dispositivos sin respuesta.");
  }
  refreshPublicStatus();
}

void ImpedanceApp::failMeasurement(const char* message, bool recover) {
  measurementError_ = message;
  if (sweepMode_) sweepError_ = message;
  driverError_ = driver_.lastError();
  cancelled_ = strstr(message, "cancel") != nullptr;
  using namespace ImpedanceDiagnostics;
  ErrorCode code = ErrorCode::InvalidState;
  Subsystem subsystem = Subsystem::Application;
  bool recoverable = true;
  if (driverError_.code == AD5934::ErrorCode::AddressNack || driverError_.code == AD5934::ErrorCode::DataNack) {
    code = ErrorCode::I2cNack; subsystem = Subsystem::Ad5934;
  } else if (driverError_.code == AD5934::ErrorCode::Timeout) {
    code = ErrorCode::I2cTimeout; subsystem = Subsystem::Ad5934;
  } else if (driverError_.code == AD5934::ErrorCode::Bus || driverError_.code == AD5934::ErrorCode::ShortRead) {
    code = ErrorCode::I2cError; subsystem = Subsystem::Ad5934;
  } else if (strstr(message, "timeout")) {
    code = sweepMode_ ? ErrorCode::SweepTimeout : ErrorCode::DftTimeout;
  } else if (strstr(message, "readback") || strstr(message, "verificar")) {
    code = ErrorCode::RegisterVerifyFailed; subsystem = Subsystem::Ad5934;
  } else if (strstr(message, "preset") || strstr(message, "ADG715")) {
    code = ErrorCode::Adg715VerifyFailed; subsystem = Subsystem::Adg715;
  }
  if (!cancelled_) recordError(code, subsystem, message, recoverable);
  publicStatus_.lastOperationSuccessful = false;
  if (recover) setMeasurementState(MeasurementState::Stopping);
  else setMeasurementState(MeasurementState::Error);
}

bool ImpedanceApp::startMeasurement(bool busReady, bool sweep) {
  if (!busReady) { output_.println("Error: bus I2C no inicializado."); return false; }
  if (busy(this)) { output_.println("Error: ya hay una operacion AD5934 activa."); return false; }
  refreshPublicStatus();
  if (!publicStatus_.hardwareReady) {
    recordError(ImpedanceDiagnostics::ErrorCode::InvalidState, ImpedanceDiagnostics::Subsystem::Application,
                "hardware readiness unknown; run diag or recover", true);
    output_.println("Error: estado hardware desconocido/degradado; ejecute ad5934 diag o recover antes de medir.");
    return false;
  }
  uint32_t sweepFinalHz = 0;
  if (sweep) {
    if (!validateSweep(sweepFinalHz, startCode_, incrementCode_)) {
      recordError(ImpedanceDiagnostics::ErrorCode::InvalidArgument, ImpedanceDiagnostics::Subsystem::Application,
                  "invalid sweep frequency/step/increment configuration", false);
      output_.println("Error: sweep invalido: limite DDS/frecuencia final, step o maximo 64 puntos."); return false;
    }
    activeStartHz_ = sweepConfig_.startHz;
    activeIncrementHz_ = sweepConfig_.incrementHz;
    activeIncrements_ = sweepConfig_.increments;
  } else if (!AD5934::frequencyToCode(profile_.frequencyHz, profile_.mclkHz, startCode_)) {
    recordError(ImpedanceDiagnostics::ErrorCode::InvalidArgument, ImpedanceDiagnostics::Subsystem::Application,
                "requested frequency outside supported range", false);
    output_.println("Error: frecuencia fuera de rango para MCLK externo de 1 MHz."); return false;
  } else {
    incrementCode_ = 0;
    activeStartHz_ = profile_.frequencyHz;
    activeIncrementHz_ = 0;
    activeIncrements_ = 0;
  }
  MuxApp::CalibrationInfo selectedCalibration{};
  if (!mux_.resolveCalibrationPreset(profile_.calibrationPreset, selectedCalibration)) {
    output_.println("Error: preset de calibracion no permitido."); return false;
  }
  if (!profile_.externalClock || profile_.excitationRange != 1 || profile_.pgaGain != 1 ||
      (!sweep && (profile_.numberOfIncrements != 0 || incrementCode_ != 0))) {
    output_.println("Error: el perfil contiene opciones fuera del alcance implementado.");
    return false;
  }
  sweepMode_ = sweep;
  if (sweep) {
    hasSweep_ = true;
    sweepComplete_ = false;
    sweepPointCount_ = 0;
    sweepTargetPoints_ = static_cast<uint16_t>(sweepConfig_.increments + 1);
    sweepCurrentIndex_ = 0;
    sweepEndHz_ = sweepFinalHz;
    activeSweepConfig_ = sweepConfig_;
    activeSweepCalibration_ = selectedCalibration;
    activeSweepMclkHz_ = profile_.mclkHz;
    activeSweepSettling_ = profile_.settlingCycles;
    sweepElapsedMs_ = 0;
    sweepError_ = nullptr;
  }
  measurementStartedMs_ = millis();
  measurementElapsedMs_ = 0;
  if (!driver_.begin()) { failMeasurement("fallo de comunicacion AD5934", true); return false; }
  uint16_t control;
  if (!driver_.readRegister16(AD5934::CONTROL_HIGH, control)) {
    failMeasurement("no se pudo leer el control inicial", true); return false;
  }
  const uint8_t mode = static_cast<uint8_t>((control >> 12) & 0x0F);
  if ((mode != 0x0A && mode != 0x0B) || (control & 0x00FF) != 0x08) {
    output_.println("Estado inicial inseguro: se cancela la medida e intenta recuperar standby.");
    failMeasurement("estado inicial AD5934 no seguro", true);
    return false;
  }
  publicStatus_.ad5934Communication = true;
  uint8_t observedMask = 0;
  if (!mux_.readMask(observedMask)) {
    recordError(ImpedanceDiagnostics::ErrorCode::I2cError, ImpedanceDiagnostics::Subsystem::Adg715,
                "ADG715 preflight read failed", true);
    output_.print("Error de lectura ADG715: "); output_.println(mux_.lastErrorText());
    return false;
  }
  publicStatus_.adg715Communication = true;
  publicStatus_.adg715Mask = observedMask;
  publicStatus_.adg715MaskKnown = true;
  publicStatus_.operational = mux_.knownPresetMask(observedMask);
  if (!publicStatus_.operational) {
    recordError(ImpedanceDiagnostics::ErrorCode::InvalidState, ImpedanceDiagnostics::Subsystem::Adg715,
                "ADG715 mask is not a recognized route", true);
    output_.println("Estado degradado: mascara ADG715 desconocida; no se inicia adquisicion.");
    return false;
  }
  publicStatus_.diagnosticComplete = true;
  if (!mux_.applyCalibrationPreset(profile_.calibrationPreset)) {
    output_.println("No se inicia DFT porque fallo la seleccion/readback ADG715; se verificara standby.");
    failMeasurement("no se pudo aplicar/verificar preset de calibracion", true);
    return false;
  }
  publicStatus_.adg715Mask = selectedCalibration.mask;
  publicStatus_.adg715MaskKnown = true;
  publicStatus_.adg715Communication = true;
  publicStatus_.operational = true;
  measurementCalibration_ = selectedCalibration;
  hasResult_ = false;
  measurementError_ = nullptr;
  configPhase_ = 0;
  verifyPhase_ = 0;
  setMeasurementState(MeasurementState::Configuring);
  publicStatus_.lastOperationSuccessful = false;
  output_.println(sweep ? "Barrido DFT iniciado; configurando registros verificados." :
                          "Adquisicion DFT unica iniciada; configurando registros verificados.");
  return true;
}

void ImpedanceApp::setMeasurementState(MeasurementState state) {
  measurementState_ = state;
  stateStartedMs_ = millis();
}

void ImpedanceApp::printMeasurementError() {
  if (driverError_.code == AD5934::ErrorCode::None) {
    output_.println("Fallo de aplicacion/timeout; sin error I2C capturado.");
    return;
  }
  output_.print("AD5934 error detail: ");
  output_.print(AD5934::errorText(driverError_.code));
  output_.print("; fase="); output_.print(AD5934::phaseText(driverError_.phase));
  output_.print("; registro="); printHex(driverError_.reg, 2);
  output_.print("; bytes completados="); output_.println(driverError_.completedBytes);
}

void ImpedanceApp::printOutput() {
  output_.println("ATLANTES - AD5934 Single Measurement");
  output_.println("-------------------------------------");
  printCalibration(measurementCalibration_);
  output_.print("MCLK          : "); output_.print(profile_.mclkHz); output_.println(" Hz external (CN-0349 oscillator)");
  output_.print("Frequency     : "); output_.print(result_.frequencyHz); output_.println(" Hz requested");
  output_.print("Excitation    : Range "); output_.print(profile_.excitationRange); output_.println(" / 2 Vpp nominal");
  output_.print("PGA           : x"); output_.println(profile_.pgaGain);
  output_.print("Settling      : "); output_.print(profile_.settlingCycles); output_.println(" cycles x1");
  output_.println("Status        : VALID (D1)");
  output_.print("REAL          : "); output_.println(result_.real);
  output_.print("IMAG          : "); output_.println(result_.imaginary);
  output_.print("Magnitude     : "); output_.println(result_.magnitude, 3);
  output_.print("Elapsed       : "); output_.print(result_.elapsedMs); output_.println(" ms");
  output_.println("Magnitude is a raw DFT code, not ohms.");
}

void ImpedanceApp::finishStopping() {
  uint16_t previous = 0, verified = 0;
  if (!driver_.enterStandby(previous, verified)) {
    driverError_ = driver_.lastError();
    if (measurementError_) output_.println("Recuperacion: fallo al llevar AD5934 a standby.");
    else measurementError_ = "fallo de recuperacion a standby";
    if (sweepMode_) sweepError_ = "fallo de recuperacion a standby";
    printMeasurementError();
    setMeasurementState(MeasurementState::Error);
    publicStatus_.operational = false;
    publicStatus_.configurationVerified = false;
    publicStatus_.diagnosticComplete = false;
    publicStatus_.activeError = ImpedanceDiagnostics::ErrorCode::RecoveryFailed;
    publicStatus_.hardwareReady = false;
    ++errorCounters_[static_cast<uint8_t>(ImpedanceDiagnostics::ErrorCode::RecoveryFailed)];
    if (lastErrorRecord_.code != ImpedanceDiagnostics::ErrorCode::None) lastErrorRecord_.recovery = ImpedanceDiagnostics::RecoveryState::Failed;
    return;
  }
  publicStatus_.operational = cancelled_ || !measurementError_;
  if (measurementError_ && lastErrorRecord_.code != ImpedanceDiagnostics::ErrorCode::None) {
    lastErrorRecord_.recovery = ImpedanceDiagnostics::RecoveryState::Complete;
    publicStatus_.activeError = ImpedanceDiagnostics::ErrorCode::None;
    publicStatus_.configurationVerified = false;
    publicStatus_.diagnosticComplete = false;
  }
  measurementElapsedMs_ = millis() - measurementStartedMs_;
  if (sweepMode_) sweepElapsedMs_ = measurementElapsedMs_;
  if (cancelled_) {
    measurementError_ = "cancelada por usuario; AD5934 en standby";
    setMeasurementState(sweepMode_ ? MeasurementState::Cancelled : MeasurementState::Idle);
    output_.println(sweepMode_ ? "Barrido cancelado; puntos parciales conservados y AD5934 en standby." :
                                "Adquisicion cancelada; AD5934 verificado en standby.");
  } else if (measurementError_) {
    setMeasurementState(MeasurementState::Error);
    output_.print("Adquisicion fallida: "); output_.println(measurementError_);
    printMeasurementError();
  } else {
    if (sweepMode_) {
      sweepComplete_ = !measurementError_ && sweepPointCount_ == sweepTargetPoints_;
      setMeasurementState(measurementError_ ? MeasurementState::Error : MeasurementState::Complete);
      output_.print("Barrido "); output_.print(sweepComplete_ ? "completado" : "finalizado parcialmente");
      output_.print(": "); output_.print(sweepPointCount_); output_.print("/"); output_.print(sweepTargetPoints_);
      output_.println(" puntos; AD5934 en standby.");
      publicStatus_.lastOperationSuccessful = sweepComplete_;
    } else {
      result_ = Result{pendingReal_, result_.imaginary, 0, profile_.frequencyHz, measurementElapsedMs_, measurementCalibration_};
      result_.magnitude = AD5934::magnitude(result_.real, result_.imaginary);
      result_.valid = true;
      hasResult_ = true;
      setMeasurementState(MeasurementState::Complete);
      printOutput();
      output_.println("AD5934 finalizado en standby.");
      publicStatus_.lastOperationSuccessful = true;
    }
  }
}

void ImpedanceApp::advanceMeasurement() {
  uint32_t code = 0;
  uint8_t byte = 0;
  const uint32_t now = millis();
  const bool phaseTimed = measurementState_ == MeasurementState::Configuring ||
      measurementState_ == MeasurementState::PreparingStandby ||
      measurementState_ == MeasurementState::Initializing ||
      measurementState_ == MeasurementState::Incrementing ||
      measurementState_ == MeasurementState::Stopping;
  if (phaseTimed && now - stateStartedMs_ >= APPLICATION_PHASE_TIMEOUT_MS) {
    failMeasurement("timeout en fase de control/configuracion AD5934", true);
    return;
  }
  switch (measurementState_) {
    case MeasurementState::Configuring: {
      if (configPhase_ == 0) {
        if (!driver_.configureRange1PgaX1()) { failMeasurement("fallo al configurar control Range 1/PGA x1", true); return; }
        configPhase_ = 1;
        setMeasurementState(MeasurementState::PreparingStandby);
        return;
      } else {
        const uint8_t field = configPhase_ - 1;
        if (field < 3) {
          byte = static_cast<uint8_t>(startCode_ >> (16 - 8 * field));
          if (!driver_.writeRegister8(AD5934::START_FREQUENCY_MSB + field, byte)) { failMeasurement("fallo al escribir frecuencia DDS", true); return; }
        } else if (field < 6) {
          byte = static_cast<uint8_t>(incrementCode_ >> (16 - 8 * (field - 3)));
          if (!driver_.writeRegister8(AD5934::FREQUENCY_INCREMENT_MSB + field - 3, byte)) { failMeasurement("fallo al escribir incremento", true); return; }
        } else if (field < 8) {
          byte = static_cast<uint8_t>(activeIncrements_ >> (field == 6 ? 8 : 0));
          if (!driver_.writeRegister8(AD5934::NUMBER_INCREMENTS_MSB + field - 6, byte)) { failMeasurement("fallo al escribir numero de incrementos", true); return; }
        } else {
          const uint16_t settling = profile_.settlingCycles;
          byte = field == 8 ? static_cast<uint8_t>(settling >> 8) : static_cast<uint8_t>(settling);
          if (!driver_.writeRegister8(field == 8 ? AD5934::SETTLING_CYCLES_MSB : AD5934::SETTLING_CYCLES_LSB, byte)) { failMeasurement("fallo al escribir settling cycles", true); return; }
        }
      }
      if (++configPhase_ >= CONFIG_FIELD_COUNT) { verifyPhase_ = 0; setMeasurementState(MeasurementState::Initializing); }
      return;
    }
    case MeasurementState::PreparingStandby: {
      uint16_t previous = 0, verified = 0;
      if (!driver_.wakeToStandby(previous, verified)) {
        failMeasurement("fallo al pasar de power-down a standby", true);
        return;
      }
      setMeasurementState(MeasurementState::Configuring);
      return;
    }
    case MeasurementState::Initializing:
      if (verifyPhase_ < CONFIG_FIELD_COUNT) {
        const uint8_t field = verifyPhase_++;
        uint8_t reg;
        uint8_t expected;
        if (field == 0) { reg = AD5934::CONTROL_HIGH; expected = 0xB1; }
        else if (field < 4) { reg = AD5934::START_FREQUENCY_MSB + field - 1; expected = static_cast<uint8_t>(startCode_ >> (16 - 8 * (field - 1))); }
        else if (field < 7) { reg = AD5934::FREQUENCY_INCREMENT_MSB + field - 4; expected = static_cast<uint8_t>(incrementCode_ >> (16 - 8 * (field - 4))); }
        else if (field < 9) { reg = AD5934::NUMBER_INCREMENTS_MSB + field - 7; expected = static_cast<uint8_t>(activeIncrements_ >> (field == 7 ? 8 : 0)); }
        else { reg = field == 9 ? AD5934::SETTLING_CYCLES_MSB : AD5934::SETTLING_CYCLES_LSB; expected = field == 9 ? static_cast<uint8_t>(profile_.settlingCycles >> 8) : static_cast<uint8_t>(profile_.settlingCycles); }
        if (!driver_.readRegister8(reg, byte)) { failMeasurement("fallo al verificar perfil AD5934", true); return; }
        if (byte != expected) { failMeasurement("readback de perfil AD5934 no coincide", true); return; }
        return;
      }
      if (!driver_.sendCommand(AD5934::ControlCommand::InitializeStartFrequency)) { failMeasurement("fallo al inicializar frecuencia", true); return; }
      setMeasurementState(MeasurementState::Settling);
      return;
    case MeasurementState::Settling: {
      const uint32_t waitMs = (static_cast<uint32_t>(profile_.startSettlingCycles) * 1000U + activeStartHz_ - 1) / activeStartHz_;
      if (now - stateStartedMs_ < (waitMs ? waitMs : 1)) return;
      if (!driver_.sendCommand(AD5934::ControlCommand::StartFrequencySweep)) { failMeasurement("fallo al iniciar DFT", true); return; }
      lastPollMs_ = now;
      acquisitionStartedMs_ = now;
      setMeasurementState(MeasurementState::Acquiring);
      return;
    }
    case MeasurementState::Acquiring:
      if (now - acquisitionStartedMs_ >= ACQUISITION_TIMEOUT_MS) { failMeasurement("timeout esperando D1 de datos DFT", true); return; }
      if (now - lastPollMs_ < STATUS_POLL_INTERVAL_MS) return;
      lastPollMs_ = now;
      if (!driver_.readStatus(byte)) { failMeasurement("fallo I2C leyendo status DFT", true); return; }
      if (byte & AD5934::STATUS_DATA_VALID) setMeasurementState(MeasurementState::ReadingReal);
      return;
    case MeasurementState::ReadingReal:
      if (!driver_.readReal(pendingReal_)) { failMeasurement("fallo leyendo REAL", true); return; }
      setMeasurementState(MeasurementState::ReadingImaginary);
      return;
    case MeasurementState::ReadingImaginary:
      if (!driver_.readImaginary(result_.imaginary)) { failMeasurement("fallo leyendo IMAG", true); return; }
      measurementError_ = nullptr;
      if (sweepMode_) {
        const uint32_t ddsCode = startCode_ + incrementCode_ * sweepCurrentIndex_;
        SweepPoint& point = sweepPoints_[sweepPointCount_++];
        point.index = sweepCurrentIndex_;
        point.frequencyHz = AD5934Math::frequencyFromCode(ddsCode, profile_.mclkHz);
        point.real = pendingReal_;
        point.imaginary = result_.imaginary;
        point.magnitude = AD5934::magnitude(pendingReal_, result_.imaginary);
        point.phaseDegrees = AD5934Math::phaseDegrees(pendingReal_, result_.imaginary);
        point.valid = true;
        setMeasurementState(MeasurementState::Incrementing);
        return;
      }
      setMeasurementState(MeasurementState::Stopping);
      return;
    case MeasurementState::Incrementing:
      if (sweepPointCount_ >= sweepTargetPoints_) {
        if (!driver_.readStatus(byte)) { failMeasurement("fallo leyendo Status final del barrido", true); return; }
        if (!(byte & AD5934::STATUS_SWEEP_COMPLETE)) {
          failMeasurement("D2 no indica fin tras almacenar el ultimo punto", true);
          return;
        }
        measurementError_ = nullptr;
        setMeasurementState(MeasurementState::Stopping);
        return;
      }
      if (!driver_.sendCommand(AD5934::ControlCommand::IncrementFrequency)) {
        failMeasurement("fallo al incrementar frecuencia", true); return;
      }
      ++sweepCurrentIndex_;
      acquisitionStartedMs_ = now;
      lastPollMs_ = now;
      setMeasurementState(MeasurementState::Acquiring);
      return;
    case MeasurementState::Stopping:
      finishStopping();
      return;
    case MeasurementState::Idle:
    case MeasurementState::Complete:
    case MeasurementState::Error:
    case MeasurementState::Cancelled:
      return;
  }
}

void ImpedanceApp::update() {
  if (dumpActive_) {
    const AD5934::RegisterGroup* group = AD5934::groupAt(dumpGroup_);
    if (dumpOffset_ == 0) {
      output_.print(group->name);
      output_.println(group->first == AD5934::CONTROL_HIGH ? " [R/W; escritura protegida]" : group->writable ? " [R/W]" : " [R]");
    }
    const uint8_t reg = group->first + dumpOffset_;
    uint8_t value;
    if (!driver_.readRegister8(reg, value)) { error(); output_.println("Dump interrumpido por error; no se muestran valores inventados."); dumpActive_ = false; return; }
    printValue(reg, 1, value);
    if (++dumpOffset_ == group->size) { dumpOffset_ = 0; ++dumpGroup_; }
    if (!AD5934::groupAt(dumpGroup_)) { dumpActive_ = false; output_.println("Dump completado: 17 registros."); }
    return;
  }
  if (busy(this)) advanceMeasurement();
}

bool ImpedanceApp::process(char* line, bool busReady, bool busBusy) {
  if (strncmp(line, "ad5934", 6) != 0 || (line[6] != '\0' && line[6] != ' ')) return false;
  char* args[6] = {};
  size_t count = 0;
  char* context = nullptr;
  for (char* token = strtok_r(line, " ", &context); token && count < 6; token = strtok_r(nullptr, " ", &context)) args[count++] = token;
  if (count == 1 || (count == 2 && strcmp(args[1], "help") == 0)) { help(); return true; }
  if (count == 2 && strcmp(args[1], "status") == 0) { integratedStatus(); return true; }
  if (count == 2 && strcmp(args[1], "diag") == 0) {
    if (!busReady) { output_.println("Diagnostico rechazado: bus I2C no inicializado."); return true; }
    if (busBusy) { output_.println("Diagnostico rechazado: bus/subsistema ocupado."); return true; }
    diagnostics(); return true;
  }
  if (count == 2 && strcmp(args[1], "errors") == 0) { errors(false); return true; }
  if (count == 3 && strcmp(args[1], "errors") == 0 && strcmp(args[2], "clear") == 0) { errors(true); return true; }
  if (count == 2 && strcmp(args[1], "recover") == 0) {
    if (!busReady) { output_.println("Recuperacion rechazada: bus I2C no inicializado."); return true; }
    if (busBusy) { output_.println("Recuperacion rechazada: bus/subsistema ocupado."); return true; }
    recover(); return true;
  }
  if (strcmp(args[1], "measure") == 0) {
    if (count >= 4 && strcmp(args[2], "sweep") == 0) {
      if (count == 4 && strcmp(args[3], "help") == 0) { sweepHelp(); return true; }
      if (count == 4 && strcmp(args[3], "config") == 0) { sweepConfig(); return true; }
      if (count == 4 && strcmp(args[3], "status") == 0) { sweepStatus(); return true; }
      if (count == 4 && strcmp(args[3], "result") == 0) { sweepResult(); return true; }
      if (count == 4 && strcmp(args[3], "stop") == 0) {
        if (!busy(this) || !sweepMode_) output_.println("No hay barrido activo.");
        else failMeasurement("barrido cancelado por usuario", true);
        return true;
      }
      if (count == 4 && strcmp(args[3], "start") == 0) {
        if (busBusy) { output_.println("Error: bus ocupado por otra operacion."); return true; }
        startMeasurement(busReady, true);
        return true;
      }
      if (count == 6 && strcmp(args[3], "set") == 0) {
        if (busy(this)) { output_.println("Error: barrido ocupado; configuracion sin cambios."); return true; }
        uint32_t value = 0;
        SweepConfig previous = sweepConfig_;
        if (strcmp(args[4], "start") == 0 && parseDecimal(args[5], profile_.mclkHz / 16, value) && value > 0)
          sweepConfig_.startHz = value;
        else if (strcmp(args[4], "step") == 0 && parseDecimal(args[5], profile_.mclkHz / 16, value))
          sweepConfig_.incrementHz = value;
        else if (strcmp(args[4], "increments") == 0 && parseDecimal(args[5], MAX_SWEEP_POINTS - 1, value))
          sweepConfig_.increments = static_cast<uint16_t>(value);
        else { output_.println("Error: parametro/formato invalido; configuracion sin cambios."); return true; }
        uint32_t end = 0, start = 0, step = 0;
        if (!validateSweep(end, start, step)) {
          sweepConfig_ = previous;
          output_.println("Error: configuracion DDS/frecuencia final invalida; sin cambios.");
          return true;
        }
        output_.println("Configuracion de barrido actualizada en RAM.");
        return true;
      }
      output_.println("Error: comando sweep no valido. Use ad5934 measure sweep help.");
      return true;
    }
    if (count == 3 && strcmp(args[2], "help") == 0) { measureHelp(); return true; }
    if (count == 3 && strcmp(args[2], "config") == 0) { measureConfig(); return true; }
    if (count == 3 && strcmp(args[2], "status") == 0) { measureStatus(); return true; }
    if (count == 3 && strcmp(args[2], "result") == 0) { measureResult(); return true; }
    if (count == 3 && strcmp(args[2], "stop") == 0) {
      if (!busy(this)) output_.println("No hay adquisicion activa.");
      else failMeasurement("cancelada por usuario", true);
      return true;
    }
    if (count == 5 && strcmp(args[2], "set") == 0) {
      if (busy(this)) { output_.println("Error: no se cambia el perfil durante una operacion activa."); return true; }
      if (strcmp(args[3], "preset") == 0) {
        MuxApp::CalibrationInfo calibration{};
        if (!mux_.calibrationPresetByName(args[4], calibration)) {
          output_.println("Error: preset de adquisicion no permitido. Use CAL_HIGH_100R, CAL_HIGH_1K, CAL_LOW_1K o CAL_LOW_10K; configuracion sin cambios.");
          return true;
        }
        profile_.calibrationPreset = calibration.id;
        output_.println("Preset de adquisicion configurado:");
        printCalibration(calibration);
        return true;
      }
      uint32_t value;
      const uint32_t maxHz = profile_.mclkHz / 16;
      if (strcmp(args[3], "freq") == 0 && parseDecimal(args[4], maxHz, value) && value > 0) {
        profile_.frequencyHz = value; output_.print("Frecuencia solicitada: "); output_.print(value); output_.println(" Hz."); return true;
      }
      if (strcmp(args[3], "settling") == 0 && parseDecimal(args[4], 511, value)) {
        profile_.settlingCycles = static_cast<uint16_t>(value); output_.print("Settling cycles: "); output_.println(value); return true;
      }
      output_.println("Error: frecuencia o settling fuera de rango/formato decimal."); return true;
    }
    if (count != 3 || strcmp(args[2], "start") != 0) { output_.println("Error: comando measure no valido. Use ad5934 measure help."); return true; }
    if (busBusy) { output_.println("Error: bus ocupado por otra operacion."); return true; }
    startMeasurement(busReady);
    return true;
  }
  const char* command = args[1];
  const bool isRead = strcmp(command, "read") == 0 || strcmp(command, "read16") == 0 || strcmp(command, "read24") == 0;
  const bool isWrite = strcmp(command, "write") == 0 || strcmp(command, "write16") == 0 || strcmp(command, "write24") == 0;
  const bool isSimple = strcmp(command, "ping") == 0 || strcmp(command, "standby") == 0 || strcmp(command, "status") == 0 || strcmp(command, "dump") == 0;
  if ((!isRead && !isWrite && !isSimple) || count != (isRead ? 3U : isWrite ? 4U : 2U)) { output_.println("Error: comando o argumentos AD5934 no validos. Use ad5934 help."); return true; }
  if (!busReady) { output_.println("Error: bus I2C no inicializado."); return true; }
  if (busBusy || busy(this)) { output_.println("Error: diagnostico/adquisicion I2C en curso; espere o use measure status/stop."); return true; }
  if (isSimple) {
    if (strcmp(command, "ping") == 0) reportConnection(driver_.begin());
    else if (strcmp(command, "standby") == 0) {
      uint16_t before, after;
      if (!driver_.enterStandby(before, after)) { error(); return true; }
      const uint8_t previousMode = before >> 12;
      output_.print("Control previo: "); printHex(before, 4); output_.print("; control verificado: "); printHex(after, 4);
      if (previousMode == 0x0A) output_.println(before == after ? ". Power-down ya estaba normalizado; sin cambios." : ". Power-down conservado; Control Low normalizado a 0x08.");
      else if (before == after) output_.println(". Ya estaba en standby.");
      else output_.println(". Standby confirmado; rango de salida y PGA conservados.");
    } else if (strcmp(command, "status") == 0) status();
    else { dumpActive_ = true; dumpGroup_ = 0; dumpOffset_ = 0; output_.println("AD5934 register dump: solo registros documentados, valores raw."); output_.println("Real/Imaginary pueden no ser validos; consultar D1 de Status. No es una medida."); }
    return true;
  }
  const uint8_t width = strstr(command, "16") ? 2 : strstr(command, "24") ? 3 : 1;
  uint32_t reg, value;
  if (!parseHex(args[2], 0xFF, reg) || (isWrite && !parseHex(args[3], (1UL << (width * 8)) - 1, value))) { output_.println("Error: hexadecimal no valido o valor fuera de rango para el ancho solicitado."); return true; }
  if (isWrite) write(static_cast<uint8_t>(reg), width, value);
  else if (read(static_cast<uint8_t>(reg), width, value)) printValue(reg, width, value);
  else error();
  return true;
}
