#include <unity.h>
#include "app/Analyzer.h"
#include "pin_config.h"

void setUp() {
  Preferences::values.clear();
  Preferences::values["calib_start"] = 0;
  Preferences::values["o2_calib_21"] = 10;
  Preferences::values["ext_o2_air"] = 10;
  Preferences::values["he_calib_100"] = 620;
  Preferences::available = true;
  Preferences::failWrite = false;
  Preferences::writes = 0;
  Preferences::reads = 0;
  Preferences::failAfter = -1;
  nowMs = 0;
  pinInputs.fill(HIGH);
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

void test_external_o2_at_startup_disables_he_co_without_changing_settings() {
  pinInputs[PIN_EXT_O2_DETECT] = EXT_O2_DETECT_ACTIVE_LEVEL;
  Rig rig;
  Adafruit_ADS1115::devices[0]->counts = 320;
  TEST_ASSERT_EQUAL_INT(LOW, pinValues[PIN_HE_ENABLE]);
  TEST_ASSERT_EQUAL_INT(LOW, pinValues[PIN_CO_ENABLE]);
  TEST_ASSERT_TRUE(rig.analyzer.effective().heEnabled);
  TEST_ASSERT_TRUE(rig.analyzer.effective().coEnabled);
  TEST_ASSERT_FALSE(rig.analyzer.coWarming());
  rig.analyzer.measure();
  sensorsData sample;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(rig.handle, &sample, 0));
  TEST_ASSERT_EQUAL(ChannelState::Valid, sample.o2State);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 10, sample.O2Level.millivolts);
  TEST_ASSERT_EQUAL(ChannelState::Disabled, sample.heState);
  TEST_ASSERT_EQUAL(ChannelState::Disabled, sample.coState);
  TEST_ASSERT_EQUAL(ChannelState::Disabled, sample.temperatureState);
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[0]->differential01Reads);
  TEST_ASSERT_EQUAL_UINT(0, Adafruit_ADS1115::devices[0]->differential23Reads);
  TEST_ASSERT_EQUAL_UINT(0, Adafruit_ADS1115::devices[1]->resultReads);
  TEST_ASSERT_EQUAL_UINT32(1, sample.generation);
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
}

void test_external_o2_hotplug_updates_even_with_blocked_result_delivery() {
  Rig rig;
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  results.occupied = true;
  nowMs = app::policy::CO_WARMUP_MS;
  TEST_ASSERT_FALSE(rig.analyzer.coWarming());
  pinInputs[PIN_EXT_O2_DETECT] = EXT_O2_DETECT_ACTIVE_LEVEL;
  TEST_ASSERT_TRUE(rig.analyzer.service(&commands, &results));
  TEST_ASSERT_EQUAL_INT(LOW, pinValues[PIN_HE_ENABLE]);
  TEST_ASSERT_EQUAL_INT(LOW, pinValues[PIN_CO_ENABLE]);
  rig.analyzer.measure();
  sensorsData sample;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(rig.handle, &sample, 0));
  TEST_ASSERT_EQUAL(ChannelState::Disabled, sample.heState);
  TEST_ASSERT_EQUAL(ChannelState::Disabled, sample.coState);
  TEST_ASSERT_EQUAL_UINT32(1, sample.generation);
  pinInputs[PIN_EXT_O2_DETECT] = HIGH;
  TEST_ASSERT_TRUE(rig.analyzer.service(&commands, &results));
  TEST_ASSERT_EQUAL_INT(HIGH, pinValues[PIN_HE_ENABLE]);
  TEST_ASSERT_EQUAL_INT(HIGH, pinValues[PIN_CO_ENABLE]);
  TEST_ASSERT_TRUE(rig.analyzer.coWarming());
  rig.analyzer.measure();
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(rig.handle, &sample, 0));
  TEST_ASSERT_NOT_EQUAL(ChannelState::Disabled, sample.heState);
  TEST_ASSERT_NOT_EQUAL(ChannelState::Disabled, sample.coState);
  TEST_ASSERT_EQUAL_UINT32(1, sample.generation);
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
}

void test_external_o2_unplug_restores_latest_settings_and_respects_o2_disabled() {
  pinInputs[PIN_EXT_O2_DETECT] = EXT_O2_DETECT_ACTIVE_LEVEL;
  Rig rig;
  app::Command command;
  command.settings = rig.analyzer.effective();
  command.settings.o2Enabled = false;
  command.settings.heEnabled = false;
  const auto result = rig.analyzer.execute(command);
  TEST_ASSERT_EQUAL(app::Failure::None, result.failure);
  TEST_ASSERT_FALSE(result.effective.heEnabled);
  TEST_ASSERT_TRUE(result.effective.coEnabled);
  TEST_ASSERT_EQUAL_INT(LOW, pinValues[PIN_CO_ENABLE]);
  rig.analyzer.measure();
  sensorsData sample;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(rig.handle, &sample, 0));
  TEST_ASSERT_EQUAL(ChannelState::Disabled, sample.o2State);
  TEST_ASSERT_EQUAL(ChannelState::Disabled, sample.heState);
  TEST_ASSERT_EQUAL(ChannelState::Disabled, sample.coState);
  const auto writes = Preferences::writes;
  pinInputs[PIN_EXT_O2_DETECT] = HIGH;
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  rig.analyzer.service(&commands, &results);
  TEST_ASSERT_EQUAL_INT(LOW, pinValues[PIN_HE_ENABLE]);
  TEST_ASSERT_EQUAL_INT(HIGH, pinValues[PIN_CO_ENABLE]);
  rig.analyzer.measure();
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(rig.handle, &sample, 0));
  TEST_ASSERT_EQUAL(ChannelState::Disabled, sample.o2State);
  TEST_ASSERT_EQUAL(ChannelState::Disabled, sample.heState);
  TEST_ASSERT_NOT_EQUAL(ChannelState::Disabled, sample.coState);
  TEST_ASSERT_EQUAL_UINT32(result.generation, sample.generation);
  TEST_ASSERT_EQUAL_UINT(writes, Preferences::writes);
}

void test_external_o2_resume_reads_current_jack_state() {
  Rig rig;
  for (const int level : {LOW, HIGH}) {
    app::Command prepare;
    prepare.type = app::CommandType::PrepareSleep;
    prepare.id = 7;
    TEST_ASSERT_EQUAL(app::Failure::None, rig.analyzer.execute(prepare).failure);
    pinInputs[PIN_EXT_O2_DETECT] = level;
    TEST_ASSERT_EQUAL_INT(LOW, pinValues[PIN_HE_ENABLE]);
    TEST_ASSERT_EQUAL_INT(LOW, pinValues[PIN_CO_ENABLE]);
    app::Command resume;
    resume.type = app::CommandType::Resume;
    resume.id = prepare.id;
    TEST_ASSERT_EQUAL(app::Failure::None, rig.analyzer.execute(resume).failure);
    const int expectedPower = level == EXT_O2_DETECT_ACTIVE_LEVEL ? LOW : HIGH;
    TEST_ASSERT_EQUAL_INT(expectedPower, pinValues[PIN_HE_ENABLE]);
    TEST_ASSERT_EQUAL_INT(expectedPower, pinValues[PIN_CO_ENABLE]);
  }
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
}

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
  TEST_ASSERT_EQUAL_INT(HIGH, pinValues[PIN_CO_ENABLE]);
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

