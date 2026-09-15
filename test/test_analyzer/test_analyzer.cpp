#include <unity.h>
#include "app/Analyzer.h"
#include "pin_config.h"

void setUp() {
  Preferences::values.clear();
  Preferences::values["calib_start"] = 0;
  Preferences::available = true;
  Preferences::failWrite = false;
  Preferences::writes = 0;
  Preferences::reads = 0;
  Preferences::failAfter = -1;
  nowMs = 0;
  Adafruit_ADS1115::present[0] = Adafruit_ADS1115::present[1] = true;
}
void tearDown() {}

struct Rig {
  FakeQueue queue{sizeof(sensorsData)};
  QueueHandle_t handle = &queue;
  SensorManager sensors{handle};
  SettingsStore settingsStore;
  app::Analyzer analyzer{settingsStore, sensors};
  Rig() { analyzer.begin(); }
};

void test_apply_settings_acknowledges_live_state_and_generation() {
  Rig rig;
  app::Command command;
  command.id = 17;
  command.settings = rig.analyzer.effective();
  command.settings.coEnabled = false;
  command.settings.o2Air = 40;
  const auto result = rig.analyzer.execute(command);
  TEST_ASSERT_EQUAL_UINT32(17, result.id);
  TEST_ASSERT_EQUAL(app::Failure::None, result.failure);
  TEST_ASSERT_FALSE(result.effective.coEnabled);
  TEST_ASSERT_EQUAL_FLOAT(10, result.effective.o2Air);
  TEST_ASSERT_EQUAL_INT(LOW, pinValues[PIN_CO_ENABLE]);
  rig.analyzer.measure();
  sensorsData sample;
  xQueueReceive(rig.handle, &sample, 0);
  TEST_ASSERT_EQUAL_UINT32(result.generation, sample.generation);
  TEST_ASSERT_EQUAL(ChannelState::Disabled, sample.coState);
  TEST_ASSERT_EQUAL_UINT32(2, result.generation);
}

void test_rejection_and_write_failure_keep_effective_state() {
  Rig rig;
  app::Command command;
  command.settings = rig.analyzer.effective();
  command.settings.po2Bottom = 1.6f;
  command.settings.po2Deco = 1.4f;
  TEST_ASSERT_EQUAL(app::Failure::Invalid, rig.analyzer.execute(command).failure);
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
  command.settings = rig.analyzer.effective();
  command.settings.heEnabled = false;
  Preferences::failWrite = true;
  const auto result = rig.analyzer.execute(command);
  TEST_ASSERT_EQUAL(app::Failure::Storage, result.failure);
  TEST_ASSERT_TRUE(result.effective.heEnabled);
  TEST_ASSERT_EQUAL_INT(HIGH, pinValues[PIN_HE_ENABLE]);
  TEST_ASSERT_EQUAL_UINT32(1, result.generation);
}

void test_calibration_failure_restores_live_coefficients() {
  Rig rig;
  Adafruit_ADS1115::devices[0]->counts = 640;
  app::Command command;
  command.type = app::CommandType::CalibrateAir;
  Preferences::failWrite = true;
  const auto result = rig.analyzer.execute(command);
  TEST_ASSERT_EQUAL(app::Failure::Storage, result.failure);
  TEST_ASSERT_EQUAL_FLOAT(10, result.effective.o2Air);
  Adafruit_ADS1115::devices[0]->counts = 320;
  rig.analyzer.measure();
  sensorsData sample;
  xQueueReceive(rig.handle, &sample, 0);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.9f, sample.O2Level.percentage);
}

void test_clear_optional_calibration_and_unchanged_commands() {
  Preferences::values["o2_calib_100"] = 50;
  Rig rig;
  app::Command command;
  command.type = app::CommandType::ClearPure;
  const auto result = rig.analyzer.execute(command);
  TEST_ASSERT_EQUAL(app::Failure::None, result.failure);
  TEST_ASSERT_TRUE(std::isnan(result.effective.o2Pure));
  TEST_ASSERT_TRUE(std::isnan(Preferences::values["o2_calib_100"]));
  const auto writes = Preferences::writes;
  const auto repeated = rig.analyzer.execute(command);
  TEST_ASSERT_EQUAL_UINT32(result.generation, repeated.generation);
  TEST_ASSERT_EQUAL_UINT(writes, Preferences::writes);
}

