#include <unity.h>
#include <cmath>

#include "sensors/conversions.h"

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
  TEST_ASSERT_EQUAL_FLOAT(0, conversions::o2Percentage(-10, 10, NAN));
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

void test_he_current_missing_o2_behavior() {
  TEST_ASSERT_EQUAL_FLOAT(100, conversions::heCorrectedMillivolts(100, NAN));
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

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_o2_air_only);
  RUN_TEST(test_o2_two_point_and_clamp);
  RUN_TEST(test_he_correction_boundaries);
  RUN_TEST(test_he_current_missing_o2_behavior);
  RUN_TEST(test_he_polynomial_and_calibration_scale);
  RUN_TEST(test_co_endpoints_and_unclamped_values);
  RUN_TEST(test_temperature_current_rounding);
  return UNITY_END();
}