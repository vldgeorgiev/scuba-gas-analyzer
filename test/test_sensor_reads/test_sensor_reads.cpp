#include <unity.h>

#include "sensors/COSensor.h"
#include "sensors/O2Sensor.h"
#include "sensors/HESensor.h"
#include "sensors/TempSensor.h"

#include "sensors/sensors.h"
#include "display/ReadingStatus.h"

void setUp() {
  delayCalls = 0;
  delayedMs = 0;
  nowMs = 0;
  Adafruit_ADS1115::present[0] = true;
  Adafruit_ADS1115::present[1] = true;
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

void test_slow_consumer_receives_only_latest_measurement() {
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager manager(handle);
  manager.init();
  manager.setSensorsConfig(true, false, false, 10, NAN, 621.2f);
  for (unsigned sample = 1; sample <= 10; ++sample) {
    nowMs = sample * 500;
    Adafruit_ADS1115::devices[0]->counts = sample * 32;
    TEST_ASSERT_EQUAL(SensorError::None, manager.readSensors());
  }
  TEST_ASSERT_EQUAL_UINT(10, queue.sends);
  TEST_ASSERT_EQUAL_UINT(10, Adafruit_ADS1115::devices[0]->differential23Reads);
  sensorsData received;
  TEST_ASSERT_EQUAL_INT(1, xQueueReceive(handle, &received, 0));
  TEST_ASSERT_EQUAL_UINT32(5000, received.timestampMs);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.9f, received.O2Level.percentage);
  TEST_ASSERT_EQUAL_INT(0, xQueueReceive(handle, &received, 0));
}

void test_stopped_producer_blanks_all_numbers_without_new_sample() {
  sensorsData latest;
  latest.timestampMs = 500;
  latest.O2Level = {10, 20.9f};
  latest.CoLevel = {400, 0};
  latest.HeLevel = {100, 14.3614f};
  latest.HeTemperature = 25;
  TEST_ASSERT_EQUAL_FLOAT(20.9f, latest.forDisplay(2299).O2Level.percentage);
  const sensorsData stale = latest.forDisplay(2300);
  TEST_ASSERT_TRUE(std::isnan(stale.O2Level.percentage));
  TEST_ASSERT_TRUE(std::isnan(stale.O2Level.millivolts));
  TEST_ASSERT_TRUE(std::isnan(stale.CoLevel.ppm));
  TEST_ASSERT_TRUE(std::isnan(stale.CoLevel.millivolts));
  TEST_ASSERT_TRUE(std::isnan(stale.HeLevel.percentage));
  TEST_ASSERT_TRUE(std::isnan(stale.HeLevel.millivolts));
  TEST_ASSERT_TRUE(std::isnan(stale.HeTemperature));
  TEST_ASSERT_TRUE(std::isnan(conversions::maximumOperatingDepth(1.4f, stale.O2Level.percentage)));
  TEST_ASSERT_EQUAL_FLOAT(20.9f, latest.O2Level.percentage);
}

void test_freshness_survives_clock_wrap_and_new_sample_restores_display() {
  sensorsData latest;
  latest.timestampMs = UINT32_MAX - 500;
  latest.O2Level = {10, 20.9f};
  latest.CoLevel = {400, 0};
  latest.HeLevel = {100, 14.3614f};
  latest.HeTemperature = 25;
  TEST_ASSERT_EQUAL_FLOAT(20.9f, latest.forDisplay(1298).O2Level.percentage);
  TEST_ASSERT_TRUE(std::isnan(latest.forDisplay(1299).O2Level.percentage));
  latest.timestampMs = 1400;
  TEST_ASSERT_EQUAL_FLOAT(20.9f, latest.forDisplay(1400).O2Level.percentage);
  const sensorsData restored = latest.forDisplay(1400);
  TEST_ASSERT_EQUAL_FLOAT(10, restored.O2Level.millivolts);
  TEST_ASSERT_EQUAL_FLOAT(400, restored.CoLevel.millivolts);
  TEST_ASSERT_EQUAL_FLOAT(0, restored.CoLevel.ppm);
  TEST_ASSERT_EQUAL_FLOAT(100, restored.HeLevel.millivolts);
  TEST_ASSERT_EQUAL_FLOAT(14.3614f, restored.HeLevel.percentage);
  TEST_ASSERT_EQUAL_FLOAT(25, restored.HeTemperature);
}

