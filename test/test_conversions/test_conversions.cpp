#include <unity.h>
#include <cmath>
#include <limits>

#include "sensors/conversions.h"
#include "display/UiPresentation.h"

constexpr float kTolerance = 0.0001f;

void setUp() {}
void tearDown() {}

void test_o2_air_only() {
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 20.9f, conversions::o2Percentage(10, 10, NAN));
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 41.8f, conversions::o2Percentage(20, 10, NAN));
}

void test_o2_two_point_and_clamp() {
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 20.9f, conversions::o2Percentage(10, 10, 50));
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 60.45f, conversions::o2Percentage(30, 10, 50));
  TEST_ASSERT_EQUAL_FLOAT(100, conversions::o2Percentage(60, 10, 50));
  TEST_ASSERT_EQUAL_FLOAT(0, conversions::o2Percentage(0, 10, NAN));
}

void test_he_correction_boundaries() {
  TEST_ASSERT_EQUAL_FLOAT(100, conversions::heCorrectedMillivolts(100, 40));
  TEST_ASSERT_EQUAL_FLOAT(100, conversions::heCorrectedMillivolts(100, 40.1f));
  TEST_ASSERT_EQUAL_FLOAT(100, conversions::heCorrectedMillivolts(100, 44));
  const float boundaries[] = {48, 52, 57, 62, 66, 71, 75, 82, 89};
  const float corrections[] = {0, 9, 10, 11, 12, 13, 14, 15, 16};
  for (unsigned index = 0; index < 9; ++index) {
    TEST_ASSERT_EQUAL_FLOAT(100 - corrections[index],
                           conversions::heCorrectedMillivolts(100, boundaries[index]));
    TEST_ASSERT_EQUAL_FLOAT(100 - (9 + index),
                           conversions::heCorrectedMillivolts(100, boundaries[index] + 0.1f));
  }
}

void test_he_requires_usable_o2() {
  TEST_ASSERT_TRUE(std::isnan(conversions::heCorrectedMillivolts(100, NAN)));
  TEST_ASSERT_TRUE(std::isnan(conversions::heCorrectedMillivolts(100, INFINITY)));
  TEST_ASSERT_TRUE(std::isnan(conversions::heCorrectedMillivolts(100, -1)));
  TEST_ASSERT_TRUE(std::isnan(conversions::heCorrectedMillivolts(100, 101)));
  TEST_ASSERT_TRUE(std::isnan(conversions::heCorrectedMillivolts(10, 100)));
}

void test_he_polynomial_and_calibration_scale() {
  TEST_ASSERT_EQUAL_FLOAT(0, conversions::hePercentage(0, 621.2f));
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 14.3614f, conversions::hePercentage(100, 621.2f));
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 28.4648f, conversions::hePercentage(200, 621.2f));
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 14.3614f, conversions::hePercentage(50, 310.6f));
}

void test_co_endpoints_and_unclamped_values() {
  TEST_ASSERT_EQUAL_FLOAT(0, conversions::coPpm(400));
  TEST_ASSERT_EQUAL_FLOAT(250, conversions::coPpm(1200));
  TEST_ASSERT_EQUAL_FLOAT(500, conversions::coPpm(2000));
  TEST_ASSERT_EQUAL_FLOAT(-125, conversions::coPpm(0));
  TEST_ASSERT_EQUAL_FLOAT(625, conversions::coPpm(2400));
}

void test_temperature_current_rounding() {
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 25, conversions::temperatureCelsius(250));
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 25.13f, conversions::temperatureCelsius(251.26f));
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, -1.01f, conversions::temperatureCelsius(-10.06f));
}

