#pragma once

#include <stdint.h>
#include <math.h>

namespace AD5934Math {
constexpr uint32_t MAX_OUTPUT_FREQUENCY_HZ = 100000;

inline bool frequencyToCode(uint32_t frequencyHz, uint32_t mclkHz, uint32_t& code) {
  if (frequencyHz == 0 || mclkHz == 0 || frequencyHz > MAX_OUTPUT_FREQUENCY_HZ ||
      static_cast<uint64_t>(frequencyHz) * 16 > mclkHz) return false;
  const uint64_t scaled = (static_cast<uint64_t>(frequencyHz) << 27) / mclkHz;
  if (scaled > 0xFFFFFF) return false;
  code = static_cast<uint32_t>(scaled);
  return true;
}

inline int16_t decodeSigned16(uint16_t raw) {
  const int32_t signedValue = raw < 0x8000 ? raw : static_cast<int32_t>(raw) - 0x10000;
  return static_cast<int16_t>(signedValue);
}

inline double magnitude(int16_t real, int16_t imaginary) {
  const double r = real;
  const double i = imaginary;
  return sqrt(r * r + i * i);
}

inline bool validateSweep(uint32_t startHz, uint32_t stepHz, uint16_t increments,
                          uint16_t maxPoints, uint32_t maxFrequencyHz,
                          uint32_t& endHz) {
  if (startHz == 0 || maxPoints == 0 || static_cast<uint32_t>(increments) + 1 > maxPoints ||
      (increments > 0 && stepHz == 0)) return false;
  const uint64_t end = static_cast<uint64_t>(startHz) +
      static_cast<uint64_t>(stepHz) * increments;
  if (end > UINT32_MAX || end > maxFrequencyHz) return false;
  endHz = static_cast<uint32_t>(end);
  return true;
}

inline bool validSweepDdsCodes(uint32_t startCode, uint32_t stepCode, uint16_t increments) {
  return startCode <= 0xFFFFFF && stepCode <= 0xFFFFFF &&
      static_cast<uint64_t>(startCode) + static_cast<uint64_t>(stepCode) * increments <= 0xFFFFFF;
}

inline double frequencyFromCode(uint32_t code, uint32_t mclkHz) {
  return static_cast<double>(code) * mclkHz / 134217728.0;
}

inline double phaseDegrees(int16_t real, int16_t imaginary) {
  constexpr double RADIANS_TO_DEGREES = 57.295779513082320876;
  return atan2(static_cast<double>(imaginary), static_cast<double>(real)) * RADIANS_TO_DEGREES;
}

inline bool validSettlingWord(uint16_t value) {
  return value <= 0x07FF && (value & 0x0600) != 0x0400;
}

inline bool validInactiveControl(uint16_t value) {
  const uint8_t mode = static_cast<uint8_t>((value >> 12) & 0x0F);
  return (mode == 0x0A || mode == 0x0B) && (value & 0x00FF) == 0x08;
}

inline uint8_t controlHighRange1PgaX1() { return 0xA1; }

inline bool validControlOperation(uint8_t operation) {
  return operation == 0x01 || operation == 0x02 || operation == 0x03;
}

inline bool validControlTransition(uint16_t control, uint8_t operation) {
  if ((control & 0x00FF) != 0x08) return false;
  const uint8_t mode = static_cast<uint8_t>((control >> 12) & 0x0F);
  if (operation == 0x01) return mode == 0x0A || mode == 0x0B;
  if (operation == 0x02) return mode == 0x01;
  // Increment se valida por D1 en el driver; los bits de comando no describen
  // por sí solos si el resultado actual ya fue consumido.
  if (operation == 0x03) return true;
  return false;
}

inline uint8_t controlHighWithOperation(uint8_t currentHigh, uint8_t operation) {
  return static_cast<uint8_t>((operation << 4) | (currentHigh & 0x0F));
}
}  // namespace AD5934Math
