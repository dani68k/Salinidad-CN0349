#pragma once

#include <Wire.h>
#include <stdint.h>

class ADG715 {
 public:
  static constexpr uint8_t DEFAULT_ADDRESS = 0x48;
  enum class Error : uint8_t {
    None, InvalidAddress, InvalidChannel, TxBuffer, AddressNack,
    DataNack, Timeout, Bus, ShortRead, VerifyMismatch
  };

  explicit ADG715(TwoWire& wire, uint8_t address = DEFAULT_ADDRESS);
  bool begin();  // Sondeo ACK; no escribe ni presupone un estado.
  bool readState(uint8_t& state);  // Lectura real de un byte del registro de control.
  bool writeState(uint8_t state);  // Escribe y verifica lectura del registro.
  bool setSwitch(uint8_t channel, bool enabled);  // Canales 1..8.
  bool getSwitch(uint8_t channel, bool& enabled); // Consulta una lectura real.

  uint8_t address() const { return address_; }
  uint8_t getLastError() const { return static_cast<uint8_t>(lastError_); }
  uint8_t getLastWireError() const { return lastWireError_; }
  Error lastError() const { return lastError_; }
  bool hasRequestedState() const { return hasRequestedState_; }
  uint8_t lastRequestedState() const { return lastRequestedState_; }
  bool hasConfirmedState() const { return hasConfirmedState_; }
  uint8_t lastConfirmedState() const { return lastConfirmedState_; }
  static const char* errorText(Error error);

 private:
  bool validAddress();
  bool setError(Error error, uint8_t wireCode = 0);
  TwoWire& wire_;
  uint8_t address_;
  Error lastError_ = Error::None;
  uint8_t lastWireError_ = 0;
  bool hasRequestedState_ = false;
  uint8_t lastRequestedState_ = 0;
  bool hasConfirmedState_ = false;
  uint8_t lastConfirmedState_ = 0;
};