void test_clear_optional_calibration_and_unchanged_commands() {
  Preferences::values["o2_calib_21"] = 10;
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
  TEST_ASSERT_FALSE(value.calibrateOnStart);
  Preferences::values.erase("calib_start");
  SettingsStore store;
  store.begin();
  TEST_ASSERT_FALSE(store.load().calibrateOnStart);
  Preferences::values["calib_start"] = 1;
  TEST_ASSERT_TRUE(store.load().calibrateOnStart);
  Preferences::values["calib_start"] = 0;
  TEST_ASSERT_FALSE(store.load().calibrateOnStart);
  value.o2Enabled = false;
  value.coEnabled = false;
  value.heEnabled = false;
  value.calibrateOnStart = false;
  value.brightness = 8;
  value.sleepMinutes = 0;
  value.po2Bottom = 1;
  value.po2Deco = 2;
  value.o2Air = 20;
  value.o2Pure = 30;
  value.heCalibration = 400;
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
    {4.9f, 55, 700, 10, NAN, 700}, {20.1f, 55, 700, 10, NAN, 700},
    {12, 12, 700, 12, NAN, 700}, {12, INFINITY, 700, 12, NAN, 700},
    {12, 29.9f, 700, 12, NAN, 700},
    {12, 101, 700, 12, NAN, 700}, {12, -INFINITY, 700, 12, NAN, 700},
    {12, 55, 399.9f, 12, 55, 620}, {12, 55, NAN, 12, 55, 620},
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

void test_calibration_guard_boundaries() {
  TEST_ASSERT_FALSE(AnalyzerSettings::validO2Air(4.99f));
  TEST_ASSERT_TRUE(AnalyzerSettings::validO2Air(5.0f));
  TEST_ASSERT_TRUE(AnalyzerSettings::validO2Air(20.0f));
  TEST_ASSERT_FALSE(AnalyzerSettings::validO2Air(20.01f));

  TEST_ASSERT_FALSE(AnalyzerSettings::validO2Pure(29.99f, 20.0f));
  TEST_ASSERT_TRUE(AnalyzerSettings::validO2Pure(30.0f, 20.0f));
  TEST_ASSERT_TRUE(AnalyzerSettings::validO2Pure(100.0f, 20.0f));
  TEST_ASSERT_FALSE(AnalyzerSettings::validO2Pure(100.01f, 20.0f));

  TEST_ASSERT_FALSE(AnalyzerSettings::validHeCalibration(399.99f));
  TEST_ASSERT_TRUE(AnalyzerSettings::validHeCalibration(400.0f));
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

void test_repaired_wake_preserves_valid_calibration() {
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
  TEST_ASSERT_EQUAL(app::Failure::LoadedDefaults, result.failure);
  TEST_ASSERT_EQUAL_FLOAT(12, result.effective.o2Air);
  TEST_ASSERT_EQUAL_FLOAT(700, result.effective.heCalibration);
  TEST_ASSERT_EQUAL_UINT(0, Adafruit_ADS1115::devices[0]->differential23Reads);
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
  analyzer.measure();
  sensorsData sample;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(handle, &sample, 0));
  TEST_ASSERT_TRUE(std::isfinite(sample.O2Level.percentage));
  TEST_ASSERT_TRUE(std::isfinite(sample.HeLevel.percentage));
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
  const auto initial = analyzer.begin();
  TEST_ASSERT_FALSE(initial.complete);
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[1]->beginCalls);
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
    for (nowMs = 0; nowMs <= app::policy::CALIBRATION_TIMEOUT_MS;
      nowMs += app::policy::CALIBRATION_SAMPLE_MS) {
    analyzer.service(&commands, &results);
  }
  analyzer.service(&commands, &results);
  app::Result result;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(&results, &result, 0));
  TEST_ASSERT_EQUAL(app::CommandType::Startup, result.type);
  TEST_ASSERT_EQUAL(app::Failure::Sampling, result.failure);
  TEST_ASSERT_EQUAL(SensorError::ADC_Init_Failed, result.sensorError);
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

static app::Result startServiceCalibration(Rig& rig, FakeQueue& commands, FakeQueue& results,
                                           app::CommandType type, uint32_t id = 42) {
  rig.analyzer.service(&commands, &results);
  app::Result startup;
  xQueueReceive(&results, &startup, 0);
  app::Command command;
  command.type = type;
  command.id = id;
  xQueueSend(&commands, &command, 0);
  rig.analyzer.service(&commands, &results);
  app::Result progress;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(&results, &progress, 0));
  return progress;
}

static app::Result finishServiceCalibration(Rig& rig, FakeQueue& commands, FakeQueue& results,
                                            app::CommandType type, uint32_t id = 42) {
  auto progress = startServiceCalibration(rig, commands, results, type, id);
    for (nowMs = app::policy::CALIBRATION_SAMPLE_MS;
      nowMs <= app::policy::CALIBRATION_MINIMUM_MS;
      nowMs += app::policy::CALIBRATION_SAMPLE_MS) {
    rig.analyzer.service(&commands, &results);
  }
  xQueueReceive(&results, &progress, 0);
  rig.analyzer.service(&commands, &results);
  app::Result terminal;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(&results, &terminal, 0));
  return terminal;
}

void test_o2_readings_select_independent_profiles_without_generation_change() {
  Preferences::values["ext_o2_air"] = 20;
  Rig rig;
  Adafruit_ADS1115::devices[0]->counts = 320;
  rig.analyzer.measure();
  sensorsData internal;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(rig.handle, &internal, 0));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.9f, internal.O2Level.percentage);
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[0]->differential23Reads);
  TEST_ASSERT_EQUAL_UINT(0, Adafruit_ADS1115::devices[0]->differential01Reads);
  pinInputs[PIN_EXT_O2_DETECT] = EXT_O2_DETECT_ACTIVE_LEVEL;
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  rig.analyzer.service(&commands, &results);
  rig.analyzer.measure();
  sensorsData external;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(rig.handle, &external, 0));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.45f, external.O2Level.percentage);
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[0]->differential01Reads);
  TEST_ASSERT_EQUAL_UINT(1, Adafruit_ADS1115::devices[0]->differential23Reads);
  TEST_ASSERT_EQUAL_UINT32(internal.generation, external.generation);
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
}

void test_o2_calibration_saves_only_selected_profile_and_survives_reboot() {
  Preferences::values["o2_calib_100"] = 50;
  {
    pinInputs[PIN_EXT_O2_DETECT] = EXT_O2_DETECT_ACTIVE_LEVEL;
    Rig rig;
    FakeQueue commands(sizeof(app::Command));
    FakeQueue results(sizeof(app::Result));
    Adafruit_ADS1115::devices[0]->counts = 384;
    const auto air = finishServiceCalibration(rig, commands, results, app::CommandType::CalibrateAir);
    TEST_ASSERT_EQUAL(app::Failure::None, air.failure);
    TEST_ASSERT_EQUAL_FLOAT(12, air.effective.externalO2Air);
    TEST_ASSERT_EQUAL_FLOAT(10, air.effective.o2Air);
    nowMs = 0;
    Adafruit_ADS1115::devices[0]->counts = 1920;
    const auto pure = finishServiceCalibration(rig, commands, results, app::CommandType::CalibratePure);
    TEST_ASSERT_EQUAL(app::Failure::None, pure.failure);
    TEST_ASSERT_EQUAL_FLOAT(60, pure.effective.externalO2Pure);
    TEST_ASSERT_EQUAL_FLOAT(50, pure.effective.o2Pure);
    TEST_ASSERT_GREATER_THAN_UINT(0, Adafruit_ADS1115::devices[0]->differential01Reads);
    TEST_ASSERT_EQUAL_UINT(0, Adafruit_ADS1115::devices[0]->differential23Reads);
  }
  pinInputs[PIN_EXT_O2_DETECT] = HIGH;
  nowMs = 0;
  Rig restarted;
  TEST_ASSERT_EQUAL_FLOAT(12, restarted.analyzer.effective().externalO2Air);
  TEST_ASSERT_EQUAL_FLOAT(60, restarted.analyzer.effective().externalO2Pure);
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  Adafruit_ADS1115::devices[0]->counts = 448;
  const auto internal = finishServiceCalibration(restarted, commands, results, app::CommandType::CalibrateAir);
  TEST_ASSERT_EQUAL(app::Failure::None, internal.failure);
  TEST_ASSERT_EQUAL_FLOAT(14, Preferences::values["o2_calib_21"]);
  TEST_ASSERT_EQUAL_FLOAT(50, Preferences::values["o2_calib_100"]);
  TEST_ASSERT_EQUAL_FLOAT(12, Preferences::values["ext_o2_air"]);
  TEST_ASSERT_EQUAL_FLOAT(60, Preferences::values["ext_o2_pure"]);
  TEST_ASSERT_GREATER_THAN_UINT(0, Adafruit_ADS1115::devices[0]->differential23Reads);
  TEST_ASSERT_EQUAL_UINT(0, Adafruit_ADS1115::devices[0]->differential01Reads);
}

