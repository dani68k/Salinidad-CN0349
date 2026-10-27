#include "Terminal.h"

#include <ctype.h>
#include <string.h>

const Terminal::Command Terminal::commands_[] = {
    {"help", "Mostrar los comandos disponibles", &Terminal::printHelp},
    {"info", "Mostrar firmware y configuracion I2C", &Terminal::printInfo},
    {"i2c scan", "Sondear direcciones 0x08 a 0x77 sin escribir registros", &Terminal::startScan},
    {"i2c status", "Mostrar estado I2C y ultimo diagnostico", &Terminal::printStatus},
    {"clear", "Limpiar pantalla mediante ANSI", &Terminal::clear},
};

Terminal::Terminal(Stream& serial, TwoWire& wire) : serial_(serial), wire_(wire) {}

bool Terminal::attachModule(void* context, ModuleHandler handler, ModuleBusy busy, const char* help) {
  if (moduleCount_ >= MAX_MODULES) {
    serial_.println("Error: se alcanzo el maximo de modulos del terminal.");
    return false;
  }
  modules_[moduleCount_++] = Module{context, handler, busy, help};
  return true;
}

bool Terminal::moduleBusy() const {
  for (size_t index = 0; index < moduleCount_; ++index) {
    if (modules_[index].busy && modules_[index].busy(modules_[index].context)) return true;
  }
  return false;
}

void Terminal::begin(bool i2cReady) {
  i2cReady_ = i2cReady;
  inputLength_ = 0;
  input_[0] = '\0';
  discardLine_ = false;
  previousWasCr_ = false;
  scanActive_ = false;
  hasCompletedScan_ = false;
  currentScan_ = ScanResult{};
  lastScan_ = ScanResult{};
}

void Terminal::update() {
  // Presupuesto fijo para que un flujo continuo de entrada no monopolice loop().
  for (size_t count = 0; count < Config::TERMINAL_BYTES_PER_UPDATE; ++count) {
    if (serial_.available() <= 0) {
      break;
    }
    const int value = serial_.read();
    if (value < 0) {
      break;
    }
    consume(static_cast<char>(value));
  }
  if (scanActive_) {
    advanceScan();
  }
}

void Terminal::consume(char character) {
  if (character == '\n' && previousWasCr_) {
    previousWasCr_ = false;
    return;
  }
  previousWasCr_ = (character == '\r');
  if (character == '\r' || character == '\n') {
    if (!discardLine_) {
      input_[inputLength_] = '\0';
      execute();
    }
    inputLength_ = 0;
    discardLine_ = false;
    return;
  }
  if (discardLine_) {
    return;
  }
  if (character == '\b' || character == 0x7f) {
    if (inputLength_ > 0) {
      --inputLength_;
    }
    return;
  }
  const unsigned char byte = static_cast<unsigned char>(character);
  if ((byte < 0x20 && character != '\t') || byte >= 0x7f) {
    discardLine_ = true;
    serial_.println("Error: caracter no valido. Linea descartada; use comandos ASCII.");
    return;
  }
  if (inputLength_ >= sizeof(input_) - 1) {
    discardLine_ = true;
    serial_.print("Error: comando demasiado largo (maximo ");
    serial_.print(static_cast<unsigned int>(sizeof(input_) - 1));
    serial_.println(" caracteres). Linea descartada hasta fin de linea.");
    return;
  }
  input_[inputLength_++] = character;
}

void Terminal::execute() {
  // Normaliza en el mismo buffer: minusculas, sin espacios extremos o repetidos.
  size_t output = 0;
  bool pendingSpace = false;
  for (size_t index = 0; index < inputLength_; ++index) {
    const unsigned char character = static_cast<unsigned char>(input_[index]);
    if (character == ' ' || character == '\t') {
      pendingSpace = output > 0;
      continue;
    }
    if (pendingSpace) {
      input_[output++] = ' ';
      pendingSpace = false;
    }
    input_[output++] = static_cast<char>(tolower(character));
  }
  input_[output] = '\0';
  if (output == 0) {
    return;
  }
  for (const Command& command : commands_) {
    if (strcmp(input_, command.name) == 0) {
      (this->*command.handler)();
      return;
    }
  }
  const bool busBusy = scanActive_ || moduleBusy();
  for (size_t index = 0; index < moduleCount_; ++index) {
    const Module& module = modules_[index];
    if (module.handler && module.handler(module.context, input_, i2cReady_, busBusy)) return;
  }
  serial_.print("Error: comando desconocido: ");
  serial_.println(input_);
  serial_.println("Escriba help para ver los comandos disponibles.");
}

void Terminal::printHelp() {
  serial_.println("Comandos disponibles:");
  for (const Command& command : commands_) {
    serial_.print("  ");
    serial_.print(command.name);
    serial_.print(" - ");
    serial_.println(command.description);
  }
  for (size_t index = 0; index < moduleCount_; ++index) {
    if (modules_[index].help) serial_.println(modules_[index].help);
  }
}

