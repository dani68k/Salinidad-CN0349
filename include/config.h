#pragma once

#include <stddef.h>
#include <stdint.h>

namespace Config {
constexpr char FIRMWARE_NAME[] = "ATLANTES CN-0349";
constexpr char FIRMWARE_VERSION[] = "1.0.0";
constexpr char MILESTONE[] = "3A";
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr int I2C_SDA_PIN = 4;
constexpr int I2C_SCL_PIN = 5;
constexpr uint32_t I2C_FREQUENCY_HZ = 100000;
constexpr uint16_t I2C_TIMEOUT_MS = 25;
constexpr uint8_t AD5934_ADDRESS = 0x0D;
constexpr uint8_t ADG715_ADDRESS = 0x48;
constexpr float EC_NOMINAL_CELL_CONSTANT_PER_CM = 1.0f;
constexpr float EC_NOMINAL_R2_OHMS = 100.0f;
constexpr uint8_t I2C_FIRST_ADDRESS = 0x08;
constexpr uint8_t I2C_LAST_ADDRESS = 0x77;
constexpr size_t TERMINAL_BUFFER_SIZE = 96;  // 95 caracteres y terminador NUL.
constexpr size_t TERMINAL_BYTES_PER_UPDATE = 32;
static_assert(TERMINAL_BUFFER_SIZE >= 2, "El buffer necesita espacio para texto y NUL");
static_assert(TERMINAL_BYTES_PER_UPDATE > 0, "El terminal debe poder consumir entrada");
}  // namespace Config