void test_brightness_does_not_invalidate_measurements() {
  Rig rig;
  app::Command command;
  command.settings = rig.analyzer.effective();
  command.settings.brightness = 64;
  const auto result = rig.analyzer.execute(command);
  TEST_ASSERT_EQUAL_UINT32(1, result.generation);
  sensorsData old;
  old.O2Level = {10, 20.9f};
  TEST_ASSERT_TRUE(std::isnan(old.forDisplay(0, 2).O2Level.percentage));
  TEST_ASSERT_EQUAL_FLOAT(20.9f, old.forDisplay(0, 1).O2Level.percentage);
}

void test_startup_barrier_single_outstanding_and_matching_results() {
  app::UiState state;
  FakeQueue queue(sizeof(app::Command));
  app::Command command;
  TEST_ASSERT_FALSE(state.submit(command, &queue));
  TEST_ASSERT_EQUAL_UINT(0, queue.sends);
  app::Result startup;
  TEST_ASSERT_TRUE(state.accept(startup));
  TEST_ASSERT_FALSE(state.busy());
  TEST_ASSERT_TRUE(state.submit(command, &queue));
  const uint32_t pending = state.pendingId;
  TEST_ASSERT_FALSE(state.submit(command, &queue));
  app::Result result;
  result.type = app::CommandType::ApplySettings;
  result.id = pending + 1;
  TEST_ASSERT_FALSE(state.accept(result));
  TEST_ASSERT_TRUE(state.busy());
  result.id = pending;
  result.failure = app::Failure::Storage;
  TEST_ASSERT_TRUE(state.accept(result));
  TEST_ASSERT_FALSE(state.busy());
  TEST_ASSERT_FALSE(state.accept(result));
  TEST_ASSERT_FALSE(state.accept(startup));
}

void test_full_command_queue_does_not_create_pending_operation() {
  app::UiState state;
  state.accept(app::Result{});
  FakeQueue queue(sizeof(app::Command));
  queue.occupied = true;
  TEST_ASSERT_FALSE(state.submit(app::Command{}, &queue));
  TEST_ASSERT_FALSE(state.busy());
  queue.occupied = false;
  state.nextId = UINT32_MAX;
  TEST_ASSERT_TRUE(state.submit(app::Command{}, &queue));
  TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, state.pendingId);
  TEST_ASSERT_EQUAL_UINT32(1, state.nextId);
}

void test_partial_write_is_reconciled_before_later_unchanged_success() {
  Rig rig;
  app::Command command;
  command.settings = rig.analyzer.effective();
  command.settings.coEnabled = false;
  command.settings.heEnabled = false;
  Preferences::failAfter = 1;
  TEST_ASSERT_EQUAL(app::Failure::Storage, rig.analyzer.execute(command).failure);
  TEST_ASSERT_EQUAL_FLOAT(0, Preferences::values["co_enabled"]);
  TEST_ASSERT_TRUE(rig.analyzer.effective().coEnabled);
  Preferences::failAfter = -1;
  command.settings = rig.analyzer.effective();
  TEST_ASSERT_EQUAL(app::Failure::None, rig.analyzer.execute(command).failure);
  TEST_ASSERT_EQUAL_FLOAT(1, Preferences::values["co_enabled"]);
  TEST_ASSERT_EQUAL_FLOAT(1, Preferences::values["he_enabled"]);
}

void test_routine_commands_and_reads_do_not_reread_preferences() {
  Rig rig;
  const auto reads = Preferences::reads;
  app::Command command;
  command.settings = rig.analyzer.effective();
  rig.analyzer.execute(command);
  rig.analyzer.measure();
  TEST_ASSERT_EQUAL_UINT(reads, Preferences::reads);
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
}

void test_invalid_load_is_visible_and_startup_does_not_calibrate_defaults() {
  Preferences::values["o2_calib_21"] = 0;
  Preferences::values["calib_start"] = 1;
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager sensors(handle);
  SettingsStore settingsStore;
  app::Analyzer analyzer(settingsStore, sensors);
  const auto result = analyzer.begin();
  TEST_ASSERT_EQUAL(app::CommandType::Startup, result.type);
  TEST_ASSERT_EQUAL(app::Failure::LoadedDefaults, result.failure);
  TEST_ASSERT_TRUE(result.effective.valid());
  TEST_ASSERT_EQUAL_UINT(0, Adafruit_ADS1115::devices[0]->differential23Reads);
}

