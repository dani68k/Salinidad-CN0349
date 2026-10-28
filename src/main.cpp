#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "terminal/Terminal.h"
#include "ble/BleApp.h"
namespace {
AD5934 ad5934(Wire, Config::AD5934_ADDRESS);
ADG715 adg715(Wire, Config::ADG715_ADDRESS);
MuxApp mux(adg715, ad5934);
ImpedanceApp acquisition(ad5934, mux);
CN0349Manager instrument(acquisition);
Terminal terminal(Serial, instrument);
BleApp ble(instrument);
}
void setup() {
  Serial.begin(Config::SERIAL_BAUD);
  const bool ready = Wire.begin(Config::I2C_SDA_PIN, Config::I2C_SCL_PIN, Config::I2C_FREQUENCY_HZ);
  Wire.setTimeOut(Config::I2C_TIMEOUT_MS);
  instrument.begin(ready);
  terminal.begin();
  ble.begin();
}
void loop() {
  instrument.update();
  terminal.update();
  ble.update();
}
