#pragma once

#include <Wire.h>
#include <stdint.h>
#include "../../include/ad5934_math.h"

// AD5934 Rev. E, tablas 7, 11-13 y figuras 26, 27, 29.
class AD5934 {
 public:
  static constexpr uint8_t DEFAULT_ADDRESS = 0x0D;
  enum Register : uint8_t {
    CONTROL_HIGH = 0x80, CONTROL_LOW = 0x81,
    START_FREQUENCY_MSB = 0x82, START_FREQUENCY_MID = 0x83, START_FREQUENCY_LSB = 0x84,
    FREQUENCY_INCREMENT_MSB = 0x85, FREQUENCY_INCREMENT_MID = 0x86, FREQUENCY_INCREMENT_LSB = 0x87,
    NUMBER_INCREMENTS_MSB = 0x88, NUMBER_INCREMENTS_LSB = 0x89,
    SETTLING_CYCLES_MSB = 0x8A, SETTLING_CYCLES_LSB = 0x8B,
    STATUS = 0x8F, REAL_MSB = 0x94, REAL_LSB = 0x95,
    IMAGINARY_MSB = 0x96, IMAGINARY_LSB = 0x97
  };
  static constexpr uint8_t STATUS_DATA_VALID = 0x02;
  static constexpr uint8_t STATUS_SWEEP_COMPLETE = 0x04;
  static constexpr uint32_t MAX_OUTPUT_FREQUENCY_HZ = AD5934Math::MAX_OUTPUT_FREQUENCY_HZ;
  enum class ControlCommand : uint8_t { InitializeStartFrequency = 0x1,
                                         StartFrequencySweep = 0x2,
                                         IncrementFrequency = 0x3 };
  struct RegisterGroup {
    uint8_t first;
    uint8_t size;
    const char* name;
    bool writable;
  };
  enum class ErrorCode : uint8_t {
    None, InvalidAddress, InvalidRegister, InvalidSpan, ReadOnly,
    ControlProtected, InvalidValue, TxBuffer, AddressNack, DataNack,
    Timeout, Bus, ShortRead
  };
  enum class Phase : uint8_t { Validation, Probe, Pointer, Receive, Write };
  struct Error {
    ErrorCode code = ErrorCode::None;
    Phase phase = Phase::Validation;
    uint8_t reg = 0;
    uint8_t wireCode = 0;  // Solo disponible para endTransmission().
    uint8_t completedBytes = 0;
  };

  explicit AD5934(TwoWire& wire, uint8_t address = DEFAULT_ADDRESS);
  bool begin();  // Sondeo, sin inicializar el bus ni escribir registros.
  bool readRegister8(uint8_t reg, uint8_t& value);
  bool writeRegister8(uint8_t reg, uint8_t value);
  bool readRegister16(uint8_t reg, uint16_t& value);
  bool writeRegister16(uint8_t reg, uint16_t value);
  bool readRegister24(uint8_t reg, uint32_t& value);
  bool writeRegister24(uint8_t reg, uint32_t value);
  bool readStatus(uint8_t& value);
  bool configureSweep(uint32_t startFrequencyCode, uint16_t settlingWord);
  bool setStartFrequencyCode(uint32_t code);
  bool setFrequencyIncrementCode(uint32_t code);
  bool setNumberOfIncrements(uint16_t count);
  bool setSettlingCycles(uint16_t cycles);
  bool configureRange1PgaX1();
  bool wakeToStandby(uint16_t& previousControl, uint16_t& verifiedControl);
  bool sendCommand(ControlCommand command);
  bool readReal(int16_t& value);
  bool readImaginary(int16_t& value);
  static bool frequencyToCode(uint32_t frequencyHz, uint32_t mclkHz, uint32_t& code);
  static double magnitude(int16_t real, int16_t imaginary);
  // Transición explícita de seguridad; conserva rango/PGA y normaliza D3 reservado.
  bool enterStandby(uint16_t& previousControl, uint16_t& verifiedControl);
  bool validateWrite(uint8_t reg, uint8_t width, uint32_t value);
  bool validateRead(uint8_t reg, uint8_t width);
  const Error& lastError() const { return error_; }
  uint8_t address() const { return address_; }
  static const RegisterGroup* groupFor(uint8_t reg);
  static const RegisterGroup* groupAt(size_t index);
  static const char* errorText(ErrorCode code);
  static const char* phaseText(Phase phase);
  static uint8_t meaningfulMask(uint8_t reg);

 private:
  static constexpr uint8_t ADDRESS_POINTER = 0xB0;
  bool validateSpan(uint8_t reg, uint8_t width, bool writing);
  bool readBytes(uint8_t reg, uint8_t width, uint32_t& value);
  bool writeBytes(uint8_t reg, uint8_t width, uint32_t value);
  bool transmit(const uint8_t* bytes, size_t length, uint8_t reg, Phase phase);
  bool fail(ErrorCode code, uint8_t reg, Phase phase, uint8_t wireCode = 0);
  void clearError();
  TwoWire& wire_;
  uint8_t address_;
  Error error_;
};
