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
  Preferences::values["brightness"] = 64;
  Preferences::values["he_enabled"] = 0;
  Preferences::values["po2_max_bottom"] = 1.2f;
  Preferences::values["po2_max_deco"] = 1.5f;
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager sensors(handle);
  SettingsStore settingsStore;
  app::Analyzer analyzer(settingsStore, sensors);
  const auto result = analyzer.begin();
  TEST_ASSERT_EQUAL(app::CommandType::Startup, result.type);
  TEST_ASSERT_EQUAL(app::Failure::LoadedDefaults, result.failure);
  TEST_ASSERT_TRUE(result.effective.valid());
  TEST_ASSERT_EQUAL_UINT8(64, result.effective.brightness);
  TEST_ASSERT_FALSE(result.effective.heEnabled);
  TEST_ASSERT_EQUAL_FLOAT(1.2f, result.effective.po2Bottom);
  TEST_ASSERT_EQUAL_FLOAT(1.5f, result.effective.po2Deco);
  TEST_ASSERT_EQUAL_UINT(0, Adafruit_ADS1115::devices[0]->differential23Reads);
}

void test_valid_settings_repair_is_a_noop() {
  AnalyzerSettings value;
  value.o2Enabled = false;
  value.coEnabled = false;
  value.heEnabled = false;
  value.calibrateOnStart = false;
  value.brightness = 8;
  value.sleepMinutes = 0;
  value.po2Bottom = 1;
  value.po2Deco = 2;
  value.o2Air = 50;
  value.o2Pure = 100;
  value.heCalibration = 700;
  const auto previous = value;
  TEST_ASSERT_FALSE(value.repairInvalidFields());
  TEST_ASSERT_TRUE(value.sameMeasurementSettings(previous));
  TEST_ASSERT_FALSE(value.calibrateOnStart);
  TEST_ASSERT_EQUAL_UINT8(8, value.brightness);
  TEST_ASSERT_EQUAL_UINT8(0, value.sleepMinutes);
  TEST_ASSERT_EQUAL_FLOAT(1, value.po2Bottom);
  TEST_ASSERT_EQUAL_FLOAT(2, value.po2Deco);
}

void test_po2_repair_preserves_valid_limits_and_calibration() {
  const struct { float bottom, deco, expectedBottom, expectedDeco; } cases[] = {
    {NAN, 1.2f, 1.2f, 1.2f}, {INFINITY, 1.8f, 1.4f, 1.8f},
    {0.9f, 1.5f, 1.4f, 1.5f}, {1.7f, 1.6f, 1.4f, 1.6f},
    {1.2f, NAN, 1.2f, 1.6f}, {1.3f, INFINITY, 1.3f, 1.6f},
    {1.5f, 1.2f, 1.5f, 1.5f}, {1.6f, 2.1f, 1.6f, 1.6f},
    {-INFINITY, -INFINITY, 1.4f, 1.6f}
  };
  for (const auto& entry : cases) {
    AnalyzerSettings value;
    value.brightness = 64;
    value.sleepMinutes = 30;
    value.o2Air = 12;
    value.o2Pure = 55;
    value.heCalibration = 700;
    const auto previous = value;
    value.po2Bottom = entry.bottom;
    value.po2Deco = entry.deco;
    TEST_ASSERT_TRUE(value.repairInvalidFields());
    TEST_ASSERT_TRUE(value.valid());
    TEST_ASSERT_EQUAL_FLOAT(entry.expectedBottom, value.po2Bottom);
    TEST_ASSERT_EQUAL_FLOAT(entry.expectedDeco, value.po2Deco);
    TEST_ASSERT_TRUE(value.sameMeasurementSettings(previous));
    TEST_ASSERT_EQUAL_UINT8(64, value.brightness);
    TEST_ASSERT_EQUAL_UINT8(30, value.sleepMinutes);
  }
}