void test_missing_external_calibration_is_not_accepted_by_settings_save() {
  Preferences::values.erase("ext_o2_air");
  Preferences::values["ext_o2_pure"] = 55;
  pinInputs[PIN_EXT_O2_DETECT] = EXT_O2_DETECT_ACTIVE_LEVEL;
  {
    Rig rig;
    TEST_ASSERT_TRUE(rig.settingsStore.hasOxygenCalibration());
    TEST_ASSERT_FALSE(rig.settingsStore.hasOxygenCalibration(true));
    TEST_ASSERT_FALSE(rig.settingsStore.loadedDefaults());
    Adafruit_ADS1115::devices[0]->counts = 320;
    rig.analyzer.measure();
    sensorsData sample;
    TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(rig.handle, &sample, 0));
    TEST_ASSERT_EQUAL(ChannelState::Invalid, sample.o2State);
    TEST_ASSERT_EQUAL_FLOAT(10, sample.O2Level.millivolts);
    app::Command save;
    save.settings = rig.analyzer.effective();
    save.settings.externalO2Air = 18;
    save.settings.externalO2Pure = 80;
    save.settings.brightness = 64;
    const auto saved = rig.analyzer.execute(save);
    TEST_ASSERT_TRUE(saved.oxygenCalibrationRequired);
    TEST_ASSERT_EQUAL_FLOAT(10, saved.effective.externalO2Air);
    TEST_ASSERT_TRUE(std::isnan(saved.effective.externalO2Pure));
    FakeQueue commands(sizeof(app::Command));
    FakeQueue results(sizeof(app::Result));
    rig.analyzer.service(&commands, &results);
    app::Result rejected;
    TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(&results, &rejected, 0));
    app::Command pure;
    pure.type = app::CommandType::CalibratePure;
    pure.id = 42;
    TEST_ASSERT_EQUAL_INT(pdPASS, xQueueSend(&commands, &pure, 0));
    rig.analyzer.service(&commands, &results);
    rig.analyzer.service(&commands, &results);
    TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(&results, &rejected, 0));
    TEST_ASSERT_EQUAL(app::Failure::CalibrationRequired, rejected.failure);
    TEST_ASSERT_TRUE(rejected.complete);
  }
  Rig restarted;
  TEST_ASSERT_FALSE(restarted.settingsStore.hasOxygenCalibration(true));
  TEST_ASSERT_TRUE(restarted.settingsStore.hasOxygenCalibration());
  TEST_ASSERT_FALSE(restarted.settingsStore.loadedDefaults());
}

void test_o2_reset_and_clear_affect_only_selected_profile() {
  Preferences::values["o2_calib_21"] = 12;
  Preferences::values["o2_calib_100"] = 50;
  Preferences::values["ext_o2_air"] = 18;
  Preferences::values["ext_o2_pure"] = 80;
  Rig rig;
  app::Command command;
  command.type = app::CommandType::ClearPure;
  pinInputs[PIN_EXT_O2_DETECT] = EXT_O2_DETECT_ACTIVE_LEVEL;
  const auto externalClear = rig.analyzer.execute(command);
  TEST_ASSERT_TRUE(std::isnan(externalClear.effective.externalO2Pure));
  TEST_ASSERT_EQUAL_FLOAT(50, externalClear.effective.o2Pure);
  command.type = app::CommandType::ResetAir;
  const auto externalReset = rig.analyzer.execute(command);
  TEST_ASSERT_EQUAL_FLOAT(10, externalReset.effective.externalO2Air);
  TEST_ASSERT_EQUAL_FLOAT(12, externalReset.effective.o2Air);
  pinInputs[PIN_EXT_O2_DETECT] = HIGH;
  const auto internalReset = rig.analyzer.execute(command);
  TEST_ASSERT_EQUAL_FLOAT(10, internalReset.effective.o2Air);
  TEST_ASSERT_EQUAL_FLOAT(50, internalReset.effective.o2Pure);
  command.type = app::CommandType::ClearPure;
  const auto internalClear = rig.analyzer.execute(command);
  TEST_ASSERT_TRUE(std::isnan(internalClear.effective.o2Pure));
  TEST_ASSERT_EQUAL_FLOAT(10, internalClear.effective.externalO2Air);
}

void test_external_calibration_partial_write_is_unaccepted_until_retry() {
  Preferences::values.erase("ext_o2_air");
  Preferences::values["ext_o2_pure"] = 55;
  pinInputs[PIN_EXT_O2_DETECT] = EXT_O2_DETECT_ACTIVE_LEVEL;
  Rig rig;
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  Adafruit_ADS1115::devices[0]->counts = 384;
  Preferences::failAfter = 1;
  const auto failed = finishServiceCalibration(rig, commands, results, app::CommandType::CalibrateAir);
  TEST_ASSERT_EQUAL(app::Failure::Storage, failed.failure);
  TEST_ASSERT_TRUE(failed.oxygenCalibrationRequired);
  TEST_ASSERT_FALSE(rig.settingsStore.hasOxygenCalibration(true));
  TEST_ASSERT_EQUAL_FLOAT(10, rig.analyzer.effective().externalO2Air);
  Preferences::failAfter = -1;
  app::Command save;
  save.settings = rig.analyzer.effective();
  TEST_ASSERT_EQUAL(app::Failure::None, rig.analyzer.execute(save).failure);
  TEST_ASSERT_TRUE(std::isnan(Preferences::values["ext_o2_air"]));
  TEST_ASSERT_TRUE(std::isnan(Preferences::values["ext_o2_pure"]));
  TEST_ASSERT_EQUAL_FLOAT(10, Preferences::values["o2_calib_21"]);
  nowMs = 0;
  const auto accepted = finishServiceCalibration(rig, commands, results, app::CommandType::CalibrateAir);
  TEST_ASSERT_EQUAL(app::Failure::None, accepted.failure);
  TEST_ASSERT_FALSE(accepted.oxygenCalibrationRequired);
  TEST_ASSERT_EQUAL_FLOAT(12, Preferences::values["ext_o2_air"]);
  TEST_ASSERT_TRUE(std::isnan(Preferences::values["ext_o2_pure"]));
  SettingsStore reloaded;
  reloaded.begin();
  reloaded.load();
  TEST_ASSERT_TRUE(reloaded.hasOxygenCalibration(true));
  TEST_ASSERT_TRUE(reloaded.hasOxygenCalibration());
}

void test_external_startup_calibration_accepts_only_external_profile() {
  Preferences::values.erase("ext_o2_air");
  Preferences::values["calib_start"] = 1;
  pinInputs[PIN_EXT_O2_DETECT] = EXT_O2_DETECT_ACTIVE_LEVEL;
  Rig rig;
  TEST_ASSERT_TRUE(rig.analyzer.calibrating());
  Adafruit_ADS1115::devices[0]->counts = 384;
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  for (nowMs = 0; nowMs <= app::policy::CALIBRATION_MINIMUM_MS;
       nowMs += app::policy::CALIBRATION_SAMPLE_MS) {
    rig.analyzer.service(&commands, &results);
  }
  rig.analyzer.service(&commands, &results);
  app::Result startup;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(&results, &startup, 0));
  TEST_ASSERT_EQUAL(app::CommandType::Startup, startup.type);
  TEST_ASSERT_EQUAL(app::Failure::None, startup.failure);
  TEST_ASSERT_FALSE(startup.oxygenCalibrationRequired);
  TEST_ASSERT_TRUE(rig.settingsStore.hasOxygenCalibration(true));
  TEST_ASSERT_EQUAL_FLOAT(12, startup.effective.externalO2Air);
  TEST_ASSERT_EQUAL_FLOAT(10, startup.effective.o2Air);
  TEST_ASSERT_EQUAL_FLOAT(10, Preferences::values["o2_calib_21"]);
}

void test_corrupt_external_calibration_repairs_only_external_profile() {
  Preferences::values["ext_o2_air"] = 25;
  Preferences::values["ext_o2_pure"] = 90;
  Preferences::values["o2_calib_100"] = 50;
  SettingsStore store;
  store.begin();
  const auto loaded = store.load();
  TEST_ASSERT_TRUE(store.loadedDefaults());
  TEST_ASSERT_FALSE(store.hasOxygenCalibration(true));
  TEST_ASSERT_TRUE(store.hasOxygenCalibration());
  TEST_ASSERT_EQUAL_FLOAT(10, loaded.o2Air);
  TEST_ASSERT_EQUAL_FLOAT(50, loaded.o2Pure);
  TEST_ASSERT_EQUAL_FLOAT(10, loaded.externalO2Air);
  TEST_ASSERT_TRUE(std::isnan(loaded.externalO2Pure));
  TEST_ASSERT_TRUE(store.save(loaded, loaded));
  TEST_ASSERT_TRUE(std::isnan(Preferences::values["ext_o2_air"]));
  TEST_ASSERT_TRUE(std::isnan(Preferences::values["ext_o2_pure"]));
}

void test_stable_service_calibration_saves_at_five_seconds() {
  Rig rig;
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  Adafruit_ADS1115::devices[0]->counts = 640;
  auto started = startServiceCalibration(rig, commands, results, app::CommandType::CalibrateAir);
  TEST_ASSERT_FALSE(started.complete);
  TEST_ASSERT_EQUAL(app::CalibrationPhase::Settling, started.calibrationPhase);
    for (nowMs = app::policy::CALIBRATION_SAMPLE_MS;
      nowMs <= app::policy::CALIBRATION_MINIMUM_MS;
      nowMs += app::policy::CALIBRATION_SAMPLE_MS) {
    rig.analyzer.service(&commands, &results);
  }
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20, rig.analyzer.effective().o2Air);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20, Preferences::values["o2_calib_21"]);
  xQueueReceive(&results, &started, 0);
  rig.analyzer.service(&commands, &results);
  app::Result saved;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(&results, &saved, 0));
  TEST_ASSERT_TRUE(saved.complete);
  TEST_ASSERT_EQUAL(app::CalibrationPhase::Saved, saved.calibrationPhase);
  TEST_ASSERT_EQUAL_UINT32(app::policy::CALIBRATION_MINIMUM_MS, saved.calibrationElapsedMs);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20, saved.calibration);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20, saved.effective.o2Air);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20, Preferences::values["o2_calib_21"]);
}

