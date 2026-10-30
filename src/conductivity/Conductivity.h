#pragma once
/** @file Conductivity.h
 * @brief Calibración AD y conversiones puras de conductividad EC.
 */

#include <math.h>
#include <stdint.h>

/** @brief Modelos de conversión EC, sin adquisición ni acceso al hardware.
 * @details Los valores se estiman con la configuración y calibración dadas;
 * su validez numérica no demuestra exactitud metrológica de la celda.
 */
namespace Conductivity {

/** @brief Modelo de EC: RAW, SERIES experimental o AnalogDevices de dos puntos. */
enum class Model { Raw, SeriesR2Experimental, AnalogDevices };
/** @brief Estado o motivo de fallo de una conversión EC. */
enum class Error { None, NotRun, InvalidModel, InvalidImpedance, InvalidCellConstant, InvalidR2,
  NonPositiveCellResistance, Overflow, CalibrationInvalid, CalibrationMismatch,
  InvalidMagnitude, NonPositiveAdmittance, InvalidDenominator };

/** @brief Estado o motivo de fallo de la calibración AD de dos puntos. */
enum class AdCalibrationError { None, NotRun, InvalidPoint, NonIncreasingCodes, SpanTooSmall,
  InvalidGainFactor, InvalidOffset, ConfigurationMismatch };

/** @brief Calibración AD de dos puntos y metadatos de compatibilidad.
 * @note calibrateAnalogDevices() inicializa la frecuencia efectiva con la
 * solicitada; el controlador puede completarla con la frecuencia DDS efectiva.
 */
struct AdCalibration {
  bool valid = false; ///< Indica que la calibración es utilizable.
  AdCalibrationError error = AdCalibrationError::NotRun; ///< Estado o motivo de fallo.
  double nLow = 0; ///< Magnitud DFT de la referencia de menor admitancia.
  double nHigh = 0; ///< Magnitud DFT de la referencia de mayor admitancia.
  double yLowSiemens = 0; ///< Admitancia de la referencia inferior, en S.
  double yHighSiemens = 0; ///< Admitancia de la referencia superior, en S.
  double gainFactorSiemensPerCode = 0; ///< Pendiente GF, en S/código DFT.
  double systemOffsetCode = 0; ///< Offset NOS, en códigos DFT.
  double rfbOhms = 0; ///< Realimentación nominal, en Ω.
  double frequencyHz = 0; ///< Frecuencia solicitada, en Hz.
  double effectiveFrequencyHz = 0; ///< Frecuencia DDS efectiva, en Hz.
  double mclkHz = 0; ///< MCLK configurado, en Hz; completado por el controlador.
  uint16_t settlingCycles = 0; ///< Ciclos de asentamiento; completados por el controlador.
  uint8_t excitationRange = 0; ///< Rango de excitación configurado.
  uint8_t pgaGain = 0; ///< Ganancia PGA configurada.
  uint32_t calibrationId = 0; ///< Identificador de la calibración.
};

/** @brief Conductividad estimada, estado y valores intermedios de conversión.
 * @note Los campos específicos de un modelo no describen necesariamente los
 * valores de los demás modelos. La compensación EC25 se realiza fuera de este módulo.
 */
struct Result {
  bool valid = false; ///< Indica conversión válida.
  Error error = Error::NotRun; ///< Estado o motivo de fallo.
  Model model = Model::Raw; ///< Modelo de conversión.
  float apparentImpedanceOhms = 0; ///< Impedancia aparente de entrada, en Ω, para RAW/SERIES.
  float r2Ohms = 0; ///< Resistencia R2 utilizada, en Ω.
  float cellResistanceOhms = 0; ///< Resistencia de celda estimada, en Ω.
  float nominalKPerCm = 0; ///< Constante de celda utilizada, en cm⁻¹.
  float microSiemensPerCm = 0; ///< Conductividad estimada, en µS/cm.
  float milliSiemensPerCm = 0; ///< Conductividad estimada, en mS/cm.
  uint32_t calibrationId = 0; ///< Identificador de calibración AD.
  double magnitudeCode = 0; ///< Magnitud DFT de entrada del modelo AD.
  double gainFactorSiemensPerCode = 0; ///< GF AD utilizado, en S/código DFT.
  double systemOffsetCode = 0; ///< NOS AD utilizado, en códigos DFT.
  double measuredAdmittanceSiemens = 0; ///< Admitancia medida YX, en S.
  double cellAdmittanceSiemens = 0; ///< Admitancia de celda corregida, en S.
  double r2Denominator = 0; ///< Denominador de la corrección R2 AD.
};

/** @brief Calcula GF y NOS AD a partir de dos referencias.
 * @param nLow Magnitud DFT correspondiente a la admitancia inferior.
 * @param nHigh Magnitud DFT correspondiente a la admitancia superior; debe ser mayor que nLow.
 * @param yLowSiemens Admitancia inferior de referencia, en S; positiva.
 * @param yHighSiemens Admitancia superior de referencia, en S; mayor que yLowSiemens.
 * @param rfbOhms Resistencia nominal de realimentación, en Ω; positiva.
 * @param frequencyHz Frecuencia solicitada, en Hz; positiva.
 * @param excitationRange Rango de excitación asociado a la calibración.
 * @param pgaGain Ganancia PGA asociada a la calibración.
 * @param calibrationId Identificador que se adjunta al resultado.
 * @return Calibración con GF/NOS y metadatos, o estado de error si los puntos son inválidos.
 */
inline AdCalibration calibrateAnalogDevices(double nLow, double nHigh, double yLowSiemens,
                                             double yHighSiemens, double rfbOhms,
                                             double frequencyHz, uint8_t excitationRange,
                                             uint8_t pgaGain, uint32_t calibrationId) {
  AdCalibration c;
  c.nLow = nLow; c.nHigh = nHigh; c.yLowSiemens = yLowSiemens; c.yHighSiemens = yHighSiemens;
  c.rfbOhms = rfbOhms; c.frequencyHz = frequencyHz; c.excitationRange = excitationRange;
  c.effectiveFrequencyHz = frequencyHz;
  c.pgaGain = pgaGain; c.calibrationId = calibrationId;
  if (!isfinite(nLow) || !isfinite(nHigh) || !isfinite(yLowSiemens) || !isfinite(yHighSiemens) ||
      !isfinite(rfbOhms) || !isfinite(frequencyHz) || nLow < 0 || nHigh < 0 ||
      yLowSiemens <= 0 || yHighSiemens <= yLowSiemens || rfbOhms <= 0 || frequencyHz <= 0) {
    c.error = AdCalibrationError::InvalidPoint; return c;
  }
  if (nHigh <= nLow) { c.error = AdCalibrationError::NonIncreasingCodes; return c; }
  const double span = nHigh - nLow;
  if (span <= 1.0e-9) { c.error = AdCalibrationError::SpanTooSmall; return c; }
  c.gainFactorSiemensPerCode = (yHighSiemens - yLowSiemens) / span;  // CN-0349 Eq. 1.
  if (!isfinite(c.gainFactorSiemensPerCode) || c.gainFactorSiemensPerCode <= 0) {
    c.error = AdCalibrationError::InvalidGainFactor; return c;
  }
  c.systemOffsetCode = nHigh - yHighSiemens / c.gainFactorSiemensPerCode;  // Eq. 2.
  if (!isfinite(c.systemOffsetCode)) { c.error = AdCalibrationError::InvalidOffset; return c; }
  c.valid = true; c.error = AdCalibrationError::None; return c;
}

/** @brief Convierte una magnitud DFT a EC mediante calibración AD y corrección R2.
 * @param magnitudeCode Magnitud DFT de la muestra.
 * @param nominalKPerCm Constante de celda positiva, en cm⁻¹.
 * @param r2Ohms Resistencia R2 no negativa, en Ω.
 * @param calibration Calibración AD válida y compatible con la medida.
 * @param measurementFrequencyHz Frecuencia DDS efectiva de medida, en Hz.
 * @param expectedRfbOhms Realimentación esperada de la ruta, en Ω.
 * @param excitationRange Rango de excitación utilizado.
 * @param pgaGain Ganancia PGA utilizada.
 * @return Conductividad en µS/cm y mS/cm, valores intermedios y estado/error.
 * @note Comprueba frecuencia efectiva, RFB, rango y PGA; el controlador comprueba
 * también el resto de metadatos antes de utilizar la calibración.
 */
inline Result convertAnalogDevices(double magnitudeCode, double nominalKPerCm, double r2Ohms,
                                   const AdCalibration& calibration, double measurementFrequencyHz,
                                   double expectedRfbOhms, uint8_t excitationRange, uint8_t pgaGain) {
  Result result;
  result.model = Model::AnalogDevices; result.magnitudeCode = magnitudeCode;
  result.nominalKPerCm = static_cast<float>(nominalKPerCm); result.r2Ohms = static_cast<float>(r2Ohms);
  result.calibrationId = calibration.calibrationId;
  result.gainFactorSiemensPerCode = calibration.gainFactorSiemensPerCode;
  result.systemOffsetCode = calibration.systemOffsetCode;
  if (!calibration.valid || !isfinite(calibration.gainFactorSiemensPerCode) ||
      calibration.gainFactorSiemensPerCode <= 0 || !isfinite(calibration.systemOffsetCode)) {
    result.error = Error::CalibrationInvalid; return result;
  }
  if (!isfinite(measurementFrequencyHz) || !isfinite(expectedRfbOhms) ||
      fabs(calibration.effectiveFrequencyHz - measurementFrequencyHz) > 0.01 ||
      calibration.rfbOhms != expectedRfbOhms || calibration.excitationRange != excitationRange ||
      calibration.pgaGain != pgaGain) { result.error = Error::CalibrationMismatch; return result; }
  if (!isfinite(magnitudeCode)) { result.error = Error::InvalidMagnitude; return result; }
  if (!isfinite(nominalKPerCm) || nominalKPerCm <= 0) { result.error = Error::InvalidCellConstant; return result; }
  if (!isfinite(r2Ohms) || r2Ohms < 0) { result.error = Error::InvalidR2; return result; }
  const double yx = (magnitudeCode - calibration.systemOffsetCode) * calibration.gainFactorSiemensPerCode;  // Eq. 3.
  result.measuredAdmittanceSiemens = yx;
  if (!isfinite(yx) || yx <= 0) { result.error = Error::NonPositiveAdmittance; return result; }
  const double denominator = 1.0 - r2Ohms * yx;
  result.r2Denominator = denominator;
  if (!isfinite(denominator) || denominator <= 1.0e-12) { result.error = Error::InvalidDenominator; return result; }
  const double yCell = yx / denominator;  // CN-0349 Eq. 4.
  result.cellAdmittanceSiemens = yCell;
  const double micro = yCell * nominalKPerCm * 1.0e6;
  const double milli = yCell * nominalKPerCm * 1.0e3;
  if (!isfinite(yCell) || yCell <= 0 || !isfinite(micro) || !isfinite(milli) ||
      micro > 3.402823466e38 || milli > 3.402823466e38) { result.error = Error::Overflow; return result; }
  result.cellResistanceOhms = static_cast<float>(1.0 / yCell);
  result.microSiemensPerCm = static_cast<float>(micro); result.milliSiemensPerCm = static_cast<float>(milli);
  result.valid = true; result.error = Error::None; return result;
}

/** @brief Convierte impedancia aparente a EC mediante RAW o SERIES experimental.
 * @param apparentOhms Impedancia aparente positiva, en Ω.
 * @param nominalKPerCm Constante de celda positiva, en cm⁻¹.
 * @param model Model::Raw o Model::SeriesR2Experimental; AD usa convertAnalogDevices().
 * @param r2Ohms Resistencia R2 no negativa, en Ω; se resta solo en SERIES.
 * @return Conductividad en µS/cm y mS/cm, resistencia estimada y estado/error.
 * @note SERIES depende de una hipótesis experimental sobre R2 y no establece
 * por sí sola la exactitud de la cadena analógica o de la celda.
 */
inline Result convert(float apparentOhms, float nominalKPerCm, Model model, float r2Ohms) {
  Result result;
  result.model = model;
  result.apparentImpedanceOhms = apparentOhms;
  result.nominalKPerCm = nominalKPerCm;
  result.r2Ohms = r2Ohms;
  if (model != Model::Raw && model != Model::SeriesR2Experimental) {
    result.error = Error::InvalidModel;
    return result;
  }
  if (!isfinite(apparentOhms) || apparentOhms <= 0) {
    result.error = Error::InvalidImpedance;
    return result;
  }
  if (!isfinite(nominalKPerCm) || nominalKPerCm <= 0) {
    result.error = Error::InvalidCellConstant;
    return result;
  }
  if (!isfinite(r2Ohms) || r2Ohms < 0) {
    result.error = Error::InvalidR2;
    return result;
  }
  const double resistance = static_cast<double>(apparentOhms) -
      (model == Model::SeriesR2Experimental ? static_cast<double>(r2Ohms) : 0.0);
  if (resistance <= 0) {
    result.error = Error::NonPositiveCellResistance;
    return result;
  }
  const double micro = 1.0e6 * static_cast<double>(nominalKPerCm) / resistance;
  const double milli = micro / 1000.0;
  if (!isfinite(micro) || !isfinite(milli) || micro > 3.402823466e38 || milli > 3.402823466e38) {
    result.error = Error::Overflow;
    return result;
  }
  result.cellResistanceOhms = static_cast<float>(resistance);
  result.microSiemensPerCm = static_cast<float>(micro);
  result.milliSiemensPerCm = static_cast<float>(milli);
  if (!isfinite(result.cellResistanceOhms) || !isfinite(result.microSiemensPerCm) ||
      !isfinite(result.milliSiemensPerCm)) {
    result.error = Error::Overflow;
    return result;
  }
  result.valid = true;
  result.error = Error::None;
  return result;
}

}  // namespace Conductivity
