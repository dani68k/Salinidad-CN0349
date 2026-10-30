#pragma once
/** @file ImpedanceMath.h
 * @brief Comparación de frecuencias DDS y cálculo de impedancia aparente.
 */
#include <math.h>
#include <float.h>
/** @brief Funciones puras de apoyo al controlador CN0349. */
namespace CN0349Math {
/** @brief Compara dos frecuencias efectivas con tolerancia de redondeo float.
 * @param a Primera frecuencia, en Hz.
 * @param b Segunda frecuencia, en Hz.
 * @return true si ambas son finitas, positivas y equivalentes dentro de la tolerancia numérica.
 * @note La tolerancia de 5 × FLT_EPSILON no es una tolerancia metrológica.
 */
inline bool sameEffectiveFrequency(float a, float b) {
  return isfinite(a) && isfinite(b) && a > 0 && b > 0 &&
      fabsf(a - b) <= 5.0f * FLT_EPSILON * fmaxf(a, b);
}
/** @brief Calcula impedancia aparente y fase corregida instrumentalmente.
 * @param magnitude Magnitud DFT positiva y finita.
 * @param gain Factor de calibración legacy positivo y finito.
 * @param measuredPhase Fase DFT medida, en grados.
 * @param calibrationPhase Fase de calibración, en grados.
 * @param[out] ohms Impedancia aparente, en Ω, si el cálculo tiene éxito.
 * @param[out] phase Diferencia de fase normalizada a [−180, 180], en grados.
 * @return true si entradas y resultado son válidos; false en caso contrario.
 * @note La fase corregida es instrumental y no representa una fase física validada.
 */
inline bool apparentImpedance(float magnitude, float gain, float measuredPhase,
                             float calibrationPhase, float& ohms, float& phase) {
  if (!isfinite(magnitude) || magnitude <= 0 || !isfinite(gain) || gain <= 0 ||
      !isfinite(measuredPhase) || !isfinite(calibrationPhase)) return false;
  const float product = gain * magnitude;
  if (!isfinite(product) || product <= 0) return false;
  const float value = 1.0f / product;
  const double delta = static_cast<double>(measuredPhase) - calibrationPhase;
  double normalized = fmod(delta, 360.0);
  if (normalized > 180.0) normalized -= 360.0;
  if (normalized < -180.0) normalized += 360.0;
  if (!isfinite(value) || value <= 0) return false;
  ohms = value;
  phase = static_cast<float>(normalized);
  return true;
}
}  // namespace CN0349Math