void test_calibration_repair_is_limited_to_invalid_fields_and_dependencies() {
  const struct { float air, pure, helium, expectedAir, expectedPure, expectedHelium; } cases[] = {
    {NAN, 55, 700, 10, NAN, 700}, {INFINITY, 55, 700, 10, NAN, 700},
    {4.9f, 55, 700, 10, NAN, 700}, {50.1f, 55, 700, 10, NAN, 700},
    {12, 12, 700, 12, NAN, 700}, {12, INFINITY, 700, 12, NAN, 700},
    {12, 101, 700, 12, NAN, 700}, {12, -INFINITY, 700, 12, NAN, 700},
    {12, 55, 0, 12, 55, 620}, {12, 55, NAN, 12, 55, 620},
    {12, 55, INFINITY, 12, 55, 620}, {12, 55, -1, 12, 55, 620}
  };
  for (const auto& entry : cases) {
    AnalyzerSettings value;
    value.o2Air = entry.air;
    value.o2Pure = entry.pure;
    value.heCalibration = entry.helium;
    value.coEnabled = false;
    value.po2Bottom = 1.2f;
    value.brightness = 64;
    TEST_ASSERT_TRUE(value.repairInvalidFields());
    TEST_ASSERT_TRUE(value.valid());
    TEST_ASSERT_EQUAL_FLOAT(entry.expectedAir, value.o2Air);
    if (std::isnan(entry.expectedPure)) TEST_ASSERT_TRUE(std::isnan(value.o2Pure));
    else TEST_ASSERT_EQUAL_FLOAT(entry.expectedPure, value.o2Pure);
    TEST_ASSERT_EQUAL_FLOAT(entry.expectedHelium, value.heCalibration);
    TEST_ASSERT_FALSE(value.coEnabled);
    TEST_ASSERT_EQUAL_FLOAT(1.2f, value.po2Bottom);
    TEST_ASSERT_EQUAL_UINT8(64, value.brightness);
  }
}

void test_repaired_load_defers_writes_and_retries_full_save() {
  Preferences::values["brightness"] = 0;
  Preferences::values["po2_max_bottom"] = 1.2f;
  Preferences::values["o2_calib_21"] = 12;
  Preferences::values["o2_calib_100"] = 55;
  Preferences::values["he_calib_100"] = 700;
  SettingsStore store;
  store.begin();
  const auto loaded = store.load();
  TEST_ASSERT_TRUE(store.loadedDefaults());
  TEST_ASSERT_TRUE(loaded.valid());
  TEST_ASSERT_EQUAL_UINT8(128, loaded.brightness);
  TEST_ASSERT_EQUAL_FLOAT(1.2f, loaded.po2Bottom);
  TEST_ASSERT_EQUAL_FLOAT(12, loaded.o2Air);
  TEST_ASSERT_EQUAL_FLOAT(55, loaded.o2Pure);
  TEST_ASSERT_EQUAL_FLOAT(700, loaded.heCalibration);
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
  const auto reads = Preferences::reads;
  Preferences::failAfter = 3;
  TEST_ASSERT_FALSE(store.save(loaded, loaded));
  TEST_ASSERT_EQUAL_FLOAT(0, Preferences::values["brightness"]);
  Preferences::failAfter = -1;
  TEST_ASSERT_TRUE(store.save(loaded, loaded));
  TEST_ASSERT_EQUAL_UINT(reads, Preferences::reads);
  const auto writes = Preferences::writes;
  TEST_ASSERT_TRUE(store.save(loaded, loaded));
  TEST_ASSERT_EQUAL_UINT(writes, Preferences::writes);
  SettingsStore restarted;
  restarted.begin();
  const auto reloaded = restarted.load();
  TEST_ASSERT_FALSE(restarted.loadedDefaults());
  TEST_ASSERT_TRUE(reloaded.sameMeasurementSettings(loaded));
  TEST_ASSERT_EQUAL_UINT8(loaded.brightness, reloaded.brightness);
  TEST_ASSERT_EQUAL_FLOAT(loaded.po2Bottom, reloaded.po2Bottom);
}