void test_disabled_and_invalid_cycles_are_timestamped_without_reviving_numbers() {
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager manager(handle);
  manager.init();
  nowMs = 123;
  manager.readSensors();
  sensorsData latest;
  xQueueReceive(handle, &latest, 0);
  TEST_ASSERT_EQUAL_UINT32(123, latest.timestampMs);
  TEST_ASSERT_TRUE(std::isnan(latest.forDisplay(123).CoLevel.ppm));
  manager.setSensorsConfig(true, false, false, 10, NAN, 621.2f);
  Adafruit_ADS1115::devices[0]->counts = 6400;
  nowMs = 456;
  manager.readSensors();
  xQueueReceive(handle, &latest, 0);
  TEST_ASSERT_EQUAL_UINT32(456, latest.timestampMs);
  TEST_ASSERT_TRUE(std::isnan(latest.forDisplay(456).O2Level.percentage));
  TEST_ASSERT_EQUAL(SensorError::Invalid_Reading, latest.forDisplay(3000).lastError);
}

void test_no_received_sample_is_blank_even_before_freshness_expires() {
  const sensorsData initial;
  const sensorsData displayed = initial.forDisplay(0);
  TEST_ASSERT_TRUE(std::isnan(displayed.O2Level.percentage));
  TEST_ASSERT_TRUE(std::isnan(displayed.O2Level.millivolts));
  TEST_ASSERT_TRUE(std::isnan(displayed.CoLevel.ppm));
  TEST_ASSERT_TRUE(std::isnan(displayed.CoLevel.millivolts));
  TEST_ASSERT_TRUE(std::isnan(displayed.HeLevel.percentage));
  TEST_ASSERT_TRUE(std::isnan(displayed.HeLevel.millivolts));
  TEST_ASSERT_TRUE(std::isnan(displayed.HeTemperature));
}

void test_conversion_timeout_does_not_fetch_or_publish_counts() {
  Adafruit_ADS1115 adc;
  adc.conversionCompletes = false;
  adc.counts = 6400;
  COSensor sensor(3, adc);
  bool timedOut = false;
  const COReading reading = sensor.readLevel(&timedOut);
  TEST_ASSERT_TRUE(timedOut);
  TEST_ASSERT_TRUE(std::isnan(reading.millivolts));
  TEST_ASSERT_TRUE(std::isnan(reading.ppm));
  TEST_ASSERT_EQUAL_UINT32(25, nowMs);
  TEST_ASSERT_EQUAL_UINT(0, adc.resultReads);
  TEST_ASSERT_FALSE(adc.continuous);
}

void test_conversion_deadline_is_wrap_safe_and_rejects_late_result() {
  Adafruit_ADS1115 adc;
  nowMs = UINT32_MAX - 10;
  adc.completeAfterMs = 24;
  adc.counts = 320;
  int16_t result = 123;
  TEST_ASSERT_TRUE(acquisition::readCounts(adc, ADS1X15_REG_CONFIG_MUX_DIFF_2_3, result));
  TEST_ASSERT_EQUAL_INT16(320, result);
  adc.completeAfterMs = 25;
  TEST_ASSERT_FALSE(acquisition::readCounts(adc, ADS1X15_REG_CONFIG_MUX_DIFF_2_3, result));
  TEST_ASSERT_EQUAL_UINT(1, adc.resultReads);
  adc.completeAfterMs = 0;
  adc.resultDelayMs = 25;
  result = 123;
  TEST_ASSERT_FALSE(acquisition::readCounts(adc, ADS1X15_REG_CONFIG_MUX_DIFF_2_3, result));
  TEST_ASSERT_EQUAL_INT16(123, result);
}

