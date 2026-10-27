#include <unity.h>
#include <initializer_list>
#include "../../src/temperature/Pt100.h"

// Independent forward reference using the specified full precision constants.
static double reference(double t) {
  double ratio = 1 + 3.9083e-3*t - 5.775e-7*t*t;
  if (t < 0) ratio -= 4.183e-12*(t-100)*t*t*t;
  return 100*ratio;
}
void points() {
  const double temperatures[] = {-200,-100,0,20,25,40,100,850};
  for (double t : temperatures) {
    const auto r = Pt100::convert(reference(t));
    TEST_ASSERT_TRUE(r.valid);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Pt100::Error::None), static_cast<int>(r.error));
    TEST_ASSERT_DOUBLE_WITHIN(1e-8, t, r.celsius);
  }
}
void continuity() {
  for (double t : {-1e-6, 0.0, 1e-6}) {
    const auto r = Pt100::convert(reference(t));
    TEST_ASSERT_TRUE(r.valid);
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, t, r.celsius);
  }
}
void invalid() {
  for (double ohms : {reference(-200)-1e-8, reference(850)+1e-8}) {
    auto r = Pt100::convert(ohms);
    TEST_ASSERT_FALSE(r.valid); TEST_ASSERT_TRUE(isnan(r.celsius));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Pt100::Error::OutOfRange), static_cast<int>(r.error));
  }
  for (double ohms : {0.0, -1.0}) {
    auto r = Pt100::convert(ohms); TEST_ASSERT_FALSE(r.valid);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Pt100::Error::NonPositive), static_cast<int>(r.error));
  }
  for (double ohms : {static_cast<double>(NAN), static_cast<double>(INFINITY), -static_cast<double>(INFINITY)}) {
    auto r = Pt100::convert(ohms); TEST_ASSERT_FALSE(r.valid); TEST_ASSERT_TRUE(isnan(r.celsius));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Pt100::Error::NonFinite), static_cast<int>(r.error));
  }
}
void setUp() {}
void tearDown() {}
int main() { UNITY_BEGIN(); RUN_TEST(points); RUN_TEST(continuity); RUN_TEST(invalid); return UNITY_END(); }