void test_startup_reports_adc_failure_without_blocking_other_device() {
  Preferences::values["calib_start"] = 1;
  Adafruit_ADS1115::present[0] = false;
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager sensors(handle);
  SettingsStore settingsStore;
  app::Analyzer analyzer(settingsStore, sensors);
  const auto result = analyzer.begin();
  TEST_ASSERT_EQUAL(app::Failure::Sampling, result.failure);
  TEST_ASSERT_EQUAL(SensorError::ADC_Init_Failed, result.sensorError);
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[1]->beginCalls);
  app::UiState state;
  TEST_ASSERT_TRUE(state.accept(result));
  TEST_ASSERT_FALSE(state.busy());
}

void test_full_result_path_retains_outcome_without_blocking_measurements() {
  Rig rig;
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  app::UiState ui;
  rig.analyzer.service(&commands, &results);
  app::Result startup;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(&results, &startup, 0));
  TEST_ASSERT_TRUE(ui.accept(startup));
  app::Command command;
  command.settings = ui.effective;
  command.settings.coEnabled = false;
  TEST_ASSERT_TRUE(ui.submit(command, &commands));
  TEST_ASSERT_TRUE(rig.analyzer.service(&commands, &results));
  const uint32_t requestedId = ui.pendingId;
  results.occupied = true;
  for (unsigned pass = 0; pass < 4; ++pass) {
    TEST_ASSERT_FALSE(rig.analyzer.service(&commands, &results));
    rig.analyzer.measure();
    TEST_ASSERT_TRUE(ui.busy());
  }
  TEST_ASSERT_EQUAL_UINT(4, rig.queue.sends);
  results.occupied = false;
  rig.analyzer.service(&commands, &results);
  app::Result delivered;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(&results, &delivered, 0));
  TEST_ASSERT_EQUAL_UINT32(requestedId, delivered.id);
  TEST_ASSERT_FALSE(delivered.effective.coEnabled);
  TEST_ASSERT_TRUE(ui.accept(delivered));
  TEST_ASSERT_FALSE(ui.busy());
  rig.analyzer.service(&commands, &results);
  TEST_ASSERT_FALSE(results.occupied);
}

void test_successful_calibration_then_reset_changes_live_and_stored_values() {
  Rig rig;
  Adafruit_ADS1115::devices[0]->counts = 640;
  app::Command command;
  command.type = app::CommandType::CalibrateAir;
  const auto calibrated = rig.analyzer.execute(command);
  TEST_ASSERT_EQUAL(app::Failure::None, calibrated.failure);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20, calibrated.effective.o2Air);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20, Preferences::values["o2_calib_21"]);
  command.type = app::CommandType::ResetAir;
  const auto reset = rig.analyzer.execute(command);
  TEST_ASSERT_EQUAL(app::Failure::None, reset.failure);
  TEST_ASSERT_EQUAL_FLOAT(10, reset.effective.o2Air);
  TEST_ASSERT_EQUAL_FLOAT(10, Preferences::values["o2_calib_21"]);
  TEST_ASSERT_EQUAL_UINT32(calibrated.generation + 1, reset.generation);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_apply_settings_acknowledges_live_state_and_generation);
  RUN_TEST(test_rejection_and_write_failure_keep_effective_state);
  RUN_TEST(test_calibration_failure_restores_live_coefficients);
  RUN_TEST(test_clear_optional_calibration_and_unchanged_commands);
  RUN_TEST(test_brightness_does_not_invalidate_measurements);
  RUN_TEST(test_startup_barrier_single_outstanding_and_matching_results);
  RUN_TEST(test_full_command_queue_does_not_create_pending_operation);
  RUN_TEST(test_partial_write_is_reconciled_before_later_unchanged_success);
  RUN_TEST(test_routine_commands_and_reads_do_not_reread_preferences);
  RUN_TEST(test_invalid_load_is_visible_and_startup_does_not_calibrate_defaults);
  RUN_TEST(test_startup_reports_adc_failure_without_blocking_other_device);
  RUN_TEST(test_full_result_path_retains_outcome_without_blocking_measurements);
  RUN_TEST(test_successful_calibration_then_reset_changes_live_and_stored_values);
  return UNITY_END();
}