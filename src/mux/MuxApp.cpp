#include "MuxApp.h"

#include <ctype.h>
#include <string.h>
#include "../terminal/Terminal.h"

const MuxApp::Preset MuxApp::presets_[] = {
    {MuxApp::CalibrationPreset::CalHigh100R, "CAL_HIGH_100R", 0x09, 100, 100, "RFB=100 ohm, RCAL=100 ohm"},
    {MuxApp::CalibrationPreset::CalHigh1K, "CAL_HIGH_1K", 0x11, 100, 1000, "RFB=100 ohm, RCAL=1 kohm"},
    {MuxApp::CalibrationPreset::CalLow1K, "CAL_LOW_1K", 0x12, 1000, 1000, "RFB=1 kohm, RCAL=1 kohm"},
    {MuxApp::CalibrationPreset::CalLow10K, "CAL_LOW_10K", 0x22, 1000, 10000, "RFB=1 kohm, RCAL=10 kohm"},
    {static_cast<MuxApp::CalibrationPreset>(0xFF), "RTD", 0x41, 100, 0, "RFB=100 ohm, DUT=RTD"},
    {static_cast<MuxApp::CalibrationPreset>(0xFF), "MEASURE_HIGH", 0x81, 100, 0, "RFB=100 ohm, DUT=conductivity cell"},
    {static_cast<MuxApp::CalibrationPreset>(0xFF), "MEASURE_LOW", 0x82, 1000, 0, "RFB=1 kohm, DUT=conductivity cell"},
    {static_cast<MuxApp::CalibrationPreset>(0xFF), "ALL_OFF", 0x00, 0, 0, "todos los interruptores abiertos"},
};

MuxApp::MuxApp(ADG715& mux, AD5934& impedance, Stream& output)
    : mux_(mux), impedance_(impedance), output_(output) {}

void MuxApp::begin(Terminal& terminal) {
  terminal.attachModule(this, &MuxApp::dispatch, &MuxApp::busy,
                        "  adg715 help - Diagnostico y presets seguros del ADG715");
}

void MuxApp::reportConnection(bool ready) {
  output_.print("ADG715 en direccion configurada ");
  printHex(mux_.address());
  output_.println(ready ? ": ACK (sin modificar el estado)." : ": no responde.");
  if (!ready) printLastError();
}

bool MuxApp::selectPreset(const char* name, bool quiet) {
  const Preset* preset = presetByName(name);
  if (!preset) {
    output_.print("Error: preset ADG715 desconocido: ");
    output_.println(name);
    return false;
  }
  uint8_t actual;
  if (!readMask(actual)) {
    printLastError();
    return false;
  }
  if (actual == preset->mask) {
    if (quiet) return true;
    output_.print("ADG715 ya confirma ");
    output_.print(preset->name);
    output_.print(" ");
    printHex(actual);
    output_.println(".");
    return true;
  }
  return setAuthorized(preset->mask, quiet);
}

bool MuxApp::readMask(uint8_t& mask) {
  if (!mux_.readState(mask)) return false;
  observedMask_ = mask;
  hasObservedMask_ = true;
  return true;
}

bool MuxApp::resolveCalibrationPreset(CalibrationPreset id, CalibrationInfo& info) const {
  for (const Preset& candidate : presets_) {
    if (candidate.id == id && id != static_cast<CalibrationPreset>(0xFF)) { info = candidate; return true; }
  }
  return false;
}

bool MuxApp::calibrationPresetByName(const char* name, CalibrationInfo& info) const {
  const Preset* preset = presetByName(name);
  if (!preset) return false;
  for (const Preset& candidate : presets_) {
    if (candidate.mask == preset->mask && strcmp(candidate.name, preset->name) == 0 &&
        candidate.id != static_cast<CalibrationPreset>(0xFF)) {
      info = candidate;
      return true;
    }
  }
  return false;
}

bool MuxApp::applyCalibrationPreset(CalibrationPreset id) {
  CalibrationInfo info{};
  if (!resolveCalibrationPreset(id, info)) return false;
  return selectPreset(info.name);
}

bool MuxApp::dispatch(void* context, char* line, bool busReady, bool busBusy) {
  return static_cast<MuxApp*>(context)->process(line, busReady, busBusy);
}

void MuxApp::printHex(uint8_t value) {
  output_.print("0x");
  if (value < 0x10) output_.print('0');
  output_.print(value, HEX);
}