void test_calibration_stops_on_first_conversion_timeout() {
  Adafruit_ADS1115 adc;
  adc.conversionCompletes = false;
  O2Sensor oxygen(adc);
  HESensor helium(adc);
  TEST_ASSERT_TRUE(std::isnan(oxygen.calibrate()));
  TEST_ASSERT_TRUE(std::isnan(helium.calibrate()));
  TEST_ASSERT_EQUAL_UINT(1, adc.differential23Reads);
  TEST_ASSERT_EQUAL_UINT(1, adc.differential01Reads);
  TEST_ASSERT_EQUAL_UINT(0, adc.resultReads);
  TEST_ASSERT_EQUAL_UINT32(50, nowMs);
}

void test_adc1_failure_does_not_prevent_adc2_initialization_or_co() {
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager manager(handle);
  Adafruit_ADS1115::present[0] = false;
  TEST_ASSERT_EQUAL(SensorError::ADC_Init_Failed, manager.init());
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[1]->beginCalls);
  TEST_ASSERT_EQUAL_UINT(10, Wire1.timeoutMs);
  manager.setSensorsConfig(true, true, false, 10, NAN, 621.2f);
  Adafruit_ADS1115::devices[1]->counts = 6400;
  manager.readSensors();
  sensorsData snapshot;
  xQueueReceive(handle, &snapshot, 0);
  TEST_ASSERT_TRUE(std::isnan(snapshot.O2Level.percentage));
  TEST_ASSERT_EQUAL_FLOAT(0, snapshot.CoLevel.ppm);
  TEST_ASSERT_EQUAL_UINT(0, Adafruit_ADS1115::devices[0]->differential23Reads);
}

void test_adc2_timeout_leaves_o2_working_and_skips_other_adc2_channels() {
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager manager(handle);
  manager.init();
  manager.setSensorsConfig(true, true, true, 10, NAN, 621.2f);
  Adafruit_ADS1115::devices[0]->counts = 320;
  Adafruit_ADS1115::devices[1]->conversionCompletes = false;
  TEST_ASSERT_EQUAL(SensorError::ADC_Timeout, manager.readSensors());
  sensorsData snapshot;
  xQueueReceive(handle, &snapshot, 0);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.9f, snapshot.O2Level.percentage);
  TEST_ASSERT_TRUE(std::isnan(snapshot.CoLevel.ppm));
  TEST_ASSERT_TRUE(std::isnan(snapshot.HeLevel.percentage));
  TEST_ASSERT_TRUE(std::isnan(snapshot.HeTemperature));
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[1]->singleEndedReads);
  TEST_ASSERT_EQUAL_UINT(0, Adafruit_ADS1115::devices[1]->differential01Reads);
  TEST_ASSERT_EQUAL_UINT(0, Adafruit_ADS1115::devices[1]->resultReads);
  nowMs += 500;
  manager.readSensors();
  TEST_ASSERT_EQUAL_UINT(2, Adafruit_ADS1115::devices[0]->differential23Reads);
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[1]->singleEndedReads);
}

void test_recovery_is_per_device_delayed_and_wrap_safe() {
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager manager(handle);
  nowMs = UINT32_MAX - 500;
  Adafruit_ADS1115::present[0] = false;
  manager.init();
  manager.setSensorsConfig(true, false, false, 10, NAN, 621.2f);
  nowMs += 999;
  manager.readSensors();
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[0]->beginCalls);
  nowMs += 1;
  manager.readSensors();
  TEST_ASSERT_EQUAL_UINT(2, Adafruit_ADS1115::devices[0]->beginCalls);
  Adafruit_ADS1115::present[0] = true;
  Adafruit_ADS1115::devices[0]->counts = 320;
  nowMs += 1000;
  TEST_ASSERT_EQUAL(SensorError::None, manager.readSensors());
  TEST_ASSERT_EQUAL_UINT(3, Adafruit_ADS1115::devices[0]->beginCalls);
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[1]->beginCalls);
  TEST_ASSERT_EQUAL(RATE_ADS1115_128SPS, Adafruit_ADS1115::devices[0]->dataRate);
  sensorsData snapshot;
  xQueueReceive(handle, &snapshot, 0);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.9f, snapshot.O2Level.percentage);
}

