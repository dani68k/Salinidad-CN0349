#include <unity.h>
#include <ad5934_math.h>
#include <impedance_diagnostics.h>

void test_dds_frequency_word_for_cn0349_clock(void) {
  uint32_t code = 0;
  TEST_ASSERT_TRUE(AD5934Math::frequencyToCode(2000, 1000000, code));
  TEST_ASSERT_EQUAL_HEX32(0x041893, code);
}

void test_dds_frequency_range_is_limited_by_mclk(void) {
  uint32_t code = 123;
  TEST_ASSERT_FALSE(AD5934Math::frequencyToCode(0, 1000000, code));
  TEST_ASSERT_FALSE(AD5934Math::frequencyToCode(62501, 1000000, code));
  TEST_ASSERT_EQUAL_UINT32(123, code);
}

void test_signed_dft_decoding(void) {
  TEST_ASSERT_EQUAL_INT16(0, AD5934Math::decodeSigned16(0x0000));
  TEST_ASSERT_EQUAL_INT16(32767, AD5934Math::decodeSigned16(0x7FFF));
  TEST_ASSERT_EQUAL_INT16(-32768, AD5934Math::decodeSigned16(0x8000));
  TEST_ASSERT_EQUAL_INT16(-1, AD5934Math::decodeSigned16(0xFFFF));
}

void test_magnitude_uses_wide_arithmetic(void) {
  TEST_ASSERT_DOUBLE_WITHIN(0.000001, 5.0, AD5934Math::magnitude(3, 4));
  TEST_ASSERT_DOUBLE_WITHIN(0.00001, 46340.95001184158, AD5934Math::magnitude(-32768, -32768));
}

void test_sweep_increment_count_means_points_minus_one(void) {
  uint32_t end = 0;
  TEST_ASSERT_TRUE(AD5934Math::validateSweep(1000, 250, 8, 64, 62500, end));
  TEST_ASSERT_EQUAL_UINT32(3000, end);
  TEST_ASSERT_FALSE(AD5934Math::validateSweep(1000, 250, 64, 64, 62500, end));
}

void test_sweep_rejects_zero_step_with_multiple_points_and_bad_end(void) {
  uint32_t end = 0;
  TEST_ASSERT_FALSE(AD5934Math::validateSweep(1000, 0, 1, 64, 62500, end));
  TEST_ASSERT_FALSE(AD5934Math::validateSweep(62000, 1000, 1, 64, 62500, end));
  TEST_ASSERT_TRUE(AD5934Math::validateSweep(1000, 0, 0, 64, 62500, end));
}

void test_sweep_dds_accumulation_stays_in_24_bits(void) {
  TEST_ASSERT_TRUE(AD5934Math::validSweepDdsCodes(100, 10, 8));
  TEST_ASSERT_FALSE(AD5934Math::validSweepDdsCodes(0xFFFFF0, 2, 8));
}

void test_sweep_frequency_and_phase_math(void) {
  TEST_ASSERT_DOUBLE_WITHIN(0.005, 2000.0, AD5934Math::frequencyFromCode(0x041893, 1000000));
  TEST_ASSERT_DOUBLE_WITHIN(0.00001, 90.0, AD5934Math::phaseDegrees(0, 100));
  TEST_ASSERT_DOUBLE_WITHIN(0.00001, -90.0, AD5934Math::phaseDegrees(0, -100));
  TEST_ASSERT_DOUBLE_WITHIN(0.00001, 180.0, AD5934Math::phaseDegrees(-1, 0));
}

void test_settling_reserved_multiplier(void) {
  TEST_ASSERT_TRUE(AD5934Math::validSettlingWord(10));
  TEST_ASSERT_TRUE(AD5934Math::validSettlingWord(0x07FF));
  TEST_ASSERT_FALSE(AD5934Math::validSettlingWord(0x040A));
  TEST_ASSERT_FALSE(AD5934Math::validSettlingWord(0x0800));
}

void test_control_masks_preserve_reserved_bits(void) {
  TEST_ASSERT_TRUE(AD5934Math::validInactiveControl(0xA108));
  TEST_ASSERT_TRUE(AD5934Math::validInactiveControl(0xB108));
  TEST_ASSERT_FALSE(AD5934Math::validInactiveControl(0xA100));
  TEST_ASSERT_FALSE(AD5934Math::validInactiveControl(0xA118));
  TEST_ASSERT_EQUAL_HEX8(0xA1, AD5934Math::controlHighRange1PgaX1());
  TEST_ASSERT_EQUAL_HEX8(0x11, AD5934Math::controlHighWithOperation(0xA1, 1));
  TEST_ASSERT_EQUAL_HEX8(0x21, AD5934Math::controlHighWithOperation(0xA1, 2));
  TEST_ASSERT_FALSE(AD5934Math::validControlOperation(0x04));
  TEST_ASSERT_TRUE(AD5934Math::validControlTransition(0xA108, 1));
  TEST_ASSERT_TRUE(AD5934Math::validControlTransition(0xB108, 1));
  TEST_ASSERT_TRUE(AD5934Math::validControlTransition(0x1108, 2));
  TEST_ASSERT_TRUE(AD5934Math::validControlOperation(3));
  TEST_ASSERT_TRUE(AD5934Math::validControlTransition(0x2108, 3));
  TEST_ASSERT_TRUE(AD5934Math::validControlTransition(0x0008, 3));
  TEST_ASSERT_TRUE(AD5934Math::validControlTransition(0xB108, 3));
  TEST_ASSERT_FALSE(AD5934Math::validControlTransition(0xA108, 2));
  TEST_ASSERT_FALSE(AD5934Math::validControlTransition(0x2100, 1));
}

void test_typed_impedance_error_names(void) {
  TEST_ASSERT_EQUAL_STRING("NONE", ImpedanceDiagnostics::errorName(ImpedanceDiagnostics::ErrorCode::None));
  TEST_ASSERT_EQUAL_STRING("I2C_NACK", ImpedanceDiagnostics::errorName(ImpedanceDiagnostics::ErrorCode::I2cNack));
  TEST_ASSERT_EQUAL_STRING("DFT_TIMEOUT", ImpedanceDiagnostics::errorName(ImpedanceDiagnostics::ErrorCode::DftTimeout));
  TEST_ASSERT_EQUAL_STRING("RECOVERY_FAILED", ImpedanceDiagnostics::errorName(ImpedanceDiagnostics::ErrorCode::RecoveryFailed));
  ImpedanceDiagnostics::ErrorRecord record{};
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ImpedanceDiagnostics::ErrorCode::None), static_cast<uint8_t>(record.code));
  TEST_ASSERT_FALSE(record.recoverable);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_dds_frequency_word_for_cn0349_clock);
  RUN_TEST(test_dds_frequency_range_is_limited_by_mclk);
  RUN_TEST(test_signed_dft_decoding);
  RUN_TEST(test_magnitude_uses_wide_arithmetic);
  RUN_TEST(test_sweep_increment_count_means_points_minus_one);
  RUN_TEST(test_sweep_rejects_zero_step_with_multiple_points_and_bad_end);
  RUN_TEST(test_sweep_dds_accumulation_stays_in_24_bits);
  RUN_TEST(test_sweep_frequency_and_phase_math);
  RUN_TEST(test_settling_reserved_multiplier);
  RUN_TEST(test_control_masks_preserve_reserved_bits);
  RUN_TEST(test_typed_impedance_error_names);
  return UNITY_END();
}
