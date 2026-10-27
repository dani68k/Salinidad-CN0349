#pragma once

#include <math.h>

namespace Conductivity {

enum class Model { Raw, SeriesR2Experimental };
enum class Error { None, NotRun, InvalidModel, InvalidImpedance, InvalidCellConstant, InvalidR2, NonPositiveCellResistance, Overflow };

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
};

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