void test_disabled_device_is_not_retried_and_invalid_values_do_not_reinitialize() {
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager manager(handle);
  Adafruit_ADS1115::present[1] = false;
  manager.init();
  manager.setSensorsConfig(true, false, false, 10, NAN, 621.2f);
  Adafruit_ADS1115::devices[0]->counts = 6400;
  for (unsigned cycle = 0; cycle < 20; ++cycle) {
    nowMs += 1000;
    TEST_ASSERT_EQUAL(SensorError::Invalid_Reading, manager.readSensors());
  }
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[0]->beginCalls);
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[1]->beginCalls);
}

void test_timed_out_calibration_keeps_previous_coefficient_after_recovery() {
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager manager(handle);
  manager.init();
  manager.setSensorsConfig(true, false, false, 10, NAN, 621.2f);
  Adafruit_ADS1115::devices[0]->conversionCompletes = false;
  TEST_ASSERT_TRUE(std::isnan(manager.calibrateO2_21()));
  TEST_ASSERT_TRUE(std::isnan(manager.calibrateO2_100()));
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[0]->differential23Reads);
  Adafruit_ADS1115::devices[0]->conversionCompletes = true;
  Adafruit_ADS1115::devices[0]->counts = 320;
  nowMs += 1000;
  TEST_ASSERT_EQUAL(SensorError::None, manager.readSensors());
  sensorsData snapshot;
  xQueueReceive(handle, &snapshot, 0);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.9f, snapshot.O2Level.percentage);
}

void test_calibration_request_retries_offline_device_without_acquisition() {
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager manager(handle);
  Adafruit_ADS1115::present[0] = false;
  Adafruit_ADS1115::present[1] = false;
  manager.init();
  TEST_ASSERT_TRUE(std::isnan(manager.calibrateO2_21()));
  TEST_ASSERT_TRUE(std::isnan(manager.calibrateHe_100()));
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[0]->beginCalls);
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[1]->beginCalls);
  Adafruit_ADS1115::present[0] = true;
  Adafruit_ADS1115::present[1] = true;
  Adafruit_ADS1115::devices[0]->counts = 320;
  Adafruit_ADS1115::devices[1]->counts = 9600;
  nowMs += 1000;
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 10, manager.calibrateO2_21());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 600, manager.calibrateHe_100());
  TEST_ASSERT_EQUAL_UINT(2, Adafruit_ADS1115::devices[0]->beginCalls);
  TEST_ASSERT_EQUAL_UINT(2, Adafruit_ADS1115::devices[1]->beginCalls);
}

void test_partial_calibration_timeout_keeps_pure_o2_and_he_coefficients() {
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager manager(handle);
  manager.init();
  manager.setSensorsConfig(true, false, true, 10, 50, 621.2f);
  auto* oxygen = Adafruit_ADS1115::devices[0];
  auto* helium = Adafruit_ADS1115::devices[1];
  oxygen->counts = 960;
  helium->counts = 1600;
  oxygen->successfulConversions = 3;
  helium->successfulConversions = 3;
  TEST_ASSERT_TRUE(std::isnan(manager.calibrateO2_100()));
  TEST_ASSERT_TRUE(std::isnan(manager.calibrateHe_100()));
  TEST_ASSERT_EQUAL_UINT(4, oxygen->differential23Reads);
  TEST_ASSERT_EQUAL_UINT(4, helium->differential01Reads);
  TEST_ASSERT_EQUAL_UINT(3, oxygen->resultReads);
  TEST_ASSERT_EQUAL_UINT(3, helium->resultReads);
  oxygen->successfulConversions = UINT32_MAX;
  helium->successfulConversions = UINT32_MAX;
  nowMs += 1000;
  TEST_ASSERT_EQUAL(SensorError::None, manager.readSensors());
  sensorsData snapshot;
  xQueueReceive(handle, &snapshot, 0);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 60.45f, snapshot.O2Level.percentage);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, conversions::hePercentage(89, 621.2f), snapshot.HeLevel.percentage);
}

