#include <unity.h>
#include "app/SleepPolicy.h"
#include "settings/SettingsStore.h"
#include "app/Analyzer.h"

void setUp() {
  Preferences::values.clear();
  Preferences::available = true;
  Preferences::failWrite = false;
  Preferences::failAfter = -1;
  Preferences::writes = 0;
  Preferences::values["calib_start"] = 0;
  nowMs = 0;
  Adafruit_ADS1115::present[0] = Adafruit_ADS1115::present[1] = true;
}
void tearDown() {}

void test_sleep_default_and_invalid_load_do_not_reset_other_settings() {
  SettingsStore store;
  store.begin();
  Preferences::values["o2_calib_21"] = 12;
  TEST_ASSERT_EQUAL_UINT(5, store.load().sleepMinutes);
  Preferences::values["sleep_minutes"] = 255;
  const auto loaded = store.load();
  TEST_ASSERT_EQUAL_UINT(5, loaded.sleepMinutes);
  TEST_ASSERT_EQUAL_FLOAT(12, loaded.o2Air);
  TEST_ASSERT_TRUE(store.save(loaded, loaded));
  TEST_ASSERT_EQUAL_FLOAT(5, Preferences::values["sleep_minutes"]);
  TEST_ASSERT_EQUAL_FLOAT(12, Preferences::values["o2_calib_21"]);
}

void test_sleep_options_roundtrip_without_changing_measurement_semantics() {
  SettingsStore store;
  store.begin();
  auto previous = store.load();
  const uint8_t options[] = {0, 1, 2, 5, 10, 30};
  for (uint8_t minutes : options) {
    auto candidate = previous;
    candidate.sleepMinutes = minutes;
    TEST_ASSERT_TRUE(candidate.valid());
    TEST_ASSERT_TRUE(candidate.sameMeasurementSettings(previous));
    TEST_ASSERT_TRUE(store.save(candidate, previous));
    TEST_ASSERT_EQUAL_UINT(minutes, store.load().sleepMinutes);
    previous = candidate;
  }
  auto invalid = previous;
  invalid.sleepMinutes = 3;
  const auto writes = Preferences::writes;
  TEST_ASSERT_FALSE(store.save(invalid, previous));
  TEST_ASSERT_EQUAL_UINT(writes, Preferences::writes);
}

void test_sleep_only_after_selected_interval_when_idle_and_released() {
  const uint8_t options[] = {1, 2, 5, 10, 30};
  for (uint8_t minutes : options) {
    const uint32_t deadline = static_cast<uint32_t>(minutes) * 60000;
    TEST_ASSERT_FALSE(app::sleepDue(minutes, deadline - 1, false, true));
    TEST_ASSERT_TRUE(app::sleepDue(minutes, deadline, false, true));
    TEST_ASSERT_FALSE(app::sleepDue(minutes, deadline, true, true));
    TEST_ASSERT_FALSE(app::sleepDue(minutes, deadline, false, false));
    TEST_ASSERT_FALSE(app::sleepDue(minutes, 0, false, true));
  }
  TEST_ASSERT_FALSE(app::sleepDue(0, UINT32_MAX, false, true));
  TEST_ASSERT_FALSE(app::sleepDue(3, UINT32_MAX, false, true));
}

void test_wake_press_is_consumed_until_debounced_release() {
  app::WakeButton button;
  TEST_ASSERT_FALSE(button.released());
  TEST_ASSERT_TRUE(button.update(true, 0));
  TEST_ASSERT_TRUE(button.update(true, 5000));
  TEST_ASSERT_FALSE(button.released());
  TEST_ASSERT_TRUE(button.update(false, 5001));
  button.update(false, 5050);
  TEST_ASSERT_FALSE(button.released());
  TEST_ASSERT_TRUE(button.update(false, 5051));
  TEST_ASSERT_TRUE(button.released());
  TEST_ASSERT_FALSE(button.update(false, 5052));
  TEST_ASSERT_TRUE(button.update(true, 5053));
  TEST_ASSERT_FALSE(button.released());
}

