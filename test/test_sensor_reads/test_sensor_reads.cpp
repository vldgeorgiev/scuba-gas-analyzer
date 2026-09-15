#include <unity.h>

#include "sensors/COSensor.h"
#include "sensors/O2Sensor.h"
#include "sensors/HESensor.h"
#include "sensors/TempSensor.h"

#include "sensors/sensors.h"

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
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 217, next.millivolts);
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

void test_default_snapshot_contains_no_numeric_readings() {
  sensorsData data;
  TEST_ASSERT_TRUE(std::isnan(data.O2Level.millivolts));
  TEST_ASSERT_TRUE(std::isnan(data.O2Level.percentage));
  TEST_ASSERT_TRUE(std::isnan(data.CoLevel.millivolts));
  TEST_ASSERT_TRUE(std::isnan(data.CoLevel.ppm));
  TEST_ASSERT_TRUE(std::isnan(data.HeLevel.millivolts));
  TEST_ASSERT_TRUE(std::isnan(data.HeLevel.percentage));
  TEST_ASSERT_TRUE(std::isnan(data.HeTemperature));
  TEST_ASSERT_EQUAL(SensorError::None, data.lastError);
}

void test_invalid_he_dependency_preserves_raw_input_and_recovers() {
  Adafruit_ADS1115 adc;
  adc.counts = 1600;
  HESensor sensor(adc);
  sensor.setCalibrations(621.2f);
  const HEReading invalid = sensor.readLevel(NAN);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 100, invalid.millivolts);
  TEST_ASSERT_TRUE(std::isnan(invalid.percentage));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 14.3614f, sensor.readLevel(20.9f).percentage);
  sensor.setCalibrations(0);
  TEST_ASSERT_TRUE(std::isnan(sensor.readLevel(20.9f).percentage));
}

void test_invalid_o2_pair_does_not_become_a_clamped_value() {
  Adafruit_ADS1115 adc;
  adc.millivoltsPerCount = 0.03125f;
  adc.counts = 640;
  O2Sensor sensor(adc);
  sensor.setCalibrations(10, 10);
  const O2Reading invalid = sensor.readLevel();
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20, invalid.millivolts);
  TEST_ASSERT_TRUE(std::isnan(invalid.percentage));
  sensor.setCalibrations(10, NAN);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 41.8f, sensor.readLevel().percentage);
}

void test_invalid_co_remains_unavailable_without_integer_cast() {
  Adafruit_ADS1115 adc;
  adc.counts = -1;
  COSensor sensor(3, adc);
  const COReading invalid = sensor.readLevel();
  TEST_ASSERT_TRUE(std::isnan(invalid.ppm));
  adc.counts = 6400;
  TEST_ASSERT_EQUAL_FLOAT(0, sensor.readLevel().ppm);
}

void test_all_disabled_publishes_complete_snapshot_without_reading() {
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager manager(handle);
  TEST_ASSERT_EQUAL(SensorError::None, manager.init());
  TEST_ASSERT_EQUAL(SensorError::None, manager.readSensors());
  TEST_ASSERT_EQUAL_UINT(1, queue.sends);
  sensorsData snapshot;
  std::memcpy(&snapshot, queue.data.data(), sizeof(snapshot));
  TEST_ASSERT_TRUE(std::isnan(snapshot.CoLevel.ppm));
  TEST_ASSERT_TRUE(std::isnan(snapshot.HeTemperature));
  TEST_ASSERT_TRUE(std::isnan(snapshot.O2Level.percentage));
  TEST_ASSERT_TRUE(std::isnan(snapshot.HeLevel.percentage));
  TEST_ASSERT_EQUAL(SensorError::None, snapshot.lastError);
  TEST_ASSERT_EQUAL_UINT(0, Adafruit_ADS1115::devices[0]->differential23Reads);
  TEST_ASSERT_EQUAL_UINT(0, Adafruit_ADS1115::devices[1]->differential01Reads);
  TEST_ASSERT_EQUAL_UINT(0, Adafruit_ADS1115::devices[1]->singleEndedReads);
}

void test_cycle_clears_invalid_error_and_skips_unneeded_temperature() {
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager manager(handle);
  manager.init();
  manager.setSensorsConfig(true, false, false, 10, NAN, 621.2f);
  Adafruit_ADS1115::devices[0]->counts = 6400;
  TEST_ASSERT_EQUAL(SensorError::Invalid_Reading, manager.readSensors());
  Adafruit_ADS1115::devices[0]->counts = 320;
  TEST_ASSERT_EQUAL(SensorError::None, manager.readSensors());
  TEST_ASSERT_EQUAL(SensorError::None, manager.getLastError());
  sensorsData snapshot;
  std::memcpy(&snapshot, queue.data.data(), sizeof(snapshot));
  TEST_ASSERT_EQUAL(SensorError::None, snapshot.lastError);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.9f, snapshot.O2Level.percentage);
  TEST_ASSERT_TRUE(std::isnan(snapshot.HeTemperature));
  TEST_ASSERT_EQUAL_UINT(0, Adafruit_ADS1115::devices[1]->singleEndedReads);

  manager.setSensorsConfig(false, true, false, 10, NAN, 621.2f);
  Adafruit_ADS1115::devices[1]->counts = 6400;
  TEST_ASSERT_EQUAL(SensorError::None, manager.readSensors());
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[1]->singleEndedReads);
  TEST_ASSERT_EQUAL_UINT8(3, Adafruit_ADS1115::devices[1]->lastChannel);
}

void test_he_only_is_unavailable_but_retains_raw_and_temperature() {
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager manager(handle);
  manager.init();
  manager.setSensorsConfig(false, false, true, 10, NAN, 621.2f);
  Adafruit_ADS1115::devices[1]->counts = 1600;
  TEST_ASSERT_EQUAL(SensorError::Invalid_Reading, manager.readSensors());
  sensorsData snapshot;
  std::memcpy(&snapshot, queue.data.data(), sizeof(snapshot));
  TEST_ASSERT_TRUE(std::isnan(snapshot.HeLevel.percentage));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 100, snapshot.HeLevel.millivolts);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 10, snapshot.HeTemperature);
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[1]->differential01Reads);
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[1]->singleEndedReads);
  TEST_ASSERT_EQUAL_UINT8(2, Adafruit_ADS1115::devices[1]->lastChannel);
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
  RUN_TEST(test_default_snapshot_contains_no_numeric_readings);
  RUN_TEST(test_invalid_he_dependency_preserves_raw_input_and_recovers);
  RUN_TEST(test_invalid_o2_pair_does_not_become_a_clamped_value);
  RUN_TEST(test_invalid_co_remains_unavailable_without_integer_cast);
  RUN_TEST(test_all_disabled_publishes_complete_snapshot_without_reading);
  RUN_TEST(test_cycle_clears_invalid_error_and_skips_unneeded_temperature);
  RUN_TEST(test_he_only_is_unavailable_but_retains_raw_and_temperature);
  return UNITY_END();
}