void test_unstable_service_calibration_fails_at_timeout_without_saving() {
  Rig rig;
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  Adafruit_ADS1115::devices[0]->counts = 320;
  auto progress = startServiceCalibration(rig, commands, results, app::CommandType::CalibrateAir);
  for (nowMs = app::policy::CALIBRATION_SAMPLE_MS;
       nowMs <= app::policy::CALIBRATION_TIMEOUT_MS;
       nowMs += app::policy::CALIBRATION_SAMPLE_MS) {
    Adafruit_ADS1115::devices[0]->counts = (nowMs / app::policy::CALIBRATION_SAMPLE_MS) % 2 ? 320 : 352;
    rig.analyzer.service(&commands, &results);
  }
  xQueueReceive(&results, &progress, 0);
  rig.analyzer.service(&commands, &results);
  app::Result failed;
  xQueueReceive(&results, &failed, 0);
  TEST_ASSERT_EQUAL(app::Failure::Sampling, failed.failure);
  TEST_ASSERT_EQUAL(app::CalibrationPhase::Failed, failed.calibrationPhase);
  TEST_ASSERT_EQUAL_UINT32(app::policy::CALIBRATION_TIMEOUT_MS, failed.calibrationElapsedMs);
  TEST_ASSERT_EQUAL_FLOAT(10, failed.effective.o2Air);
  TEST_ASSERT_EQUAL_FLOAT(10, Preferences::values["o2_calib_21"]);
}

static void assertDriftingCalibrationFails(bool rising) {
  Rig rig;
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  auto progress = startServiceCalibration(rig, commands, results, app::CommandType::CalibrateAir);
  for (nowMs = app::policy::CALIBRATION_SAMPLE_MS;
       nowMs <= app::policy::CALIBRATION_TIMEOUT_MS;
       nowMs += app::policy::CALIBRATION_SAMPLE_MS) {
    const int16_t offset = static_cast<int16_t>((nowMs / app::policy::CALIBRATION_SAMPLE_MS) / 2);
    Adafruit_ADS1115::devices[0]->counts = rising ? 640 + offset : 680 - offset;
    rig.analyzer.service(&commands, &results);
  }
  xQueueReceive(&results, &progress, 0);
  rig.analyzer.service(&commands, &results);
  app::Result failed;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(&results, &failed, 0));
  TEST_ASSERT_EQUAL(app::Failure::Sampling, failed.failure);
  TEST_ASSERT_EQUAL(app::CalibrationPhase::Failed, failed.calibrationPhase);
  TEST_ASSERT_EQUAL_FLOAT(10, failed.effective.o2Air);
}

void test_positive_drift_fails_even_when_window_spread_is_small() {
  assertDriftingCalibrationFails(true);
}

void test_negative_drift_fails_even_when_window_spread_is_small() {
  assertDriftingCalibrationFails(false);
}

void test_drifting_calibration_can_settle_before_timeout() {
  Rig rig;
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  auto progress = startServiceCalibration(rig, commands, results, app::CommandType::CalibrateAir);
  for (nowMs = app::policy::CALIBRATION_SAMPLE_MS;
       nowMs <= app::policy::CALIBRATION_TIMEOUT_MS;
       nowMs += app::policy::CALIBRATION_SAMPLE_MS) {
    const uint32_t sampleNumber = nowMs / app::policy::CALIBRATION_SAMPLE_MS;
    Adafruit_ADS1115::devices[0]->counts = nowMs <= app::policy::CALIBRATION_MINIMUM_MS
      ? 600 + static_cast<int16_t>(sampleNumber / 2) : 610;
    rig.analyzer.service(&commands, &results);
    if (!rig.analyzer.calibrating()) break;
  }
  xQueueReceive(&results, &progress, 0);
  rig.analyzer.service(&commands, &results);
  app::Result saved;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(&results, &saved, 0));
  TEST_ASSERT_EQUAL(app::CalibrationPhase::Saved, saved.calibrationPhase);
  TEST_ASSERT_GREATER_THAN(app::policy::CALIBRATION_MINIMUM_MS, saved.calibrationElapsedMs);
  TEST_ASSERT_LESS_THAN(app::policy::CALIBRATION_TIMEOUT_MS, saved.calibrationElapsedMs);
}

void test_calibration_progress_and_cancel_keep_previous_value() {
  Rig rig;
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  app::UiState state;
  app::Result startup;
  rig.analyzer.service(&commands, &results);
  xQueueReceive(&results, &startup, 0);
  TEST_ASSERT_TRUE(state.accept(startup));
  app::Command command;
  command.type = app::CommandType::CalibrateAir;
  TEST_ASSERT_TRUE(state.submit(command, &commands));
  rig.analyzer.service(&commands, &results);
  app::Result progress;
  xQueueReceive(&results, &progress, 0);
  TEST_ASSERT_TRUE(state.accept(progress));
  TEST_ASSERT_TRUE(state.busy());
  TEST_ASSERT_TRUE(state.cancelCalibration(&commands));
  rig.analyzer.service(&commands, &results);
  rig.analyzer.service(&commands, &results);
  app::Result cancelled;
  xQueueReceive(&results, &cancelled, 0);
  TEST_ASSERT_TRUE(state.accept(cancelled));
  TEST_ASSERT_FALSE(state.busy());
  TEST_ASSERT_EQUAL(app::Failure::Cancelled, cancelled.failure);
  TEST_ASSERT_EQUAL(app::CalibrationPhase::Cancelled, cancelled.calibrationPhase);
  TEST_ASSERT_EQUAL_FLOAT(10, cancelled.effective.o2Air);
}

void test_calibration_reports_saved_only_after_persistence() {
  Rig rig;
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  Adafruit_ADS1115::devices[0]->counts = 640;
  auto progress = startServiceCalibration(rig, commands, results, app::CommandType::CalibrateAir);
  Preferences::failWrite = true;
    for (nowMs = app::policy::CALIBRATION_SAMPLE_MS;
      nowMs <= app::policy::CALIBRATION_MINIMUM_MS;
      nowMs += app::policy::CALIBRATION_SAMPLE_MS) {
    rig.analyzer.service(&commands, &results);
  }
  xQueueReceive(&results, &progress, 0);
  rig.analyzer.service(&commands, &results);
  app::Result failed;
  xQueueReceive(&results, &failed, 0);
  TEST_ASSERT_EQUAL(app::Failure::Storage, failed.failure);
  TEST_ASSERT_EQUAL(app::CalibrationPhase::Failed, failed.calibrationPhase);
  TEST_ASSERT_EQUAL_FLOAT(10, failed.effective.o2Air);
}

static void assertAirCalibrationRejected(int16_t counts) {
  nowMs = 0;
  Rig rig;
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  Adafruit_ADS1115::devices[0]->counts = counts;
  const auto failed = finishServiceCalibration(rig, commands, results, app::CommandType::CalibrateAir);
  TEST_ASSERT_EQUAL(app::Failure::Invalid, failed.failure);
  TEST_ASSERT_EQUAL(app::CalibrationPhase::Failed, failed.calibrationPhase);
  TEST_ASSERT_EQUAL_FLOAT(app::policy::O2_AIR_CALIBRATION_DEFAULT_MV, failed.effective.o2Air);
  TEST_ASSERT_EQUAL_FLOAT(app::policy::O2_AIR_CALIBRATION_DEFAULT_MV, Preferences::values["o2_calib_21"]);
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
}

void test_out_of_range_air_calibration_is_not_saved() {
  assertAirCalibrationRejected(128);  // 4 mV
  assertAirCalibrationRejected(672);  // 21 mV
}

void test_out_of_range_pure_o2_calibration_is_not_saved() {
  Preferences::values["o2_calib_100"] = 50;
  Rig rig;
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  Adafruit_ADS1115::devices[0]->counts = 928;  // 29 mV
  const auto failed = finishServiceCalibration(rig, commands, results, app::CommandType::CalibratePure);
  TEST_ASSERT_EQUAL(app::Failure::Invalid, failed.failure);
  TEST_ASSERT_EQUAL(app::CalibrationPhase::Failed, failed.calibrationPhase);
  TEST_ASSERT_EQUAL_FLOAT(50, failed.effective.o2Pure);
  TEST_ASSERT_EQUAL_FLOAT(50, Preferences::values["o2_calib_100"]);
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
}

