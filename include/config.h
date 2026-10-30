#pragma once
/** @file config.h
 * @brief Constantes de compilación compartidas por el firmware.
 */

#include <stddef.h>
#include <stdint.h>

/** @brief Parámetros fijos de hardware, identidad y terminal.
 * @note El Setup mutable y persistido se gestiona en CN0349Manager.
 */
namespace Config {
constexpr char FIRMWARE_NAME[] = "ATLANTES CN-0349"; ///< Nombre del firmware.
constexpr char FIRMWARE_VERSION[] = "1.0.0"; ///< Identificador de versión compilado.
constexpr char MILESTONE[] = "3B"; ///< Identificador de hito compilado.
constexpr uint32_t SERIAL_BAUD = 115200; ///< Velocidad serie, en baudios.
constexpr int I2C_SDA_PIN = 4; ///< GPIO utilizado para SDA.
constexpr int I2C_SCL_PIN = 5; ///< GPIO utilizado para SCL.
constexpr uint32_t I2C_FREQUENCY_HZ = 100000; ///< Frecuencia del bus I²C, en Hz.
constexpr uint16_t I2C_TIMEOUT_MS = 25; ///< Timeout de Wire, en ms.
constexpr uint8_t AD5934_ADDRESS = 0x0D; ///< Dirección I²C del AD5934.
constexpr uint8_t ADG715_ADDRESS = 0x48; ///< Dirección I²C del ADG715.
constexpr float EC_NOMINAL_CELL_CONSTANT_PER_CM = 1.0f; ///< Constante nominal de celda, en cm⁻¹.
constexpr float EC_NOMINAL_R2_OHMS = 100.0f; ///< Resistencia nominal R2, en Ω.
constexpr uint8_t I2C_FIRST_ADDRESS = 0x08; ///< Primera dirección del rango de sondeo configurado.
constexpr uint8_t I2C_LAST_ADDRESS = 0x77; ///< Última dirección del rango de sondeo configurado.
constexpr size_t TERMINAL_BUFFER_SIZE = 96; ///< Buffer de entrada: 95 caracteres y terminador NUL.
constexpr size_t TERMINAL_BYTES_PER_UPDATE = 32; ///< Máximo de bytes de entrada procesados por update.
static_assert(TERMINAL_BUFFER_SIZE >= 2, "El buffer necesita espacio para texto y NUL");
static_assert(TERMINAL_BYTES_PER_UPDATE > 0, "El terminal debe poder consumir entrada");
}  // namespace Config