void test_repaired_wake_preserves_values_but_keeps_calibration_required() {
  Preferences::values["brightness"] = 0;
  Preferences::values["calib_start"] = 1;
  Preferences::values["o2_calib_21"] = 12;
  Preferences::values["he_calib_100"] = 700;
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager sensors(handle);
  SettingsStore store;
  app::Analyzer analyzer(store, sensors);
  const auto result = analyzer.begin(true);
  TEST_ASSERT_EQUAL(app::Failure::CalibrationRequired, result.failure);
  TEST_ASSERT_EQUAL_FLOAT(12, result.effective.o2Air);
  TEST_ASSERT_EQUAL_FLOAT(700, result.effective.heCalibration);
  TEST_ASSERT_EQUAL_UINT(0, Adafruit_ADS1115::devices[0]->differential23Reads);
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
  analyzer.measure();
  sensorsData sample;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(handle, &sample, 0));
  TEST_ASSERT_TRUE(std::isnan(sample.O2Level.percentage));
  TEST_ASSERT_TRUE(std::isnan(sample.HeLevel.percentage));
}

void test_invalid_timeout_alone_does_not_flag_general_settings_recovery() {
  Preferences::values["sleep_minutes"] = 3;
  Preferences::values["calib_start"] = 1;
  Preferences::values["brightness"] = 64;
  Preferences::values["o2_calib_21"] = 12;
  SettingsStore store;
  store.begin();
  const auto loaded = store.load();
  TEST_ASSERT_TRUE(loaded.valid());
  TEST_ASSERT_FALSE(store.loadedDefaults());
  TEST_ASSERT_TRUE(loaded.calibrateOnStart);
  TEST_ASSERT_EQUAL_UINT8(app::DEFAULT_SLEEP_MINUTES, loaded.sleepMinutes);
  TEST_ASSERT_EQUAL_UINT8(64, loaded.brightness);
  TEST_ASSERT_EQUAL_FLOAT(12, loaded.o2Air);
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
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

void test_co_warmup_skips_conversions_until_configured_deadline() {
  Rig rig;
  auto* oxygen = Adafruit_ADS1115::devices[0];
  auto* secondary = Adafruit_ADS1115::devices[1];
  oxygen->counts = 320;
  secondary->counts = 6400;
  TEST_ASSERT_TRUE(rig.analyzer.coWarming());
  nowMs = app::Analyzer::CO_STARTUP_MS - 1;
  rig.analyzer.measure();
  sensorsData sample;
  xQueueReceive(rig.handle, &sample, 0);
  TEST_ASSERT_EQUAL(ChannelState::Warming, sample.coState);
  TEST_ASSERT_TRUE(std::isnan(sample.CoLevel.ppm));
  TEST_ASSERT_TRUE(std::isnan(sample.CoLevel.millivolts));
  TEST_ASSERT_EQUAL(ChannelState::Valid, sample.o2State);
  TEST_ASSERT_EQUAL(ChannelState::Valid, sample.heState);
  TEST_ASSERT_EQUAL_UINT(1, secondary->singleEndedReads);
  nowMs = app::Analyzer::CO_STARTUP_MS;
  TEST_ASSERT_FALSE(rig.analyzer.coWarming());
  rig.analyzer.measure();
  xQueueReceive(rig.handle, &sample, 0);
  TEST_ASSERT_EQUAL(ChannelState::Valid, sample.coState);
  TEST_ASSERT_EQUAL_FLOAT(0, sample.CoLevel.ppm);
  TEST_ASSERT_EQUAL_UINT(3, secondary->singleEndedReads);
}

void test_co_reenable_restarts_warmup_across_clock_wrap() {
  Rig rig;
  app::Command command;
  command.settings = rig.analyzer.effective();
  command.settings.coEnabled = false;
  rig.analyzer.execute(command);
  TEST_ASSERT_FALSE(rig.analyzer.coWarming());
  nowMs = UINT32_MAX - 1000;
  command.settings.coEnabled = true;
  rig.analyzer.execute(command);
  TEST_ASSERT_TRUE(rig.analyzer.coWarming());
  nowMs += app::Analyzer::CO_STARTUP_MS - 1;
  command.settings.brightness = 64;
  rig.analyzer.execute(command);
  TEST_ASSERT_TRUE(rig.analyzer.coWarming());
  nowMs += 1;
  TEST_ASSERT_FALSE(rig.analyzer.coWarming());
}

void test_adc_recovery_does_not_restart_co_warmup() {
  Rig rig;
  auto* secondary = Adafruit_ADS1115::devices[1];
  nowMs = app::Analyzer::CO_STARTUP_MS;
  secondary->conversionCompletes = false;
  rig.analyzer.measure();
  TEST_ASSERT_FALSE(rig.analyzer.coWarming());
  secondary->conversionCompletes = true;
  secondary->counts = 6400;
  nowMs += 1000;
  rig.analyzer.measure();
  sensorsData sample;
  xQueueReceive(rig.handle, &sample, 0);
  TEST_ASSERT_EQUAL(ChannelState::Valid, sample.coState);
  TEST_ASSERT_FALSE(rig.analyzer.coWarming());
  TEST_ASSERT_EQUAL_UINT(2, secondary->beginCalls);
}

void test_prepare_stops_publication_and_resume_restores_power_with_fresh_generation() {
  Rig rig;
  nowMs = app::Analyzer::CO_STARTUP_MS;
  rig.analyzer.measure();
  TEST_ASSERT_TRUE(rig.queue.occupied);
  app::Command prepare;
  prepare.type = app::CommandType::PrepareSleep;
  prepare.id = 10;
  const auto prepared = rig.analyzer.execute(prepare);
  TEST_ASSERT_EQUAL(app::Failure::None, prepared.failure);
  TEST_ASSERT_TRUE(rig.analyzer.preparedForSleep());
  TEST_ASSERT_EQUAL_UINT32(10, prepared.id);
  TEST_ASSERT_FALSE(rig.queue.occupied);
  TEST_ASSERT_EQUAL_INT(LOW, pinValues[PIN_HE_ENABLE]);
  TEST_ASSERT_EQUAL_INT(LOW, pinValues[PIN_CO_ENABLE]);
  const auto sends = rig.queue.sends;
  rig.analyzer.measure();
  TEST_ASSERT_EQUAL_UINT(sends, rig.queue.sends);
  app::Command ordinary;
  TEST_ASSERT_EQUAL(app::Failure::Busy, rig.analyzer.execute(ordinary).failure);
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
  TEST_ASSERT_EQUAL_UINT32(prepared.generation, rig.analyzer.execute(prepare).generation);

  app::Command resume;
  resume.type = app::CommandType::Resume;
  resume.id = 11;
  TEST_ASSERT_EQUAL(app::Failure::Invalid, rig.analyzer.execute(resume).failure);
  TEST_ASSERT_TRUE(rig.analyzer.preparedForSleep());
  resume.id = prepare.id;
  const auto resumed = rig.analyzer.execute(resume);
  TEST_ASSERT_FALSE(rig.analyzer.preparedForSleep());
  TEST_ASSERT_EQUAL(app::Failure::None, resumed.failure);
  TEST_ASSERT_EQUAL_UINT32(prepared.generation + 1, resumed.generation);
  TEST_ASSERT_EQUAL_INT(HIGH, pinValues[PIN_HE_ENABLE]);
  TEST_ASSERT_EQUAL_INT(HIGH, pinValues[PIN_CO_ENABLE]);
  rig.analyzer.measure();
  sensorsData sample;
  xQueueReceive(rig.handle, &sample, 0);
  TEST_ASSERT_EQUAL(ChannelState::Warming, sample.coState);
  TEST_ASSERT_EQUAL_UINT32(resumed.generation, sample.generation);
  TEST_ASSERT_EQUAL_UINT32(resumed.generation, rig.analyzer.execute(resume).generation);
}

void test_prepare_timeout_resume_ignores_late_ack_and_survives_full_command_queue() {
  Rig rig;
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  app::UiState ui;
  rig.analyzer.service(&commands, &results);
  app::Result result;
  xQueueReceive(&results, &result, 0);
  ui.accept(result);
  nowMs = UINT32_MAX - 1000;
  app::Command prepare;
  prepare.type = app::CommandType::PrepareSleep;
  TEST_ASSERT_TRUE(ui.submit(prepare, &commands));
  const uint32_t sleepId = ui.pendingId;
  nowMs += 1999;
  TEST_ASSERT_FALSE(ui.preparationExpired(nowMs));
  nowMs += 1;
  TEST_ASSERT_TRUE(ui.preparationExpired(nowMs));
  TEST_ASSERT_FALSE(ui.requestResume(&commands));
  TEST_ASSERT_EQUAL(app::SleepPhase::Resuming, ui.sleepPhase);
  TEST_ASSERT_TRUE(ui.busy());
  TEST_ASSERT_FALSE(ui.submit(app::Command{}, &commands));
  rig.analyzer.service(&commands, &results);
  TEST_ASSERT_TRUE(rig.analyzer.preparedForSleep());
  TEST_ASSERT_TRUE(ui.requestResume(&commands));
  const auto queued = commands.sends;
  TEST_ASSERT_TRUE(ui.requestResume(&commands));
  TEST_ASSERT_EQUAL_UINT(queued, commands.sends);
  rig.analyzer.service(&commands, &results);
  xQueueReceive(&results, &result, 0);
  TEST_ASSERT_EQUAL(app::CommandType::PrepareSleep, result.type);
  TEST_ASSERT_FALSE(ui.accept(result));
  TEST_ASSERT_EQUAL_UINT32(sleepId, ui.pendingId);
  rig.analyzer.service(&commands, &results);
  xQueueReceive(&results, &result, 0);
  TEST_ASSERT_EQUAL(app::CommandType::Resume, result.type);
  TEST_ASSERT_TRUE(ui.accept(result));
  TEST_ASSERT_EQUAL(app::SleepPhase::Awake, ui.sleepPhase);
  TEST_ASSERT_FALSE(ui.busy());
  TEST_ASSERT_FALSE(rig.analyzer.preparedForSleep());
  rig.analyzer.measure();
  TEST_ASSERT_TRUE(rig.queue.occupied);
}

void test_prepared_ui_remains_busy_until_resume_acknowledged() {
  app::UiState ui;
  ui.accept(app::Result{});
  FakeQueue commands(sizeof(app::Command));
  app::Command prepare;
  prepare.type = app::CommandType::PrepareSleep;
  TEST_ASSERT_TRUE(ui.submit(prepare, &commands));
  app::Result result;
  result.type = prepare.type;
  result.id = ui.pendingId;
  TEST_ASSERT_TRUE(ui.accept(result));
  TEST_ASSERT_EQUAL(app::SleepPhase::Prepared, ui.sleepPhase);
  TEST_ASSERT_TRUE(ui.busy());
  TEST_ASSERT_FALSE(ui.accept(result));
  xQueueReset(&commands);
  TEST_ASSERT_TRUE(ui.requestResume(&commands));
  app::Command resume;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(&commands, &resume, 0));
  result.type = app::CommandType::Resume;
  result.failure = app::Failure::Invalid;
  TEST_ASSERT_TRUE(ui.accept(result));
  TEST_ASSERT_TRUE(ui.busy());
  TEST_ASSERT_EQUAL(app::SleepPhase::Resuming, ui.sleepPhase);
  TEST_ASSERT_FALSE(ui.resumeQueued);
  TEST_ASSERT_FALSE(ui.requestResume(&commands));
  nowMs += 999;
  TEST_ASSERT_FALSE(ui.requestResume(&commands));
  nowMs += 1;
  TEST_ASSERT_TRUE(ui.requestResume(&commands));
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(&commands, &resume, 0));
  TEST_ASSERT_EQUAL_UINT32(result.id, resume.id);
  TEST_ASSERT_EQUAL(app::CommandType::Resume, resume.type);
  result.failure = app::Failure::None;
  TEST_ASSERT_TRUE(ui.accept(result));
  TEST_ASSERT_FALSE(ui.busy());
}