void test_timeout_error_takes_precedence_over_invalid_and_offline_channels() {
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager manager(handle);
  Adafruit_ADS1115::present[1] = false;
  manager.init();
  manager.setSensorsConfig(true, true, false, 10, NAN, 621.2f);
  Adafruit_ADS1115::devices[0]->counts = 6400;
  TEST_ASSERT_EQUAL(SensorError::ADC_Init_Failed, manager.readSensors());
  Adafruit_ADS1115::devices[0]->conversionCompletes = false;
  TEST_ASSERT_EQUAL(SensorError::ADC_Timeout, manager.readSensors());
  TEST_ASSERT_EQUAL(SensorError::ADC_Init_Failed, manager.readSensors());
}

void test_invalid_single_ended_channel_does_not_issue_conversion() {
  Adafruit_ADS1115 adc;
  COSensor carbonMonoxide(4, adc);
  TempSensor temperature(255, adc);
  bool timedOut = true;
  TEST_ASSERT_TRUE(std::isnan(carbonMonoxide.readLevel(&timedOut).ppm));
  TEST_ASSERT_FALSE(timedOut);
  TEST_ASSERT_TRUE(std::isnan(temperature.readLevel(&timedOut)));
  TEST_ASSERT_FALSE(timedOut);
  TEST_ASSERT_EQUAL_UINT(0, adc.singleEndedReads);
}

void test_channel_states_distinguish_disabled_invalid_and_unavailable() {
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager manager(handle);
  manager.init();
  manager.setSensorsConfig(true, false, true, 10, NAN, 621.2f);
  Adafruit_ADS1115::devices[0]->counts = 6400;
  Adafruit_ADS1115::devices[1]->counts = 1600;
  manager.readSensors();
  sensorsData snapshot;
  xQueueReceive(handle, &snapshot, 0);
  TEST_ASSERT_EQUAL(ChannelState::Invalid, snapshot.o2State);
  TEST_ASSERT_EQUAL(ChannelState::Disabled, snapshot.coState);
  TEST_ASSERT_EQUAL(ChannelState::Invalid, snapshot.heState);
  TEST_ASSERT_EQUAL(ChannelState::Valid, snapshot.temperatureState);

  Adafruit_ADS1115::devices[0]->counts = 320;
  Adafruit_ADS1115::devices[1]->conversionCompletes = false;
  manager.readSensors();
  xQueueReceive(handle, &snapshot, 0);
  TEST_ASSERT_EQUAL(ChannelState::Valid, snapshot.o2State);
  TEST_ASSERT_EQUAL(ChannelState::Unavailable, snapshot.heState);
  TEST_ASSERT_EQUAL(ChannelState::Unavailable, snapshot.temperatureState);
  TEST_ASSERT_EQUAL(ChannelState::Disabled, snapshot.coState);

  Adafruit_ADS1115::devices[1]->conversionCompletes = true;
  nowMs += 1000;
  manager.readSensors();
  xQueueReceive(handle, &snapshot, 0);
  TEST_ASSERT_EQUAL(ChannelState::Valid, snapshot.heState);
  TEST_ASSERT_EQUAL(ChannelState::Valid, snapshot.temperatureState);
}

void test_stale_copy_keeps_disabled_distinct_and_leaves_producer_states_unchanged() {
  sensorsData snapshot;
  snapshot.timestampMs = UINT32_MAX - 500;
  snapshot.o2State = ChannelState::Valid;
  snapshot.coState = ChannelState::Disabled;
  snapshot.heState = ChannelState::Invalid;
  snapshot.temperatureState = ChannelState::Unavailable;
  const sensorsData fresh = snapshot.forDisplay(1298);
  TEST_ASSERT_EQUAL(ChannelState::Valid, fresh.o2State);
  const sensorsData stale = snapshot.forDisplay(1299);
  TEST_ASSERT_EQUAL(ChannelState::Stale, stale.o2State);
  TEST_ASSERT_EQUAL(ChannelState::Disabled, stale.coState);
  TEST_ASSERT_EQUAL(ChannelState::Stale, stale.heState);
  TEST_ASSERT_EQUAL(ChannelState::Stale, stale.temperatureState);
  TEST_ASSERT_EQUAL(ChannelState::Valid, snapshot.o2State);
  TEST_ASSERT_EQUAL(ChannelState::Unavailable, sensorsData{}.o2State);
}