bool MuxApp::parseHex(const char* text, uint8_t& value) {
  if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) text += 2;
  if (!*text) return false;
  uint16_t parsed = 0;
  for (; *text; ++text) {
    uint8_t digit;
    if (*text >= '0' && *text <= '9') digit = *text - '0';
    else if (*text >= 'a' && *text <= 'f') digit = *text - 'a' + 10;
    else if (*text >= 'A' && *text <= 'F') digit = *text - 'A' + 10;
    else return false;
    parsed = parsed * 16 + digit;
    if (parsed > 0xFF) return false;
  }
  value = static_cast<uint8_t>(parsed);
  return true;
}

const MuxApp::Preset* MuxApp::presetByName(const char* name) const {
  for (const Preset& preset : presets_) {
    size_t index = 0;
    while (name[index] && preset.name[index] &&
           toupper(static_cast<unsigned char>(name[index])) == preset.name[index]) ++index;
    if (name[index] == '\0' && preset.name[index] == '\0') return &preset;
  }
  return nullptr;
}

const MuxApp::Preset* MuxApp::presetByMask(uint8_t mask) const {
  for (const Preset& preset : presets_) if (preset.mask == mask) return &preset;
  return nullptr;
}

void MuxApp::help() {
  output_.println("Comandos ADG715 (mascaras en hexadecimal):");
  output_.println("  adg715 help                  - Mostrar ayuda");
  output_.println("  adg715 ping                  - Sondear ACK en 0x48");
  output_.println("  adg715 read                  - Leer registro real y switches");
  output_.println("  adg715 status                - Estado leido, preset y errores");
  output_.println("  adg715 presets               - Listar mascaras autorizadas CN-0349");
  output_.println("  adg715 set <preset>          - Aplicar un preset autorizado");
  output_.println("  adg715 write <mask>          - Escribir solo una mascara autorizada");
  output_.println("  adg715 off                   - Aplicar ALL_OFF");
  output_.println("  adg715 switch <n> <on|off>   - Deshabilitado en el terminal CN-0349");
  output_.println("Para conmutar, AD5934 debe estar en power-down/standby y Control Low=0x08.");
}

void MuxApp::presets() {
  output_.println("Presets autorizados para EVAL-CN0349-PMDZ:");
  for (const Preset& preset : presets_) {
    output_.print("  ");
    output_.print(preset.name);
    output_.print(" = ");
    printHex(preset.mask);
    output_.print(" - ");
    output_.println(preset.description);
  }
}

void MuxApp::printSwitches(uint8_t state) {
  for (uint8_t channel = 1; channel <= 8; ++channel) {
    output_.print("S");
    output_.print(channel);
    output_.print("=");
    output_.print((state & (1U << (channel - 1))) ? "ON" : "OFF");
    output_.print(channel == 4 || channel == 8 ? "\n" : " ");
  }
}

void MuxApp::printState(uint8_t state) {
  output_.print("ADG715 [");
  printHex(mux_.address());
  output_.print("] = ");
  printHex(state);
  output_.println(" (registro leido por I2C)");
  printSwitches(state);
  const Preset* preset = presetByMask(state);
  output_.print("Preset: ");
  output_.println(preset ? preset->name : "ninguno (mascara no autorizada)");
}

void MuxApp::readState() {
  uint8_t state;
  if (!readMask(state)) { printLastError(); return; }
  printState(state);
}

void MuxApp::printLastError() {
  const ADG715::Error error = mux_.lastError();
  output_.print("Ultimo error ADG715: ");
  output_.print(ADG715::errorText(error));
  output_.print(" (codigo ");
  output_.print(mux_.getLastError());
  output_.print(", Wire ");
  output_.print(mux_.getLastWireError());
  output_.println(")");
}

void MuxApp::status() {
  output_.print("Direccion I2C ADG715: ");
  printHex(mux_.address());
  output_.println();
  uint8_t actual;
  if (readMask(actual)) printState(actual);
  else printLastError();
  output_.print("Ultimo estado solicitado: ");
  if (mux_.hasRequestedState()) printHex(mux_.lastRequestedState());
  else output_.print("ninguno");
  output_.println();
  output_.print("Ultimo estado confirmado por lectura de registro: ");
  if (mux_.hasConfirmedState()) printHex(mux_.lastConfirmedState());
  else output_.print("ninguno");
  output_.println();
  printLastError();
  output_.println("La lectura confirma el registro digital, no la ruta analogica.");
}

bool MuxApp::impedanceIsSafe() {
  uint16_t control;
  if (!impedance_.readRegister16(AD5934::CONTROL_HIGH, control)) {
    output_.print("Error al comprobar interlock AD5934: ");
    output_.println(AD5934::errorText(impedance_.lastError().code));
    return false;
  }
  const uint8_t mode = static_cast<uint8_t>((control >> 12) & 0x0F);
  if ((mode != 0x0A && mode != 0x0B) || (control & 0x00FF) != 0x08) {
    output_.println("Rechazado: AD5934 debe estar en power-down/standby y Control Low=0x08.");
    output_.print("Control leido: ");
    output_.println(control, HEX);
    return false;
  }
  return true;
}