void test_out_of_range_helium_calibration_is_not_saved() {
  Rig rig;
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  Adafruit_ADS1115::devices[1]->counts = 6384;  // 399 mV
  const auto failed = finishServiceCalibration(rig, commands, results, app::CommandType::CalibrateHe);
  TEST_ASSERT_EQUAL(app::Failure::Invalid, failed.failure);
  TEST_ASSERT_EQUAL(app::CalibrationPhase::Failed, failed.calibrationPhase);
  TEST_ASSERT_EQUAL_FLOAT(app::policy::HE_CALIBRATION_DEFAULT_MV, failed.effective.heCalibration);
  TEST_ASSERT_EQUAL_FLOAT(app::policy::HE_CALIBRATION_DEFAULT_MV, Preferences::values["he_calib_100"]);
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
}

void test_successful_calibration_then_reset_changes_live_and_stored_values() {
  Rig rig;
  Adafruit_ADS1115::devices[0]->counts = 640;
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  const auto calibrated = finishServiceCalibration(rig, commands, results, app::CommandType::CalibrateAir);
  app::Command command;
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

void test_co_warmup_samples_raw_value_and_ends_at_configured_deadline() {
  Rig rig;
  auto* oxygen = Adafruit_ADS1115::devices[0];
  auto* secondary = Adafruit_ADS1115::devices[1];
  oxygen->counts = 320;
  secondary->counts = 7040;
  TEST_ASSERT_TRUE(rig.analyzer.coWarming());
  nowMs = app::policy::CO_WARMUP_MS - 1;
  rig.analyzer.measure();
  sensorsData sample;
  xQueueReceive(rig.handle, &sample, 0);
  TEST_ASSERT_EQUAL(ChannelState::Warming, sample.coState);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.5f, sample.CoLevel.ppm);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 440, sample.CoLevel.millivolts);
  TEST_ASSERT_EQUAL(ChannelState::Valid, sample.o2State);
  TEST_ASSERT_EQUAL(ChannelState::Valid, sample.heState);
  TEST_ASSERT_EQUAL_UINT(2, secondary->singleEndedReads);
  nowMs = app::policy::CO_WARMUP_MS;
  TEST_ASSERT_FALSE(rig.analyzer.coWarming());
  rig.analyzer.measure();
  xQueueReceive(rig.handle, &sample, 0);
  TEST_ASSERT_EQUAL(ChannelState::Valid, sample.coState);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.5f, sample.CoLevel.ppm);
  TEST_ASSERT_EQUAL_UINT(4, secondary->singleEndedReads);
}

void test_co_correction_is_persisted_applied_and_invalidates_measurements() {
  Rig rig;
  Adafruit_ADS1115::devices[1]->counts = 7040;
  nowMs = app::policy::CO_WARMUP_MS;
  app::Command command;
  command.settings = rig.analyzer.effective();
  command.settings.coCorrectionPpm = -3;
  const auto result = rig.analyzer.execute(command);
  TEST_ASSERT_EQUAL(app::Failure::None, result.failure);
  TEST_ASSERT_EQUAL_INT8(-3, result.effective.coCorrectionPpm);
  TEST_ASSERT_EQUAL_UINT32(2, result.generation);
  TEST_ASSERT_EQUAL_FLOAT(-3, Preferences::values["co_correction"]);
  rig.analyzer.measure();
  sensorsData sample;
  xQueueReceive(rig.handle, &sample, 0);
  TEST_ASSERT_EQUAL_UINT32(2, sample.generation);
  TEST_ASSERT_EQUAL(ChannelState::Valid, sample.coState);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 9.5f, sample.CoLevel.ppm);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 440, sample.CoLevel.millivolts);
  const auto writes = Preferences::writes;
  TEST_ASSERT_EQUAL_UINT32(2, rig.analyzer.execute(command).generation);
  TEST_ASSERT_EQUAL_UINT(writes, Preferences::writes);
  command.settings.coCorrectionPpm = app::CO_CORRECTION_MAX_PPM + 1;
  TEST_ASSERT_EQUAL(app::Failure::Invalid, rig.analyzer.execute(command).failure);
  TEST_ASSERT_EQUAL_INT8(-3, rig.analyzer.effective().coCorrectionPpm);

  SettingsStore restarted;
  restarted.begin();
  TEST_ASSERT_EQUAL_INT8(-3, restarted.load().coCorrectionPpm);
  TEST_ASSERT_FALSE(restarted.loadedDefaults());
}

void test_language_persistence_does_not_invalidate_measurements() {
  Rig rig;
  TEST_ASSERT_EQUAL_UINT8(app::LANGUAGE_ENGLISH, rig.analyzer.effective().language);
  app::Command command;
  command.settings = rig.analyzer.effective();
  command.settings.language = app::LANGUAGE_BULGARIAN;
  const auto result = rig.analyzer.execute(command);
  TEST_ASSERT_EQUAL(app::Failure::None, result.failure);
  TEST_ASSERT_EQUAL_UINT8(app::LANGUAGE_BULGARIAN, result.effective.language);
  TEST_ASSERT_EQUAL_UINT32(1, result.generation);
  SettingsStore restarted;
  restarted.begin();
  TEST_ASSERT_EQUAL_UINT8(app::LANGUAGE_BULGARIAN, restarted.load().language);
  TEST_ASSERT_FALSE(restarted.loadedDefaults());
  const auto writes = Preferences::writes;
  TEST_ASSERT_EQUAL(app::Failure::None, rig.analyzer.execute(command).failure);
  TEST_ASSERT_EQUAL_UINT(writes, Preferences::writes);
  command.settings.language = 2;
  TEST_ASSERT_EQUAL(app::Failure::Invalid, rig.analyzer.execute(command).failure);
  TEST_ASSERT_EQUAL_UINT8(app::LANGUAGE_BULGARIAN, rig.analyzer.effective().language);
  command.settings.language = app::LANGUAGE_ENGLISH;
  Preferences::failWrite = true;
  TEST_ASSERT_EQUAL(app::Failure::Storage, rig.analyzer.execute(command).failure);
  TEST_ASSERT_EQUAL_UINT8(app::LANGUAGE_BULGARIAN, rig.analyzer.effective().language);
  TEST_ASSERT_EQUAL_FLOAT(app::LANGUAGE_BULGARIAN, Preferences::values["language"]);
  Preferences::failWrite = false;
  TEST_ASSERT_EQUAL(app::Failure::None, rig.analyzer.execute(command).failure);
  TEST_ASSERT_EQUAL_UINT8(app::LANGUAGE_ENGLISH, rig.analyzer.effective().language);
}

void test_invalid_stored_language_falls_back_without_calibration_recovery() {
  Preferences::values["language"] = 255;
  SettingsStore store;
  store.begin();
  const auto loaded = store.load();
  TEST_ASSERT_EQUAL_UINT8(app::LANGUAGE_ENGLISH, loaded.language);
  TEST_ASSERT_FALSE(store.loadedDefaults());
  TEST_ASSERT_TRUE(store.hasOxygenCalibration());
  TEST_ASSERT_TRUE(store.hasHeliumCalibration());
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
  TEST_ASSERT_TRUE(store.save(loaded, loaded));
  TEST_ASSERT_EQUAL_FLOAT(app::LANGUAGE_ENGLISH, Preferences::values["language"]);
  const auto writes = Preferences::writes;
  TEST_ASSERT_TRUE(store.save(loaded, loaded));
  TEST_ASSERT_EQUAL_UINT(writes, Preferences::writes);
  AnalyzerSettings invalid = loaded;
  invalid.language = 255;
  TEST_ASSERT_FALSE(invalid.valid());
  TEST_ASSERT_TRUE(invalid.repairInvalidFields());
  TEST_ASSERT_EQUAL_UINT8(app::LANGUAGE_ENGLISH, invalid.language);
}

void test_invalid_stored_co_correction_is_repaired_to_zero() {
  Preferences::values["co_correction"] = app::CO_CORRECTION_MIN_PPM - 1;
  SettingsStore store;
  store.begin();
  const auto loaded = store.load();
  TEST_ASSERT_TRUE(store.loadedDefaults());
  TEST_ASSERT_EQUAL_INT8(0, loaded.coCorrectionPpm);
  TEST_ASSERT_TRUE(AnalyzerSettings::validCoCorrection(app::CO_CORRECTION_MIN_PPM));
  TEST_ASSERT_TRUE(AnalyzerSettings::validCoCorrection(app::CO_CORRECTION_MAX_PPM));
}