void test_sleep_handshake_preserves_disabled_sensor_power_and_settings() {
  Rig rig;
  app::Command settings;
  settings.settings = rig.analyzer.effective();
  settings.settings.coEnabled = false;
  settings.settings.heEnabled = false;
  rig.analyzer.execute(settings);
  const auto writes = Preferences::writes;
  app::Command command;
  command.type = app::CommandType::PrepareSleep;
  TEST_ASSERT_EQUAL(app::Failure::Invalid, rig.analyzer.execute(command).failure);
  command.id = 7;
  rig.analyzer.execute(command);
  command.type = app::CommandType::Resume;
  rig.analyzer.execute(command);
  TEST_ASSERT_EQUAL_INT(LOW, pinValues[PIN_CO_ENABLE]);
  TEST_ASSERT_EQUAL_INT(LOW, pinValues[PIN_HE_ENABLE]);
  TEST_ASSERT_FALSE(rig.analyzer.coWarming());
  TEST_ASSERT_EQUAL_UINT(writes, Preferences::writes);
}

void test_resume_when_already_awake_is_harmless_and_does_not_restart_co() {
  Rig rig;
  nowMs = app::Analyzer::CO_STARTUP_MS;
  rig.analyzer.measure();
  app::Command resume;
  resume.type = app::CommandType::Resume;
  resume.id = 50;
  const auto result = rig.analyzer.execute(resume);
  TEST_ASSERT_EQUAL(app::Failure::None, result.failure);
  TEST_ASSERT_EQUAL_UINT32(1, result.generation);
  TEST_ASSERT_FALSE(rig.analyzer.coWarming());
  TEST_ASSERT_TRUE(rig.queue.occupied);
  TEST_ASSERT_EQUAL_INT(HIGH, pinValues[PIN_CO_ENABLE]);
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
}

