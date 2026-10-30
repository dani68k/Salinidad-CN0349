#pragma once
/** @file ad5934_math.h
 * @brief Matemática DDS, DFT y validaciones de control del AD5934.
 */

#include <stdint.h>
#include <math.h>

/** @brief Funciones puras para códigos y registros del AD5934, sin I²C. */
namespace AD5934Math {
constexpr uint32_t MAX_OUTPUT_FREQUENCY_HZ = 100000; ///< Límite general de frecuencia de salida, en Hz.

/** @brief Calcula la palabra DDS truncada para una frecuencia.
 * @param frequencyHz Frecuencia solicitada, en Hz.
 * @param mclkHz Frecuencia de reloj configurada, en Hz.
 * @param[out] code Palabra DDS si la conversión es válida; no cambia si falla.
 * @return true para valores no nulos dentro de los límites de frecuencia, MCLK/16 y 24 bits.
 */
inline bool frequencyToCode(uint32_t frequencyHz, uint32_t mclkHz, uint32_t& code) {
  if (frequencyHz == 0 || mclkHz == 0 || frequencyHz > MAX_OUTPUT_FREQUENCY_HZ ||
      static_cast<uint64_t>(frequencyHz) * 16 > mclkHz) return false;
  const uint64_t scaled = (static_cast<uint64_t>(frequencyHz) << 27) / mclkHz;
  if (scaled > 0xFFFFFF) return false;
  code = static_cast<uint32_t>(scaled);
  return true;
}

/** @brief Interpreta una palabra de 16 bits como entero con signo.
 * @param raw Palabra DFT sin signo.
 * @return Valor equivalente en complemento a dos.
 */
inline int16_t decodeSigned16(uint16_t raw) {
  const int32_t signedValue = raw < 0x8000 ? raw : static_cast<int32_t>(raw) - 0x10000;
  return static_cast<int16_t>(signedValue);
}

/** @brief Calcula la magnitud de una muestra DFT compleja.
 * @param real Componente real con signo.
 * @param imaginary Componente imaginaria con signo.
 * @return Magnitud DFT en unidades de código.
 */
inline double magnitude(int16_t real, int16_t imaginary) {
  const double r = real;
  const double i = imaginary;
  return sqrt(r * r + i * i);
}

/** @brief Valida límites aritméticos de un barrido de frecuencias.
 * @param startHz Frecuencia inicial, en Hz; debe ser positiva.
 * @param stepHz Paso, en Hz; debe ser positivo si hay incrementos.
 * @param increments Número de incrementos.
 * @param maxPoints Máximo permitido de puntos, incluido el inicial.
 * @param maxFrequencyHz Límite de frecuencia final, en Hz.
 * @param[out] endHz Frecuencia final si la validación tiene éxito.
 * @return true si el barrido cumple los límites; false en caso contrario.
 * @note Esta validación matemática no habilita un barrido operativo del instrumento.
 */
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

/** @brief Comprueba que las palabras DDS y su acumulación caben en 24 bits.
 * @param startCode Código inicial.
 * @param stepCode Código de incremento.
 * @param increments Número de incrementos.
 * @return true si todos los valores caben; false en caso contrario.
 */
inline bool validSweepDdsCodes(uint32_t startCode, uint32_t stepCode, uint16_t increments) {
  return startCode <= 0xFFFFFF && stepCode <= 0xFFFFFF &&
      static_cast<uint64_t>(startCode) + static_cast<uint64_t>(stepCode) * increments <= 0xFFFFFF;
}

/** @brief Calcula la frecuencia DDS representada por un código.
 * @param code Palabra DDS.
 * @param mclkHz Frecuencia de reloj, en Hz.
 * @return Frecuencia efectiva calculada, en Hz; no se realiza validación de entradas.
 */
inline double frequencyFromCode(uint32_t code, uint32_t mclkHz) {
  return static_cast<double>(code) * mclkHz / 134217728.0;
}

/** @brief Calcula la fase cruda de una muestra DFT.
 * @param real Componente real con signo.
 * @param imaginary Componente imaginaria con signo.
 * @return Fase de atan2, en grados.
 */
inline double phaseDegrees(int16_t real, int16_t imaginary) {
  constexpr double RADIANS_TO_DEGREES = 57.295779513082320876;
  return atan2(static_cast<double>(imaginary), static_cast<double>(real)) * RADIANS_TO_DEGREES;
}

/** @brief Valida la palabra de settling y rechaza el multiplicador reservado.
 * @param value Valor del registro de ciclos de asentamiento.
 * @return true si los bits y multiplicador están admitidos; false en caso contrario.
 */
inline bool validSettlingWord(uint16_t value) {
  return value <= 0x07FF && (value & 0x0600) != 0x0400;
}

/** @brief Comprueba power-down o standby con Control Low igual a 0x08.
 * @param value Palabra completa de control AD5934.
 * @return true si el estado inactivo cumple la condición; false en caso contrario.
 */
inline bool validInactiveControl(uint16_t value) {
  const uint8_t mode = static_cast<uint8_t>((value >> 12) & 0x0F);
  return (mode == 0x0A || mode == 0x0B) && (value & 0x00FF) == 0x08;
}

/** @brief Devuelve Control High para power-down, Range 1 y PGA ×1. */
inline uint8_t controlHighRange1PgaX1() { return 0xA1; }

/** @brief Comprueba si un código de operación está implementado por el driver.
 * @param operation Código de operación de control.
 * @return true para Initialize, Start o Increment; false en otro caso.
 */
inline bool validControlOperation(uint8_t operation) {
  return operation == 0x01 || operation == 0x02 || operation == 0x03;
}

/** @brief Comprueba una transición de control soportada.
 * @param control Palabra actual de control.
 * @param operation Código de operación solicitado.
 * @return true si la transición cumple las condiciones de control; false en otro caso.
 * @note Para Increment, el driver comprueba además el bit de datos válidos D1.
 */
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

/** @brief Combina una operación con el nibble de configuración actual.
 * @param currentHigh Byte alto de control previo.
 * @param operation Código de operación para los cuatro bits superiores.
 * @return Nuevo byte alto; no valida la operación ni efectúa una escritura.
 */
inline uint8_t controlHighWithOperation(uint8_t currentHigh, uint8_t operation) {
  return static_cast<uint8_t>((operation << 4) | (currentHigh & 0x0F));
}
}  // namespace AD5934Math