void test_co_reenable_restarts_warmup_across_clock_wrap() {
  Rig rig;
  app::Command command;
  command.settings = rig.analyzer.effective();
  command.settings.coEnabled = false;
  command.settings.heEnabled = false;
  rig.analyzer.execute(command);
  TEST_ASSERT_EQUAL_INT(LOW, pinValues[PIN_CO_ENABLE]);
  TEST_ASSERT_FALSE(rig.analyzer.coWarming());
  nowMs = UINT32_MAX - 1000;
  command.settings.coEnabled = true;
  rig.analyzer.execute(command);
  TEST_ASSERT_TRUE(rig.analyzer.coWarming());
  nowMs += app::policy::CO_WARMUP_MS - 1;
  command.settings.brightness = 64;
  rig.analyzer.execute(command);
  TEST_ASSERT_TRUE(rig.analyzer.coWarming());
  nowMs += 1;
  TEST_ASSERT_FALSE(rig.analyzer.coWarming());
}

void test_helium_keeps_temperature_powered_when_co_is_disabled() {
  Rig rig;
  app::Command command;
  command.settings = rig.analyzer.effective();
  command.settings.coEnabled = false;
  const auto result = rig.analyzer.execute(command);
  TEST_ASSERT_EQUAL(app::Failure::None, result.failure);
  TEST_ASSERT_TRUE(result.effective.heEnabled);
  TEST_ASSERT_EQUAL_INT(HIGH, pinValues[PIN_HE_ENABLE]);
  TEST_ASSERT_EQUAL_INT(HIGH, pinValues[PIN_CO_ENABLE]);
  TEST_ASSERT_FALSE(rig.analyzer.coWarming());
}

void test_adc_recovery_does_not_restart_co_warmup() {
  Rig rig;
  auto* secondary = Adafruit_ADS1115::devices[1];
  nowMs = app::policy::CO_WARMUP_MS;
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
  nowMs = app::policy::CO_WARMUP_MS;
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
  TEST_ASSERT_EQUAL(ChannelState::Valid, sample.coState);
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
  nowMs = app::policy::CO_WARMUP_MS;
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

void test_missing_cold_boot_calibration_suppresses_derived_values_not_raw_samples() {
  Preferences::values.erase("o2_calib_21");
  Preferences::values.erase("he_calib_100");
  Rig rig;
  Adafruit_ADS1115::devices[0]->counts = 320;
  Adafruit_ADS1115::devices[1]->counts = 1600;
  app::Command command;
  command.settings = rig.analyzer.effective();
  const auto result = rig.analyzer.execute(command);
  TEST_ASSERT_EQUAL(app::Failure::None, result.failure);
  TEST_ASSERT_TRUE(result.oxygenCalibrationRequired);
  TEST_ASSERT_TRUE(result.heliumCalibrationRequired);
  TEST_ASSERT_EQUAL_STRING("O2 and He calibration required", result.calibrationRequiredMessage());
  rig.analyzer.measure();
  sensorsData sample;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(rig.handle, &sample, 0));
  TEST_ASSERT_TRUE(std::isnan(sample.O2Level.percentage));
  TEST_ASSERT_TRUE(std::isnan(sample.HeLevel.percentage));
  TEST_ASSERT_TRUE(std::isfinite(sample.O2Level.millivolts));
  TEST_ASSERT_TRUE(std::isfinite(sample.HeLevel.millivolts));
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
}

void test_invalid_calibration_is_independent_on_cold_boot_and_wake() {
  for (bool wake : {false, true}) {
    for (bool invalidOxygen : {false, true}) {
      for (float invalid : {0.0f, NAN, INFINITY, -INFINITY}) {
        Preferences::values["o2_calib_21"] = invalidOxygen ? invalid : 12;
        Preferences::values["he_calib_100"] = invalidOxygen ? 700 : invalid;
        FakeQueue queue(sizeof(sensorsData));
        QueueHandle_t handle = &queue;
        SensorManager sensors(handle);
        SettingsStore store;
        app::Analyzer analyzer(store, sensors);
        const auto result = analyzer.begin(wake);
        TEST_ASSERT_EQUAL(invalidOxygen, result.oxygenCalibrationRequired);
        TEST_ASSERT_EQUAL(!invalidOxygen, result.heliumCalibrationRequired);
        TEST_ASSERT_EQUAL_STRING(invalidOxygen ? "O2 calibration required" : "He calibration required",
                                 result.calibrationRequiredMessage());
        Adafruit_ADS1115::devices[0]->counts = 320;
        Adafruit_ADS1115::devices[1]->counts = 1600;
        analyzer.measure();
        sensorsData sample;
        TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(handle, &sample, 0));
        TEST_ASSERT_EQUAL(invalidOxygen, std::isnan(sample.O2Level.percentage));
        TEST_ASSERT_TRUE(std::isnan(sample.HeLevel.percentage));
        TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
      }
    }
  }
}

void test_dirty_settings_save_does_not_accept_fallback_calibration_on_reboot() {
  Preferences::values["o2_calib_21"] = 0;
  Preferences::values.erase("he_calib_100");
  Preferences::values["brightness"] = 0;
  {
    Rig rig;
    app::Command save;
    save.settings = rig.analyzer.effective();
    save.settings.brightness = 64;
    const auto result = rig.analyzer.execute(save);
    TEST_ASSERT_EQUAL(app::Failure::None, result.failure);
    TEST_ASSERT_TRUE(result.oxygenCalibrationRequired);
    TEST_ASSERT_TRUE(result.heliumCalibrationRequired);
    TEST_ASSERT_TRUE(std::isnan(Preferences::values["o2_calib_21"]));
    TEST_ASSERT_TRUE(std::isnan(Preferences::values["he_calib_100"]));
    TEST_ASSERT_FALSE(rig.settingsStore.hasOxygenCalibration());
    TEST_ASSERT_FALSE(rig.settingsStore.hasHeliumCalibration());
  }
  Rig restarted;
  TEST_ASSERT_FALSE(restarted.settingsStore.loadedDefaults());
  const auto writes = Preferences::writes;
  app::Command save;
  save.settings = restarted.analyzer.effective();
  const auto result = restarted.analyzer.execute(save);
  TEST_ASSERT_EQUAL_UINT8(64, result.effective.brightness);
  TEST_ASSERT_TRUE(result.oxygenCalibrationRequired);
  TEST_ASSERT_TRUE(result.heliumCalibrationRequired);
  TEST_ASSERT_EQUAL_UINT(writes, Preferences::writes);
}

void test_same_default_reset_accepts_only_its_channel_after_successful_save() {
  Preferences::values.erase("o2_calib_21");
  Preferences::values.erase("he_calib_100");
  Rig rig;
  app::Command reset;
  reset.type = app::CommandType::ResetAir;
  Preferences::failWrite = true;
  const auto failed = rig.analyzer.execute(reset);
  TEST_ASSERT_EQUAL(app::Failure::Storage, failed.failure);
  TEST_ASSERT_TRUE(failed.oxygenCalibrationRequired);
  TEST_ASSERT_FALSE(rig.settingsStore.hasOxygenCalibration());
  Preferences::failWrite = false;
  const auto oxygen = rig.analyzer.execute(reset);
  TEST_ASSERT_EQUAL(app::Failure::None, oxygen.failure);
  TEST_ASSERT_FALSE(oxygen.oxygenCalibrationRequired);
  TEST_ASSERT_TRUE(oxygen.heliumCalibrationRequired);
  TEST_ASSERT_EQUAL_UINT32(failed.generation + 1, oxygen.generation);
  TEST_ASSERT_EQUAL_FLOAT(10, Preferences::values["o2_calib_21"]);
  reset.type = app::CommandType::ResetHe;
  const auto helium = rig.analyzer.execute(reset);
  TEST_ASSERT_EQUAL(app::Failure::None, helium.failure);
  TEST_ASSERT_FALSE(helium.oxygenCalibrationRequired);
  TEST_ASSERT_FALSE(helium.heliumCalibrationRequired);
  TEST_ASSERT_EQUAL_UINT32(oxygen.generation + 1, helium.generation);
  TEST_ASSERT_EQUAL_FLOAT(620, Preferences::values["he_calib_100"]);
  SettingsStore restarted;
  restarted.begin();
  restarted.load();
  TEST_ASSERT_TRUE(restarted.hasOxygenCalibration());
  TEST_ASSERT_TRUE(restarted.hasHeliumCalibration());
}

