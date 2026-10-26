#include "ADG715.h"

ADG715::ADG715(TwoWire& wire, uint8_t address) : wire_(wire), address_(address) {}

bool ADG715::setError(Error error, uint8_t wireCode) {
  lastError_ = error;
  lastWireError_ = wireCode;
  return false;
}

bool ADG715::validAddress() {
  if (address_ < 0x08 || address_ > 0x77)
    return setError(Error::InvalidAddress);
  return true;
}

bool ADG715::begin() {
  if (!validAddress()) return false;
  wire_.beginTransmission(address_);
  const uint8_t code = wire_.endTransmission(true);
  if (code == 0) return true;
  const Error error = code == 1 ? Error::TxBuffer :
                      code == 2 ? Error::AddressNack :
                      code == 3 ? Error::DataNack :
                      code == 5 ? Error::Timeout : Error::Bus;
  return setError(error, code);
}

bool ADG715::readState(uint8_t& state) {
  if (!validAddress()) return false;
  const size_t received = wire_.requestFrom(address_, static_cast<size_t>(1), true);
  if (received != 1 || wire_.available() != 1)
    return setError(Error::ShortRead);
  const int value = wire_.read();
  if (value < 0) return setError(Error::ShortRead);
  state = static_cast<uint8_t>(value);
  return true;
}

bool ADG715::writeState(uint8_t state) {
  if (!validAddress()) return false;
  hasRequestedState_ = true;
  lastRequestedState_ = state;

  wire_.beginTransmission(address_);
  const size_t queued = wire_.write(state);
  const uint8_t code = wire_.endTransmission(true);  // Siempre cierra la transacción Wire.
  if (queued != 1) return setError(Error::TxBuffer, code);
  if (code != 0) {
    const Error error = code == 1 ? Error::TxBuffer :
                        code == 2 ? Error::AddressNack :
                        code == 3 ? Error::DataNack :
                        code == 5 ? Error::Timeout : Error::Bus;
    return setError(error, code);
  }

  uint8_t actual;
  if (!readState(actual)) return false;
  if (actual != state) return setError(Error::VerifyMismatch);
  hasConfirmedState_ = true;
  lastConfirmedState_ = actual;
  return true;
}

bool ADG715::setSwitch(uint8_t channel, bool enabled) {
  if (channel < 1 || channel > 8) return setError(Error::InvalidChannel);
  uint8_t actual;
  if (!readState(actual)) return false;
  const uint8_t mask = static_cast<uint8_t>(1U << (channel - 1));
  const uint8_t requested = enabled ? static_cast<uint8_t>(actual | mask)
                                    : static_cast<uint8_t>(actual & ~mask);
  return writeState(requested);
}

bool ADG715::getSwitch(uint8_t channel, bool& enabled) {
  if (channel < 1 || channel > 8) return setError(Error::InvalidChannel);
  uint8_t actual;
  if (!readState(actual)) return false;
  enabled = (actual & static_cast<uint8_t>(1U << (channel - 1))) != 0;
  return true;
}

const char* ADG715::errorText(Error error) {
  switch (error) {
    case Error::None: return "sin error previo";
    case Error::InvalidAddress: return "direccion I2C no valida";
    case Error::InvalidChannel: return "canal fuera de rango (1..8)";
    case Error::TxBuffer: return "buffer de transmision insuficiente";
    case Error::AddressNack: return "NACK de direccion";
    case Error::DataNack: return "NACK de datos";
    case Error::Timeout: return "timeout de transmision";
    case Error::Bus: return "error I2C";
    case Error::ShortRead: return "lectura incompleta";
    case Error::VerifyMismatch: return "el registro leido no coincide con el solicitado";
  }
  return "error desconocido";
}
