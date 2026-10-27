#pragma once
#include <math.h>
#include <float.h>
namespace CN0349Math {
// Bound only the rounding from 2B's float accumulation of five DDS frequencies.
// This is a numerical allowance, never a frequency/metrology tolerance.
inline bool sameEffectiveFrequency(float a, float b) {
  return isfinite(a) && isfinite(b) && a > 0 && b > 0 &&
      fabsf(a - b) <= 5.0f * FLT_EPSILON * fmaxf(a, b);
}
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