void test_air_acceptance_clears_orphan_pure_point_across_reboot() {
  Preferences::values.erase("o2_calib_21");
  Preferences::values["o2_calib_100"] = 55;
  Rig rig;
  TEST_ASSERT_TRUE(std::isnan(rig.analyzer.effective().o2Pure));
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  rig.analyzer.service(&commands, &results);
  app::Result startup;
  xQueueReceive(&results, &startup, 0);
  app::Command command;
  command.type = app::CommandType::CalibratePure;
  command.id = 41;
  xQueueSend(&commands, &command, 0);
  rig.analyzer.service(&commands, &results);
  rig.analyzer.service(&commands, &results);
  app::Result refused;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(&results, &refused, 0));
  TEST_ASSERT_EQUAL(app::Failure::CalibrationRequired, refused.failure);
  TEST_ASSERT_EQUAL_UINT(0, Adafruit_ADS1115::devices[0]->differential23Reads);
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
  Adafruit_ADS1115::devices[0]->counts = 320;
  const auto accepted = finishServiceCalibration(rig, commands, results, app::CommandType::CalibrateAir, 42);
  TEST_ASSERT_EQUAL(app::Failure::None, accepted.failure);
  TEST_ASSERT_FALSE(accepted.oxygenCalibrationRequired);
  TEST_ASSERT_EQUAL_UINT32(refused.generation + 1, accepted.generation);
  TEST_ASSERT_TRUE(std::isnan(Preferences::values["o2_calib_100"]));
  SettingsStore restarted;
  restarted.begin();
  const auto reloaded = restarted.load();
  TEST_ASSERT_TRUE(restarted.hasOxygenCalibration());
  TEST_ASSERT_TRUE(std::isnan(reloaded.o2Pure));
}

void test_partial_acceptance_failure_is_unaccepted_and_reconciled_on_retry() {
  Preferences::values.erase("o2_calib_21");
  Rig rig;
  Adafruit_ADS1115::devices[0]->counts = 320;
  Preferences::failAfter = 1;
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  const auto failed = finishServiceCalibration(rig, commands, results, app::CommandType::CalibrateAir);
  TEST_ASSERT_EQUAL(app::Failure::Storage, failed.failure);
  TEST_ASSERT_TRUE(failed.oxygenCalibrationRequired);
  TEST_ASSERT_FALSE(rig.settingsStore.hasOxygenCalibration());
  TEST_ASSERT_EQUAL_FLOAT(10, Preferences::values["o2_calib_21"]);
  Preferences::failAfter = -1;
  app::Command command;
  command.type = app::CommandType::ApplySettings;
  command.settings = rig.analyzer.effective();
  const auto reconciled = rig.analyzer.execute(command);
  TEST_ASSERT_EQUAL(app::Failure::None, reconciled.failure);
  TEST_ASSERT_TRUE(reconciled.oxygenCalibrationRequired);
  TEST_ASSERT_TRUE(std::isnan(Preferences::values["o2_calib_21"]));
  SettingsStore restarted;
  restarted.begin();
  restarted.load();
  TEST_ASSERT_FALSE(restarted.hasOxygenCalibration());
}

void test_calibration_required_message_respects_enabled_channels_and_helium_dependency() {
  app::Result result;
  result.oxygenCalibrationRequired = true;
  result.heliumCalibrationRequired = true;
  result.effective.o2Enabled = false;
  result.effective.heEnabled = false;
  TEST_ASSERT_NULL(result.calibrationRequiredMessage());
  result.effective.heEnabled = true;
  TEST_ASSERT_EQUAL_STRING("O2 and He calibration required", result.calibrationRequiredMessage());
  result.heliumCalibrationRequired = false;
  TEST_ASSERT_EQUAL_STRING("O2 calibration required", result.calibrationRequiredMessage());
  result.oxygenCalibrationRequired = false;
  TEST_ASSERT_NULL(result.calibrationRequiredMessage());
}

void test_enabled_startup_calibration_uses_incremental_session() {
  Preferences::values["calib_start"] = 1;
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager sensors(handle);
  SettingsStore store;
  app::Analyzer analyzer(store, sensors);
  const auto initial = analyzer.begin();
  Adafruit_ADS1115::devices[0]->counts = 640;
  TEST_ASSERT_EQUAL(app::CommandType::Startup, initial.type);
  TEST_ASSERT_TRUE(analyzer.calibrating());
  TEST_ASSERT_EQUAL_UINT(0, Adafruit_ADS1115::devices[0]->differential23Reads);
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
    for (nowMs = 0; nowMs <= app::policy::CALIBRATION_MINIMUM_MS;
      nowMs += app::policy::CALIBRATION_SAMPLE_MS) {
    analyzer.service(&commands, &results);
  }
  analyzer.service(&commands, &results);
  TEST_ASSERT_FALSE(analyzer.calibrating());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20, analyzer.effective().o2Air);
  app::Result startup;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(&results, &startup, 0));
  TEST_ASSERT_EQUAL(app::CommandType::Startup, startup.type);
  TEST_ASSERT_TRUE(startup.complete);
  TEST_ASSERT_EQUAL(app::CalibrationPhase::Saved, startup.calibrationPhase);
  TEST_ASSERT_EQUAL(app::Failure::None, startup.failure);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20, startup.calibration);
  TEST_ASSERT_EQUAL_UINT(1, Preferences::writes);
}

void test_out_of_range_startup_calibration_keeps_previous_value() {
  Preferences::values["calib_start"] = 1;
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager sensors(handle);
  SettingsStore store;
  app::Analyzer analyzer(store, sensors);
  const auto initial = analyzer.begin();
  Adafruit_ADS1115::devices[0]->counts = 672;  // 21 mV
  TEST_ASSERT_FALSE(initial.complete);
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  for (nowMs = 0; nowMs <= app::policy::CALIBRATION_MINIMUM_MS;
       nowMs += app::policy::CALIBRATION_SAMPLE_MS) {
    analyzer.service(&commands, &results);
  }
  analyzer.service(&commands, &results);
  app::Result startup;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(&results, &startup, 0));
  TEST_ASSERT_EQUAL(app::CommandType::Startup, startup.type);
  TEST_ASSERT_EQUAL(app::Failure::Invalid, startup.failure);
  TEST_ASSERT_EQUAL(app::CalibrationPhase::Failed, startup.calibrationPhase);
  TEST_ASSERT_EQUAL_FLOAT(app::policy::O2_AIR_CALIBRATION_DEFAULT_MV, startup.effective.o2Air);
  TEST_ASSERT_EQUAL_FLOAT(app::policy::O2_AIR_CALIBRATION_DEFAULT_MV, Preferences::values["o2_calib_21"]);
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
}

void test_unstable_startup_calibration_keeps_previous_value() {
  Preferences::values["calib_start"] = 1;
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager sensors(handle);
  SettingsStore store;
  app::Analyzer analyzer(store, sensors);
  const auto initial = analyzer.begin();
  TEST_ASSERT_FALSE(initial.complete);
  FakeQueue commands(sizeof(app::Command));
  FakeQueue results(sizeof(app::Result));
  for (nowMs = 0; nowMs <= app::policy::CALIBRATION_TIMEOUT_MS;
       nowMs += app::policy::CALIBRATION_SAMPLE_MS) {
    Adafruit_ADS1115::devices[0]->counts =
        (nowMs / app::policy::CALIBRATION_SAMPLE_MS) % 2 ? 320 : 352;
    analyzer.service(&commands, &results);
  }
  analyzer.service(&commands, &results);
  app::Result startup;
  TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(&results, &startup, 0));
  TEST_ASSERT_EQUAL(app::CommandType::Startup, startup.type);
  TEST_ASSERT_EQUAL(app::Failure::Sampling, startup.failure);
  TEST_ASSERT_EQUAL(app::CalibrationPhase::Failed, startup.calibrationPhase);
  TEST_ASSERT_EQUAL_FLOAT(app::policy::O2_AIR_CALIBRATION_DEFAULT_MV, startup.effective.o2Air);
  TEST_ASSERT_EQUAL_FLOAT(app::policy::O2_AIR_CALIBRATION_DEFAULT_MV, Preferences::values["o2_calib_21"]);
  TEST_ASSERT_EQUAL_UINT(0, Preferences::writes);
}