void test_o2_rejects_invalid_readings_and_calibrations() {
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 20.9f, conversions::o2Percentage(5, 5, NAN));
  TEST_ASSERT_EQUAL_FLOAT(100, conversions::o2Percentage(100, 10, NAN));
  const float invalidInputs[] = {NAN, INFINITY, -INFINITY, -1, 101};
  for (float value : invalidInputs) {
    TEST_ASSERT_TRUE(std::isnan(conversions::o2Percentage(value, 10, NAN)));
  }
  const float invalidAir[] = {NAN, INFINITY, -INFINITY, -1, 0, 4.9f};
  for (float value : invalidAir) {
    TEST_ASSERT_TRUE(std::isnan(conversions::o2Percentage(10, value, NAN)));
  }
  const float invalidPure[] = {INFINITY, -INFINITY, -1, 0, 10};
  for (float value : invalidPure) {
    TEST_ASSERT_TRUE(std::isnan(conversions::o2Percentage(10, 10, value)));
  }
}

void test_he_rejects_invalid_denominators_and_overflow() {
  const float invalid[] = {NAN, INFINITY, -INFINITY, -1, 0};
  for (float value : invalid) {
    TEST_ASSERT_TRUE(std::isnan(conversions::hePercentage(100, value)));
  }
  TEST_ASSERT_TRUE(std::isnan(conversions::hePercentage(INFINITY, 621.2f)));
  TEST_ASSERT_TRUE(std::isnan(conversions::hePercentage(-1, 621.2f)));
  TEST_ASSERT_TRUE(std::isnan(conversions::hePercentage(
      std::numeric_limits<float>::max(), std::numeric_limits<float>::min())));
}

void test_co_and_temperature_reject_non_finite_inputs() {
  const float invalid[] = {NAN, INFINITY, -INFINITY};
  for (float value : invalid) {
    TEST_ASSERT_TRUE(std::isnan(conversions::coPpm(value)));
    TEST_ASSERT_TRUE(std::isnan(conversions::temperatureCelsius(value)));
  }
  TEST_ASSERT_TRUE(std::isnan(conversions::coPpm(-1)));
  TEST_ASSERT_TRUE(std::isnan(conversions::temperatureCelsius(std::numeric_limits<float>::max())));
}

void test_integer_conversion_checks_range_before_casting() {
  int result = 123;
  const float invalid[] = {NAN, INFINITY, -INFINITY, std::numeric_limits<float>::max(),
                          static_cast<float>(std::numeric_limits<int>::max())};
  for (float value : invalid) {
    TEST_ASSERT_FALSE(conversions::toInt(value, result));
    TEST_ASSERT_EQUAL_INT(123, result);
  }
  TEST_ASSERT_TRUE(conversions::toInt(25.9f, result));
  TEST_ASSERT_EQUAL_INT(25, result);
  TEST_ASSERT_TRUE(conversions::toInt(-25.9f, result));
  TEST_ASSERT_EQUAL_INT(-25, result);
  TEST_ASSERT_TRUE(conversions::toInt(static_cast<float>(std::numeric_limits<int>::min()), result));
  TEST_ASSERT_EQUAL_INT(std::numeric_limits<int>::min(), result);
  const float largest = std::nextafter(static_cast<float>(std::numeric_limits<int>::max()), 0.0f);
  TEST_ASSERT_TRUE(conversions::toInt(largest, result));
  TEST_ASSERT_EQUAL_INT(2147483520, result);
}

void test_mod_requires_positive_finite_inputs() {
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 33.75f, conversions::maximumOperatingDepth(1.4f, 32));
  const float invalid[] = {NAN, INFINITY, -INFINITY, 0, -1};
  for (float value : invalid) {
    TEST_ASSERT_TRUE(std::isnan(conversions::maximumOperatingDepth(1.4f, value)));
    TEST_ASSERT_TRUE(std::isnan(conversions::maximumOperatingDepth(value, 32)));
  }
  TEST_ASSERT_TRUE(std::isnan(conversions::maximumOperatingDepth(1.4f, 101)));
  int depth;
  TEST_ASSERT_FALSE(conversions::toInt(
      conversions::maximumOperatingDepth(1.4f, std::numeric_limits<float>::min()), depth));
}

