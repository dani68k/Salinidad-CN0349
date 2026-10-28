#pragma once

#include <math.h>
#include <stdint.h>

namespace Conductivity {

enum class Model { Raw, SeriesR2Experimental, AnalogDevices };
enum class Error { None, NotRun, InvalidModel, InvalidImpedance, InvalidCellConstant, InvalidR2,
  NonPositiveCellResistance, Overflow, CalibrationInvalid, CalibrationMismatch,
  InvalidMagnitude, NonPositiveAdmittance, InvalidDenominator };

enum class AdCalibrationError { None, NotRun, InvalidPoint, NonIncreasingCodes, SpanTooSmall,
  InvalidGainFactor, InvalidOffset, ConfigurationMismatch };

struct AdCalibration {
  bool valid = false;
  AdCalibrationError error = AdCalibrationError::NotRun;
  double nLow = 0;
  double nHigh = 0;
  double yLowSiemens = 0;
  double yHighSiemens = 0;
  double gainFactorSiemensPerCode = 0;
  double systemOffsetCode = 0;
  double rfbOhms = 0;
  double frequencyHz = 0;
  double effectiveFrequencyHz = 0;
  double mclkHz = 0;
  uint16_t settlingCycles = 0;
  uint8_t excitationRange = 0;
  uint8_t pgaGain = 0;
  uint32_t calibrationId = 0;
};

struct Result {
  bool valid = false;
  Error error = Error::NotRun;
  Model model = Model::Raw;
  float apparentImpedanceOhms = 0;
  float r2Ohms = 0;
  float cellResistanceOhms = 0;
  float nominalKPerCm = 0;
  float microSiemensPerCm = 0;
  float milliSiemensPerCm = 0;
  uint32_t calibrationId = 0;
  double magnitudeCode = 0;
  double gainFactorSiemensPerCode = 0;
  double systemOffsetCode = 0;
  double measuredAdmittanceSiemens = 0;
  double cellAdmittanceSiemens = 0;
  double r2Denominator = 0;
};

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