void Terminal::printI2cConfig() {
  serial_.print("I2C SDA: GPIO");
  serial_.println(Config::I2C_SDA_PIN);
  serial_.print("I2C SCL: GPIO");
  serial_.println(Config::I2C_SCL_PIN);
  serial_.print("I2C frecuencia configurada: ");
  serial_.print(Config::I2C_FREQUENCY_HZ);
  serial_.println(" Hz");
  serial_.print("I2C timeout configurado: ");
  serial_.print(Config::I2C_TIMEOUT_MS);
  serial_.println(" ms");
}

void Terminal::printInfo() {
  serial_.println(Config::FIRMWARE_NAME);
  serial_.print("Firmware: ");
  serial_.println(Config::FIRMWARE_VERSION);
  serial_.print("Hito: ");
  serial_.println(Config::MILESTONE);
  printI2cConfig();
}

void Terminal::printHex(uint8_t value) {
  serial_.print("0x");
  if (value < 0x10) {
    serial_.print('0');
  }
  serial_.print(value, HEX);
}

const char* Terminal::resultDescription(uint8_t code) {
  switch (code) {
    case 0: return "ACK";
    case 1: return "datos demasiado largos";
    case 2: return "NACK de direccion (sin respuesta)";
    case 3: return "NACK de datos";
    case 4: return "error de comunicacion";
    case 5: return "timeout";
    default: return "error no reconocido";
  }
}

void Terminal::startScan() {
  if (moduleBusy()) {
    serial_.println("Error: diagnostico de modulo en curso; espere a que termine.");
    return;
  }
  if (!i2cReady_) {
    serial_.println("Error: I2C no inicializado. Revise la configuracion y reinicie.");
    return;
  }
  if (scanActive_) {
    serial_.println("Error: ya hay un escaneo I2C en curso.");
    return;
  }
  currentScan_ = ScanResult{};
  nextAddress_ = Config::I2C_FIRST_ADDRESS;
  scanActive_ = true;
  serial_.println("I2C Scan");
  serial_.println("---------");
}

void Terminal::advanceScan() {
  const uint8_t address = nextAddress_;
  wire_.beginTransmission(address);
  // Solo direccion + STOP: no se encola ningun byte ni direccion de registro.
  const uint8_t code = wire_.endTransmission();
  ++currentScan_.probed;
  currentScan_.lastAddress = address;
  currentScan_.lastCode = code;
  if (code == 0) {
    ++currentScan_.found;
    serial_.print("Device found: ");
    printHex(address);
    serial_.println();
  } else if (code == 2) {
    ++currentScan_.nack;
  } else {
    ++currentScan_.errors;
    currentScan_.lastErrorAddress = address;
    currentScan_.lastErrorCode = code;
    serial_.print("I2C error at ");
    printHex(address);
    serial_.print(": ");
    serial_.print(resultDescription(code));
    serial_.print(" (Wire code ");
    serial_.print(code);
    serial_.println(')');
  }
  if (address == Config::I2C_LAST_ADDRESS) {
    scanActive_ = false;
    lastScan_ = currentScan_;
    hasCompletedScan_ = true;
    serial_.print("Scan completed: ");
    serial_.print(lastScan_.found);
    serial_.println(" device(s)");
    printScanResult(lastScan_);
  } else {
    ++nextAddress_;
  }
}

void Terminal::printScanResult(const ScanResult& result) {
  serial_.print("Direcciones sondeadas: ");
  serial_.print(result.probed);
  serial_.print("; ACK: ");
  serial_.print(result.found);
  serial_.print("; NACK de direccion: ");
  serial_.print(result.nack);
  serial_.print("; errores: ");
  serial_.println(result.errors);
  if (result.probed > 0) {
    serial_.print("Ultima transaccion: ");
    printHex(result.lastAddress);
    serial_.print("; ");
    serial_.println(resultDescription(result.lastCode));
  }
  if (result.errors > 0) {
    serial_.print("Ultimo error: ");
    printHex(result.lastErrorAddress);
    serial_.print("; Wire code ");
    serial_.print(result.lastErrorCode);
    serial_.print("; ");
    serial_.println(resultDescription(result.lastErrorCode));
  }
}

void Terminal::printStatus() {
  printI2cConfig();
  serial_.print("Inicializacion I2C: ");
  serial_.println(i2cReady_ ? "correcta (no garantiza respuesta de dispositivos)" : "fallida");
  if (scanActive_) {
    serial_.println("Escaneo en curso:");
    printScanResult(currentScan_);
  }
  if (hasCompletedScan_) {
    serial_.println("Ultimo diagnostico completado:");
    printScanResult(lastScan_);
  } else {
    serial_.println("Ultimo diagnostico completado: ninguno.");
  }
}

void Terminal::clear() {
  serial_.print("\x1b[2J\x1b[H");
}