void test_release_debounce_restarts_on_bounce_and_survives_wrap() {
  app::WakeButton button;
  button.update(false, UINT32_MAX - 25);
  button.update(true, UINT32_MAX - 10);
  button.update(false, UINT32_MAX - 5);
  button.update(false, 43);
  TEST_ASSERT_FALSE(button.released());
  button.update(false, 44);
  TEST_ASSERT_TRUE(button.released());
}

void test_dropdown_selection_maps_to_valid_timeout_values() {
  char text[64];
  TEST_ASSERT_TRUE(app::formatSleepOptions(text, sizeof(text)));
  TEST_ASSERT_EQUAL_STRING("Off\n1 min\n2 min\n5 min\n10 min\n30 min", text);
  char tooSmall[4];
  TEST_ASSERT_FALSE(app::formatSleepOptions(tooSmall, sizeof(tooSmall)));
  for (uint32_t index = 0; index < 6; ++index) {
    const uint8_t minutes = app::sleepMinutesForSelection(index);
    TEST_ASSERT_TRUE(AnalyzerSettings::validSleepMinutes(minutes));
    TEST_ASSERT_EQUAL_UINT32(index, app::sleepSelectionForMinutes(minutes));
  }
  TEST_ASSERT_EQUAL_UINT(5, app::sleepMinutesForSelection(6));
  TEST_ASSERT_EQUAL_UINT(5, app::sleepMinutesForSelection(UINT32_MAX));
  TEST_ASSERT_EQUAL_UINT32(3, app::sleepSelectionForMinutes(255));
}

void test_timeout_command_acknowledges_persistence_without_changing_generation() {
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager sensors(handle);
  SettingsStore store;
  app::Analyzer analyzer(store, sensors);
  const auto startup = analyzer.begin();
  app::Command command;
  command.id = 14;
  command.settings = startup.effective;
  command.settings.sleepMinutes = 0;
  const auto result = analyzer.execute(command);
  TEST_ASSERT_EQUAL(app::Failure::None, result.failure);
  TEST_ASSERT_EQUAL_UINT32(14, result.id);
  TEST_ASSERT_EQUAL_UINT32(startup.generation, result.generation);
  TEST_ASSERT_EQUAL_UINT(0, result.effective.sleepMinutes);
  TEST_ASSERT_EQUAL_UINT(1, Preferences::writes);
  TEST_ASSERT_EQUAL_UINT(0, store.load().sleepMinutes);
}

void test_timeout_write_failure_returns_previous_effective_value() {
  FakeQueue queue(sizeof(sensorsData));
  QueueHandle_t handle = &queue;
  SensorManager sensors(handle);
  SettingsStore store;
  app::Analyzer analyzer(store, sensors);
  analyzer.begin();
  app::Command command;
  command.settings = analyzer.effective();
  command.settings.sleepMinutes = 10;
  Preferences::failWrite = true;
  const auto result = analyzer.execute(command);
  TEST_ASSERT_EQUAL(app::Failure::Storage, result.failure);
  TEST_ASSERT_EQUAL_UINT(5, result.effective.sleepMinutes);
  TEST_ASSERT_EQUAL_UINT(5, analyzer.effective().sleepMinutes);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_sleep_default_and_invalid_load_do_not_reset_other_settings);
  RUN_TEST(test_sleep_options_roundtrip_without_changing_measurement_semantics);
  RUN_TEST(test_sleep_only_after_selected_interval_when_idle_and_released);
  RUN_TEST(test_wake_press_is_consumed_until_debounced_release);
  RUN_TEST(test_release_debounce_restarts_on_bounce_and_survives_wrap);
  RUN_TEST(test_dropdown_selection_maps_to_valid_timeout_values);
  RUN_TEST(test_timeout_command_acknowledges_persistence_without_changing_generation);
  RUN_TEST(test_timeout_write_failure_returns_previous_effective_value);
  return UNITY_END();
}