void test_ui_formats_states_and_mod() {
  char text[32];
  ui::formatValue(text, sizeof(text), 20.9f, ChannelState::Valid, "%.1f%%");
  TEST_ASSERT_EQUAL_STRING("20.9%", text);
  ui::formatValue(text, sizeof(text), 20.9f, ChannelState::Stale, "%.1f%%");
  TEST_ASSERT_EQUAL_STRING("Stale", text);
  ui::formatValue(text, sizeof(text), 0, ChannelState::Warming, "%.0f ppm");
  TEST_ASSERT_EQUAL_STRING("Warming", text);
  ui::formatValue(text, sizeof(text), NAN, ChannelState::Valid, "%.1f%%");
  TEST_ASSERT_EQUAL_STRING("Invalid", text);
  ui::formatMod(text, sizeof(text), "B", 1.4f, 20.9f, ChannelState::Valid);
  TEST_ASSERT_EQUAL_STRING("B 56m", text);
  ui::formatMod(text, sizeof(text), "D", 1.6f, 0, ChannelState::Valid);
  TEST_ASSERT_EQUAL_STRING("D --m", text);
  ui::formatMod(text, sizeof(text), "B", 1.4f, 20.9f, ChannelState::Stale);
  TEST_ASSERT_EQUAL_STRING("B --m", text);
}

void test_ui_settings_indices_preserve_calibration() {
  AnalyzerSettings candidate;
  candidate.o2Air = 12;
  candidate.o2Pure = 55;
  candidate.heCalibration = 610;
  TEST_ASSERT_TRUE(ui::applySelections(candidate, 3, 5, 200, 4));
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 1.3f, candidate.po2Bottom);
  TEST_ASSERT_FLOAT_WITHIN(kTolerance, 1.5f, candidate.po2Deco);
  TEST_ASSERT_EQUAL_UINT8(10, candidate.sleepMinutes);
  TEST_ASSERT_EQUAL_FLOAT(12, candidate.o2Air);
  TEST_ASSERT_EQUAL_FLOAT(55, candidate.o2Pure);
  TEST_ASSERT_EQUAL_FLOAT(610, candidate.heCalibration);
  TEST_ASSERT_FALSE(ui::applySelections(candidate, 6, 0, 128, 3));
  TEST_ASSERT_FALSE(ui::applySelections(candidate, 3, 5, 128, -1));
  TEST_ASSERT_FALSE(ui::applySelections(candidate, 3, 5, 256, 3));
  TEST_ASSERT_EQUAL_UINT8(200, candidate.brightness);
  candidate.po2Bottom = 1.35f;
  TEST_ASSERT_TRUE(ui::applySelections(candidate, ui::po2Selection(1.35f), 5, 200, 4));
  TEST_ASSERT_EQUAL_FLOAT(1.35f, candidate.po2Bottom);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_o2_air_only);
  RUN_TEST(test_o2_two_point_and_clamp);
  RUN_TEST(test_he_correction_boundaries);
  RUN_TEST(test_he_requires_usable_o2);
  RUN_TEST(test_he_polynomial_and_calibration_scale);
  RUN_TEST(test_co_endpoints_and_unclamped_values);
  RUN_TEST(test_temperature_current_rounding);
  RUN_TEST(test_o2_rejects_invalid_readings_and_calibrations);
  RUN_TEST(test_he_rejects_invalid_denominators_and_overflow);
  RUN_TEST(test_co_and_temperature_reject_non_finite_inputs);
  RUN_TEST(test_integer_conversion_checks_range_before_casting);
  RUN_TEST(test_mod_requires_positive_finite_inputs);
  RUN_TEST(test_ui_formats_states_and_mod);
  RUN_TEST(test_ui_settings_indices_preserve_calibration);
  return UNITY_END();
}