bool MuxApp::setAuthorized(uint8_t mask, bool quiet) {
  const Preset* preset = presetByMask(mask);
  if (!preset) {
    output_.print("Rechazado: mascara no autorizada para CN-0349: ");
    printHex(mask);
    output_.println(". No se modifico ADG715.");
    return false;
  }
  if (!impedanceIsSafe()) return false;
  if (!mux_.writeState(mask)) {
    printLastError();
    output_.println("Estado solicitado y ultimo estado confirmado permanecen diferenciados.");
    return false;
  }
  observedMask_ = mask;
  hasObservedMask_ = true;
  if (quiet) return true;
  output_.print("Solicitado ");
  output_.print(preset->name);
  output_.print(" ");
  printHex(mask);
  output_.println("; confirmado por lectura del registro ADG715.");
  output_.println("Esto no verifica electricamente la ruta analogica.");
  return true;
}

bool MuxApp::process(char* line, bool busReady, bool busBusy) {
  if (strncmp(line, "adg715", 6) != 0 || (line[6] != '\0' && line[6] != ' ')) return false;
  char* args[5] = {};
  size_t count = 0;
  char* context = nullptr;
  for (char* token = strtok_r(line, " ", &context); token && count < 5;
       token = strtok_r(nullptr, " ", &context)) args[count++] = token;
  if (count == 1 || (count == 2 && strcmp(args[1], "help") == 0)) { help(); return true; }
  if (count == 2 && strcmp(args[1], "presets") == 0) { presets(); return true; }
  const bool needsBus = count >= 2 &&
      (strcmp(args[1], "ping") == 0 || strcmp(args[1], "read") == 0 ||
       strcmp(args[1], "status") == 0 || strcmp(args[1], "set") == 0 ||
       strcmp(args[1], "write") == 0 || strcmp(args[1], "off") == 0 ||
       strcmp(args[1], "switch") == 0);
  const bool noArgumentCommand = strcmp(args[1], "ping") == 0 ||
      strcmp(args[1], "read") == 0 || strcmp(args[1], "status") == 0 ||
      strcmp(args[1], "off") == 0;
  if (!needsBus || (noArgumentCommand && count != 2) ||
      ((strcmp(args[1], "set") == 0 || strcmp(args[1], "write") == 0) && count != 3) ||
      (strcmp(args[1], "switch") == 0 && count != 4)) {
    output_.println("Error: comando/argumentos ADG715 no validos. Use adg715 help.");
    return true;
  }
  if (!busReady) { output_.println("Error: bus I2C no inicializado."); return true; }
  if (busBusy) { output_.println("Error: diagnostico I2C en curso; espere a que termine."); return true; }

  const char* command = args[1];
  if (strcmp(command, "ping") == 0) {
    if (mux_.begin()) output_.println("ACK ADG715 en 0x48; registro sin cambios.");
    else printLastError();
  } else if (strcmp(command, "read") == 0) {
    readState();
  } else if (strcmp(command, "status") == 0) {
    status();
  } else if (strcmp(command, "set") == 0) {
    const Preset* preset = presetByName(args[2]);
    if (!preset) {
      output_.println("Error: preset inexistente; use adg715 presets. No se modifico ADG715.");
    } else {
      setAuthorized(preset->mask);
    }
  } else if (strcmp(command, "write") == 0) {
    uint8_t mask;
    if (!parseHex(args[2], mask)) output_.println("Error: mascara hexadecimal de 8 bits no valida.");
    else setAuthorized(mask);
  } else if (strcmp(command, "off") == 0) {
    setAuthorized(0x00);
  } else if (strcmp(command, "switch") == 0) {
    output_.println("Rechazado: control individual deshabilitado en la aplicacion CN-0349.");
  }
  return true;
}

// Reuse the existing RTD route; it is not an RCAL calibration preset.
bool MuxApp::pt100Info(CalibrationInfo& info) const {
  const Preset* preset = presetByName("RTD");
  if (!preset) return false;
  info = *preset;
  return true;
}
bool MuxApp::applyPt100(bool quiet) { return selectPreset("RTD", quiet); }
bool MuxApp::ecInfo(uint8_t mask, CalibrationInfo& info) const {
  if (mask != 0x81 && mask != 0x82) return false;
  const Preset* preset = presetByMask(mask);
  if (!preset) return false;
  info = *preset;
  return true;
}
bool MuxApp::applyEc(uint8_t mask) {
  CalibrationInfo info{};
  return ecInfo(mask, info) && selectPreset(info.name);
}
