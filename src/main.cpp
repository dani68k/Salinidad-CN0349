#include <Arduino.h>
#include <Wire.h>
#include <AD5934.h>
#include <ADG715.h>

#include "config.h"
#include "terminal/Terminal.h"
#include "impedance/ImpedanceApp.h"
#include "mux/MuxApp.h"
#include "cn0349/CN0349Manager.h"

namespace {
Terminal terminal(Serial, Wire);
AD5934 ad5934(Wire, Config::AD5934_ADDRESS);
ADG715 adg715(Wire, Config::ADG715_ADDRESS);
MuxApp mux(adg715, ad5934, Serial);
ImpedanceApp impedance(ad5934, mux, Serial);
CN0349Manager cn0349Manager(impedance, Serial);
}

void setup() {
  Serial.begin(Config::SERIAL_BAUD);
  // El arranque no depende de que haya un monitor serie conectado.
  const bool i2cReady = Wire.begin(Config::I2C_SDA_PIN, Config::I2C_SCL_PIN,
                                   Config::I2C_FREQUENCY_HZ);
  Wire.setTimeOut(Config::I2C_TIMEOUT_MS);
  Serial.println();
  terminal.printInfo();
  terminal.begin(i2cReady);
  impedance.begin(terminal);
  mux.begin(terminal);
  cn0349Manager.begin(terminal, i2cReady);
  if (i2cReady) {
    impedance.reportConnection(ad5934.begin());
    mux.reportConnection(adg715.begin());
    Serial.println("Preparando sistema. Escriba help para ver los comandos.");
    cn0349Manager.prepareMeasurements();
  } else {
    Serial.println("Error: no se pudo inicializar I2C. Terminal preparado para diagnostico.");
  }
}

void loop() {
  terminal.update();
  cn0349Manager.update();
  mux.update();
  // Punto de integracion de futuras tareas cooperativas de adquisicion y BLE.
}
//cn0349 ec start
//cn0349 ec status
//cn0349 ec result
//cn0349 ec cal set CAL_HIGH_1K