void test_channel_state_labels_are_distinct() {
  TEST_ASSERT_EQUAL_STRING("Off", channelStateText(ChannelState::Disabled));
  TEST_ASSERT_EQUAL_STRING("Invalid", channelStateText(ChannelState::Invalid));
  TEST_ASSERT_EQUAL_STRING("Unavailable", channelStateText(ChannelState::Unavailable));
  TEST_ASSERT_EQUAL_STRING("Stale", channelStateText(ChannelState::Stale));
  TEST_ASSERT_EQUAL_STRING("", channelStateText(ChannelState::Valid));
}

void test_status_adapter_overrides_generated_text_and_restores_fonts() {
  const lv_font_t smallFont{16};
  const lv_font_t largeFont{48};
  lv_obj_t oxygen{"O2 %", &smallFont};
  lv_obj_t carbonMonoxide{"CO ppm", &smallFont};
  lv_obj_t helium{"He %", &smallFont};
  lv_obj_t largeOxygen{"O2 %", &largeFont};
  objects = {&oxygen, &carbonMonoxide, &helium, &largeOxygen};
  sensorsData data;
  data.o2State = ChannelState::Stale;
  data.coState = ChannelState::Unavailable;
  data.heState = ChannelState::Invalid;
  applyReadingStatus(data);
  TEST_ASSERT_EQUAL_STRING("O2: Stale", oxygen.text.c_str());
  TEST_ASSERT_EQUAL_STRING("CO: Unavailable", carbonMonoxide.text.c_str());
  TEST_ASSERT_EQUAL_STRING("He: Invalid", helium.text.c_str());
  TEST_ASSERT_EQUAL_STRING("O2: Stale", largeOxygen.text.c_str());
  TEST_ASSERT_EQUAL_PTR(&ui_font_geneva16, largeOxygen.font);

  largeOxygen.text = "O2 %";
  applyReadingStatus(data);
  TEST_ASSERT_EQUAL_STRING("O2: Stale", largeOxygen.text.c_str());
  data.o2State = data.coState = data.heState = data.temperatureState = ChannelState::Valid;
  oxygen.text = "O2 20.9%";
  carbonMonoxide.text = "CO 0 ppm";
  helium.text = "He 14.4%";
  largeOxygen.text = "O2 20.9%";
  applyReadingStatus(data);
  TEST_ASSERT_EQUAL_STRING("O2 20.9%", oxygen.text.c_str());
  TEST_ASSERT_EQUAL_STRING("CO 0 ppm", carbonMonoxide.text.c_str());
  TEST_ASSERT_EQUAL_STRING("He 14.4%", helium.text.c_str());
  TEST_ASSERT_EQUAL_STRING("O2 20.9%", largeOxygen.text.c_str());
  TEST_ASSERT_EQUAL_PTR(&largeFont, largeOxygen.font);
  TEST_ASSERT_EQUAL_PTR(&smallFont, oxygen.font);

  data.HeLevel.percentage = 14.4f;
  data.temperatureState = ChannelState::Unavailable;
  applyReadingStatus(data);
  TEST_ASSERT_EQUAL_STRING("He 14.4%, T: Unavailable", helium.text.c_str());
  data.coState = ChannelState::Disabled;
  applyReadingStatus(data);
  TEST_ASSERT_EQUAL_STRING("CO: Off", carbonMonoxide.text.c_str());
  objects = {};
  applyReadingStatus(data);
}