void test_save_on_exit_uses_acknowledged_values_and_releases_editing() {
  for (auto failure : {app::Failure::None, app::Failure::Invalid, app::Failure::Storage}) {
    app::UiState ui;
    ui.accept(app::Result{});
    FakeQueue commands(sizeof(app::Command));
    ui.settingsEditing = true;
    auto draft = ui.effective;
    draft.brightness = 64;
    draft.language = app::LANGUAGE_BULGARIAN;
    TEST_ASSERT_TRUE(ui.closeSettings(draft, &commands));
    TEST_ASSERT_FALSE(ui.settingsEditing);
    TEST_ASSERT_TRUE(ui.busy());
    TEST_ASSERT_EQUAL_UINT8(128, ui.effective.brightness);
    TEST_ASSERT_EQUAL_UINT8(app::LANGUAGE_ENGLISH, ui.effective.language);
    app::Command sent;
    TEST_ASSERT_EQUAL_INT(pdPASS, xQueueReceive(&commands, &sent, 0));
    TEST_ASSERT_EQUAL(app::CommandType::ApplySettings, sent.type);
    TEST_ASSERT_EQUAL_UINT8(64, sent.settings.brightness);
    TEST_ASSERT_EQUAL_UINT8(app::LANGUAGE_BULGARIAN, sent.settings.language);
    app::Result result;
    result.type = sent.type;
    result.id = sent.id;
    result.failure = failure;
    if (failure == app::Failure::None) result.effective = sent.settings;
    TEST_ASSERT_TRUE(ui.accept(result));
    TEST_ASSERT_TRUE(ui.shouldSyncSettings(result));
    TEST_ASSERT_FALSE(ui.settingsEditing);
    TEST_ASSERT_FALSE(ui.busy());
    TEST_ASSERT_EQUAL_UINT8(failure == app::Failure::None ? 64 : 128, ui.effective.brightness);
    TEST_ASSERT_EQUAL_UINT8(failure == app::Failure::None ? app::LANGUAGE_BULGARIAN : app::LANGUAGE_ENGLISH,
                 ui.effective.language);
  }
}

void test_refused_save_on_exit_does_not_retain_edits_or_block_sleep() {
  app::UiState ui;
  ui.accept(app::Result{});
  FakeQueue commands(sizeof(app::Command));
  commands.occupied = true;
  ui.settingsEditing = true;
  auto draft = ui.effective;
  draft.brightness = 64;
  draft.language = app::LANGUAGE_BULGARIAN;
  TEST_ASSERT_FALSE(ui.closeSettings(draft, &commands));
  TEST_ASSERT_FALSE(ui.settingsEditing);
  TEST_ASSERT_FALSE(ui.busy());
  TEST_ASSERT_EQUAL_UINT8(128, ui.effective.brightness);
  TEST_ASSERT_EQUAL_UINT8(app::LANGUAGE_ENGLISH, ui.effective.language);
  commands.occupied = false;
  TEST_ASSERT_FALSE(ui.closeSettings(draft, &commands));
  TEST_ASSERT_EQUAL_UINT(0, commands.sends);
}

void test_delayed_results_do_not_sync_an_open_editor() {
  for (auto type : {app::CommandType::ApplySettings, app::CommandType::CalibrateAir}) {
    app::UiState ui;
    ui.accept(app::Result{});
    FakeQueue commands(sizeof(app::Command));
    app::Command command;
    command.type = type;
    TEST_ASSERT_TRUE(ui.submit(command, &commands));
    ui.settingsEditing = true;
    app::Result result;
    result.type = type;
    result.id = ui.pendingId;
    result.effective.brightness = 64;
    result.effective.language = app::LANGUAGE_BULGARIAN;
    result.effective.o2Air = 12;
    TEST_ASSERT_TRUE(ui.accept(result));
    TEST_ASSERT_TRUE(ui.settingsEditing);
    TEST_ASSERT_FALSE(ui.shouldSyncSettings(result));
    TEST_ASSERT_EQUAL_UINT8(64, ui.effective.brightness);
    TEST_ASSERT_EQUAL_UINT8(app::LANGUAGE_BULGARIAN, ui.effective.language);
    TEST_ASSERT_EQUAL_FLOAT(12, ui.effective.o2Air);
    ui.settingsEditing = false;
    TEST_ASSERT_TRUE(ui.shouldSyncSettings(result));
    ui.settingsEditing = true;
    TEST_ASSERT_TRUE(ui.shouldSyncSettings(app::Result{}));
  }
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_o2_readings_select_independent_profiles_without_generation_change);
  RUN_TEST(test_o2_calibration_saves_only_selected_profile_and_survives_reboot);
  RUN_TEST(test_missing_external_calibration_is_not_accepted_by_settings_save);
  RUN_TEST(test_o2_reset_and_clear_affect_only_selected_profile);
  RUN_TEST(test_external_calibration_partial_write_is_unaccepted_until_retry);
  RUN_TEST(test_external_startup_calibration_accepts_only_external_profile);
  RUN_TEST(test_corrupt_external_calibration_repairs_only_external_profile);
  RUN_TEST(test_external_o2_at_startup_disables_he_co_without_changing_settings);
  RUN_TEST(test_external_o2_hotplug_updates_even_with_blocked_result_delivery);
  RUN_TEST(test_external_o2_unplug_restores_latest_settings_and_respects_o2_disabled);
  RUN_TEST(test_external_o2_resume_reads_current_jack_state);
  RUN_TEST(test_apply_settings_acknowledges_live_state_and_generation);
  RUN_TEST(test_rejection_and_write_failure_keep_effective_state);
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
  RUN_TEST(test_calibration_guard_boundaries);
  RUN_TEST(test_repaired_load_defers_writes_and_retries_full_save);
  RUN_TEST(test_repaired_wake_preserves_valid_calibration);
  RUN_TEST(test_invalid_timeout_alone_does_not_flag_general_settings_recovery);
  RUN_TEST(test_startup_reports_adc_failure_without_blocking_other_device);
  RUN_TEST(test_full_result_path_retains_outcome_without_blocking_measurements);
  RUN_TEST(test_stable_service_calibration_saves_at_five_seconds);
  RUN_TEST(test_unstable_service_calibration_fails_at_timeout_without_saving);
  RUN_TEST(test_positive_drift_fails_even_when_window_spread_is_small);
  RUN_TEST(test_negative_drift_fails_even_when_window_spread_is_small);
  RUN_TEST(test_drifting_calibration_can_settle_before_timeout);
  RUN_TEST(test_calibration_progress_and_cancel_keep_previous_value);
  RUN_TEST(test_calibration_reports_saved_only_after_persistence);
  RUN_TEST(test_out_of_range_air_calibration_is_not_saved);
  RUN_TEST(test_out_of_range_pure_o2_calibration_is_not_saved);
  RUN_TEST(test_out_of_range_helium_calibration_is_not_saved);
  RUN_TEST(test_successful_calibration_then_reset_changes_live_and_stored_values);
  RUN_TEST(test_co_warmup_samples_raw_value_and_ends_at_configured_deadline);
  RUN_TEST(test_co_correction_is_persisted_applied_and_invalidates_measurements);
  RUN_TEST(test_language_persistence_does_not_invalidate_measurements);
  RUN_TEST(test_invalid_stored_language_falls_back_without_calibration_recovery);
  RUN_TEST(test_invalid_stored_co_correction_is_repaired_to_zero);
  RUN_TEST(test_co_reenable_restarts_warmup_across_clock_wrap);
  RUN_TEST(test_helium_keeps_temperature_powered_when_co_is_disabled);
  RUN_TEST(test_adc_recovery_does_not_restart_co_warmup);
  RUN_TEST(test_prepare_stops_publication_and_resume_restores_power_with_fresh_generation);
  RUN_TEST(test_prepare_timeout_resume_ignores_late_ack_and_survives_full_command_queue);
  RUN_TEST(test_prepared_ui_remains_busy_until_resume_acknowledged);
  RUN_TEST(test_sleep_handshake_preserves_disabled_sensor_power_and_settings);
  RUN_TEST(test_resume_when_already_awake_is_harmless_and_does_not_restart_co);
  RUN_TEST(test_resume_retry_delay_survives_clock_wrap);
  RUN_TEST(test_missing_cold_boot_calibration_suppresses_derived_values_not_raw_samples);
  RUN_TEST(test_invalid_calibration_is_independent_on_cold_boot_and_wake);
  RUN_TEST(test_dirty_settings_save_does_not_accept_fallback_calibration_on_reboot);
  RUN_TEST(test_same_default_reset_accepts_only_its_channel_after_successful_save);
  RUN_TEST(test_air_acceptance_clears_orphan_pure_point_across_reboot);
  RUN_TEST(test_partial_acceptance_failure_is_unaccepted_and_reconciled_on_retry);
  RUN_TEST(test_calibration_required_message_respects_enabled_channels_and_helium_dependency);
  RUN_TEST(test_enabled_startup_calibration_uses_incremental_session);
  RUN_TEST(test_out_of_range_startup_calibration_keeps_previous_value);
  RUN_TEST(test_unstable_startup_calibration_keeps_previous_value);
  RUN_TEST(test_save_on_exit_uses_acknowledged_values_and_releases_editing);
  RUN_TEST(test_refused_save_on_exit_does_not_retain_edits_or_block_sleep);
  RUN_TEST(test_delayed_results_do_not_sync_an_open_editor);
  return UNITY_END();
}