void test_resume_retry_delay_survives_clock_wrap() {
  app::UiState ui;
  ui.accept(app::Result{});
  FakeQueue commands(sizeof(app::Command));
  app::Command prepare;
  prepare.type = app::CommandType::PrepareSleep;
  ui.submit(prepare, &commands);
  xQueueReset(&commands);
  ui.requestResume(&commands);
  xQueueReset(&commands);
  app::Result failure;
  failure.type = app::CommandType::Resume;
  failure.id = ui.pendingId;
  failure.failure = app::Failure::Invalid;
  nowMs = UINT32_MAX - 100;
  TEST_ASSERT_TRUE(ui.accept(failure));
  nowMs += 999;
  TEST_ASSERT_FALSE(ui.requestResume(&commands));
  nowMs += 1;
  TEST_ASSERT_TRUE(ui.requestResume(&commands));
  TEST_ASSERT_TRUE(ui.busy());
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
  RUN_TEST(test_valid_settings_repair_is_a_noop);
  RUN_TEST(test_po2_repair_preserves_valid_limits_and_calibration);
  RUN_TEST(test_calibration_repair_is_limited_to_invalid_fields_and_dependencies);
  RUN_TEST(test_repaired_load_defers_writes_and_retries_full_save);
  RUN_TEST(test_repaired_wake_preserves_values_but_keeps_calibration_required);
  RUN_TEST(test_invalid_timeout_alone_does_not_flag_general_settings_recovery);
  RUN_TEST(test_startup_reports_adc_failure_without_blocking_other_device);
  RUN_TEST(test_full_result_path_retains_outcome_without_blocking_measurements);
  RUN_TEST(test_successful_calibration_then_reset_changes_live_and_stored_values);
  RUN_TEST(test_co_warmup_skips_conversions_until_configured_deadline);
  RUN_TEST(test_co_reenable_restarts_warmup_across_clock_wrap);
  RUN_TEST(test_adc_recovery_does_not_restart_co_warmup);
  RUN_TEST(test_prepare_stops_publication_and_resume_restores_power_with_fresh_generation);
  RUN_TEST(test_prepare_timeout_resume_ignores_late_ack_and_survives_full_command_queue);
  RUN_TEST(test_prepared_ui_remains_busy_until_resume_acknowledged);
  RUN_TEST(test_sleep_handshake_preserves_disabled_sensor_power_and_settings);
  RUN_TEST(test_resume_when_already_awake_is_harmless_and_does_not_restart_co);
  RUN_TEST(test_resume_retry_delay_survives_clock_wrap);
  return UNITY_END();
}