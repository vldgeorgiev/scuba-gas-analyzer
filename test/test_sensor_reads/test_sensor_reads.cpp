#include <unity.h>

#include "sensors/COSensor.h"
#include "sensors/O2Sensor.h"
#include "sensors/HESensor.h"
#include "sensors/TempSensor.h"

void setUp() {
  delayCalls = 0;
  delayedMs = 0;
}
void tearDown() {}

void test_co_reads_one_conversion_and_follows_the_next_input() {
  Adafruit_ADS1115 adc;
  COSensor sensor(3, adc);
  adc.counts = 6400;
  const COReading first = sensor.readLevel();
  TEST_ASSERT_EQUAL_UINT(1, adc.singleEndedReads);
  TEST_ASSERT_EQUAL_UINT8(3, adc.lastChannel);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 400, first.millivolts);
  TEST_ASSERT_EQUAL_INT(0, first.ppm);

  adc.counts = 19200;
  const COReading next = sensor.readLevel();
  TEST_ASSERT_EQUAL_UINT(2, adc.singleEndedReads);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1200, next.millivolts);
  TEST_ASSERT_EQUAL_INT(250, next.ppm);
}

void test_o2_reads_one_conversion_and_preserves_polarity_handling() {
  Adafruit_ADS1115 adc;
  adc.millivoltsPerCount = 0.03125f;
  O2Sensor sensor(adc);
  sensor.setCalibrations(10, NAN);
  adc.counts = -320;
  const O2Reading first = sensor.readLevel();
  TEST_ASSERT_EQUAL_UINT(1, adc.differential23Reads);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 10, first.millivolts);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.9f, first.percentage);

  adc.counts = 640;
  const O2Reading next = sensor.readLevel();
  TEST_ASSERT_EQUAL_UINT(2, adc.differential23Reads);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 41.8f, next.percentage);
}

void test_invalid_o2_calibration_still_skips_the_read() {
  Adafruit_ADS1115 adc;
  O2Sensor sensor(adc);
  const O2Reading reading = sensor.readLevel();
  TEST_ASSERT_EQUAL_UINT(0, adc.differential23Reads);
  TEST_ASSERT_TRUE(std::isnan(reading.millivolts));
  TEST_ASSERT_TRUE(std::isnan(reading.percentage));
}

void test_he_reads_one_conversion_and_keeps_the_existing_correction() {
  Adafruit_ADS1115 adc;
  HESensor sensor(adc);
  sensor.setCalibrations(621.2f);
  adc.counts = -1600;
  const HEReading first = sensor.readLevel(20.9f);
  TEST_ASSERT_EQUAL_UINT(1, adc.differential01Reads);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 100, first.millivolts);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 14.3614f, first.percentage);

  adc.counts = 3472;
  const HEReading next = sensor.readLevel(100);
  TEST_ASSERT_EQUAL_UINT(2, adc.differential01Reads);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 200, next.millivolts);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 28.4648f, next.percentage);
}

void test_temperature_still_uses_one_conversion() {
  Adafruit_ADS1115 adc;
  TempSensor sensor(2, adc);
  adc.counts = 4000;
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 25, sensor.readLevel());
  TEST_ASSERT_EQUAL_UINT(1, adc.singleEndedReads);
  TEST_ASSERT_EQUAL_UINT8(2, adc.lastChannel);
}

void test_o2_calibration_keeps_its_original_sample_count_and_pauses() {
  Adafruit_ADS1115 adc;
  adc.millivoltsPerCount = 0.03125f;
  adc.counts = 320;
  O2Sensor sensor(adc);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 10, sensor.calibrate());
  TEST_ASSERT_EQUAL_UINT(100, adc.differential23Reads);
  TEST_ASSERT_EQUAL_UINT(5, delayCalls);
  TEST_ASSERT_EQUAL_UINT(500, delayedMs);
}

void test_he_calibration_keeps_its_original_sample_count_and_pauses() {
  Adafruit_ADS1115 adc;
  adc.counts = 9600;
  HESensor sensor(adc);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 600, sensor.calibrate());
  TEST_ASSERT_EQUAL_UINT(100, adc.differential01Reads);
  TEST_ASSERT_EQUAL_UINT(5, delayCalls);
  TEST_ASSERT_EQUAL_UINT(500, delayedMs);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_co_reads_one_conversion_and_follows_the_next_input);
  RUN_TEST(test_o2_reads_one_conversion_and_preserves_polarity_handling);
  RUN_TEST(test_invalid_o2_calibration_still_skips_the_read);
  RUN_TEST(test_he_reads_one_conversion_and_keeps_the_existing_correction);
  RUN_TEST(test_temperature_still_uses_one_conversion);
  RUN_TEST(test_o2_calibration_keeps_its_original_sample_count_and_pauses);
  RUN_TEST(test_he_calibration_keeps_its_original_sample_count_and_pauses);
  return UNITY_END();
}