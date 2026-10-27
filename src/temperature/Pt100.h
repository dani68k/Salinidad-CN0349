#pragma once

#include <math.h>
#include <stdint.h>

// Pure IEC 60751 conversion. No acquisition or cable correction here.
namespace Pt100 {
constexpr double R0 = 100.0;
constexpr double A = 3.9083e-3;
constexpr double B = -5.775e-7;
constexpr double C = -4.183e-12;
enum class Error : uint8_t { NotAttempted, None, NonFinite, NonPositive, OutOfRange };
struct Result {
  bool valid = false;
  double celsius = NAN;
  Error error = Error::NotAttempted;
};
inline double resistance(double t) {
  return R0 * (1.0 + A*t + B*t*t + (t < 0 ? C*(t-100.0)*t*t*t : 0.0));
}
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
