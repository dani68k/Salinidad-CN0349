#pragma once
/** @file Pt100.h
 * @brief Conversiones Pt100 puras según IEC 60751 / Callendar–Van Dusen.
 */

#include <math.h>
#include <stdint.h>

/** @brief Conversión entre resistencia Pt100 y temperatura nominal.
 * @details No realiza adquisición ni compensación de cables. La exactitud
 * numérica del modelo no valida la medición física del sensor.
 */
namespace Pt100 {
constexpr double R0 = 100.0; ///< Resistencia nominal a 0 °C, en Ω.
constexpr double A = 3.9083e-3; ///< Coeficiente A del modelo.
constexpr double B = -5.775e-7; ///< Coeficiente B del modelo.
constexpr double C = -4.183e-12; ///< Coeficiente C para temperaturas negativas.
/** @brief Resultado de la validación y conversión de resistencia. */
enum class Error : uint8_t { NotAttempted, None, NonFinite, NonPositive, OutOfRange };
/** @brief Temperatura y estado devueltos por convert(). */
struct Result {
  bool valid = false; ///< Indica si la temperatura es válida.
  double celsius = NAN; ///< Temperatura en °C; NaN si la conversión es inválida.
  Error error = Error::NotAttempted; ///< Estado o motivo del error.
};
/** @brief Evalúa la ecuación directa resistencia-temperatura.
 * @param t Temperatura en °C.
 * @return Resistencia nominal Pt100 en Ω.
 * @note No valida la entrada; el intervalo nominal del modelo es −200 a 850 °C.
 */
inline double resistance(double t) {
  return R0 * (1.0 + A*t + B*t*t + (t < 0 ? C*(t-100.0)*t*t*t : 0.0));
}
/** @brief Convierte resistencia Pt100 a temperatura nominal.
 * @param ohms Resistencia en Ω, finita, positiva y dentro del intervalo nominal.
 * @return Temperatura en °C y estado; inválido para valores no finitos,
 * no positivos o fuera de las resistencias correspondientes a −200 y 850 °C.
 */
inline Result convert(double ohms) {
  Result r;
  if (!isfinite(ohms)) { r.error = Error::NonFinite; return r; }
  if (ohms <= 0) { r.error = Error::NonPositive; return r; }
  if (ohms < resistance(-200) || ohms > resistance(850)) {
    r.error = Error::OutOfRange; return r;
  }
  if (ohms >= R0) {
    const double delta = ohms/R0 - 1.0;
    // Rationalized quadratic root avoids cancellation near zero Celsius.
    r.celsius = 2.0*delta / (A + sqrt(A*A + 4.0*B*delta));
  } else {
    double low = -200.0, high = 0.0;
    for (unsigned i = 0; i < 48; ++i) {
      const double mid = (low+high)*0.5;
      if (resistance(mid) < ohms) low = mid; else high = mid;
    }
    r.celsius = (low+high)*0.5;
  }
  r.valid = true;
  r.error = Error::None;
  return r;
}
/** @brief Obtiene el nombre textual de un código de error.
 * @param e Código a consultar.
 * @return Cadena estática o UNKNOWN si el código no se reconoce.
 */
inline const char* errorName(Error e) {
  switch (e) {
    case Error::NotAttempted: return "NOT_ATTEMPTED";
    case Error::None: return "NONE";
    case Error::NonFinite: return "NON_FINITE";
    case Error::NonPositive: return "NON_POSITIVE";
    case Error::OutOfRange: return "OUT_OF_RANGE";
  }
  return "UNKNOWN";
}
}  // namespace Pt100