void test_disabled_states_survive_stale_presentation_and_zero_co_is_valid() {
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager manager(handle);
  manager.init();
  manager.readSensors();
  sensorsData snapshot;
  xQueueReceive(handle, &snapshot, 0);
  const sensorsData disabled = snapshot.forDisplay(5000);
  TEST_ASSERT_EQUAL(ChannelState::Disabled, disabled.o2State);
  TEST_ASSERT_EQUAL(ChannelState::Disabled, disabled.coState);
  TEST_ASSERT_EQUAL(ChannelState::Disabled, disabled.heState);
  TEST_ASSERT_EQUAL(ChannelState::Disabled, disabled.temperatureState);
  manager.setSensorsConfig(false, true, false, 10, NAN, 621.2f);
  Adafruit_ADS1115::devices[1]->counts = 6400;
  manager.readSensors();
  xQueueReceive(handle, &snapshot, 0);
  TEST_ASSERT_EQUAL(ChannelState::Valid, snapshot.coState);
  TEST_ASSERT_EQUAL_FLOAT(0, snapshot.CoLevel.ppm);
}

void test_temperature_timeout_keeps_completed_he_read_valid() {
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager manager(handle);
  manager.init();
  manager.setSensorsConfig(true, false, true, 10, NAN, 621.2f);
  Adafruit_ADS1115::devices[0]->counts = 320;
  Adafruit_ADS1115::devices[1]->counts = 1600;
  Adafruit_ADS1115::devices[1]->successfulConversions = 1;
  manager.readSensors();
  sensorsData snapshot;
  xQueueReceive(handle, &snapshot, 0);
  TEST_ASSERT_EQUAL(ChannelState::Valid, snapshot.o2State);
  TEST_ASSERT_EQUAL(ChannelState::Valid, snapshot.heState);
  TEST_ASSERT_EQUAL(ChannelState::Unavailable, snapshot.temperatureState);
  TEST_ASSERT_TRUE(std::isfinite(snapshot.HeLevel.percentage));
  TEST_ASSERT_TRUE(std::isnan(snapshot.HeTemperature));
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
  RUN_TEST(test_slow_consumer_receives_only_latest_measurement);
  RUN_TEST(test_stopped_producer_blanks_all_numbers_without_new_sample);
  RUN_TEST(test_freshness_survives_clock_wrap_and_new_sample_restores_display);
  RUN_TEST(test_disabled_and_invalid_cycles_are_timestamped_without_reviving_numbers);
  RUN_TEST(test_no_received_sample_is_blank_even_before_freshness_expires);
  RUN_TEST(test_conversion_timeout_does_not_fetch_or_publish_counts);
  RUN_TEST(test_conversion_deadline_is_wrap_safe_and_rejects_late_result);
  RUN_TEST(test_calibration_stops_on_first_conversion_timeout);
  RUN_TEST(test_adc1_failure_does_not_prevent_adc2_initialization_or_co);
  RUN_TEST(test_adc2_timeout_leaves_o2_working_and_skips_other_adc2_channels);
  RUN_TEST(test_recovery_is_per_device_delayed_and_wrap_safe);
  RUN_TEST(test_disabled_device_is_not_retried_and_invalid_values_do_not_reinitialize);
  RUN_TEST(test_timed_out_calibration_keeps_previous_coefficient_after_recovery);
  RUN_TEST(test_calibration_request_retries_offline_device_without_acquisition);
  RUN_TEST(test_partial_calibration_timeout_keeps_pure_o2_and_he_coefficients);
  RUN_TEST(test_timeout_error_takes_precedence_over_invalid_and_offline_channels);
  RUN_TEST(test_invalid_single_ended_channel_does_not_issue_conversion);
  RUN_TEST(test_channel_states_distinguish_disabled_invalid_and_unavailable);
  RUN_TEST(test_stale_copy_keeps_disabled_distinct_and_leaves_producer_states_unchanged);
  RUN_TEST(test_channel_state_labels_are_distinct);
  RUN_TEST(test_status_adapter_overrides_generated_text_and_restores_fonts);
  RUN_TEST(test_disabled_states_survive_stale_presentation_and_zero_co_is_valid);
  RUN_TEST(test_temperature_timeout_keeps_completed_he_read_valid);
  return UNITY_END();
}