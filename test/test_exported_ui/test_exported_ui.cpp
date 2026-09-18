#include <unity.h>
#include "FirmwareVersion.h"
#include "lvgl_ui_project.h"
#include "display/UiAdapter.h"
#include "main.h"
#include "ui-log.h"

DisplayManager displayManager;
static AnalyzerSettings effective;
static AnalyzerSettings submitted;
static bool settingsOpen;
static bool allowSubmit = true;
static unsigned errors;
static unsigned wifiScanActions;
static unsigned analyzerCommands;
static unsigned calibrationCancels;
static app::CommandType lastCommandType;
static unsigned flushes;
static bool renderedPixels;
static uint8_t brightness;

void DisplayManager::setBrightness(uint8_t value) { brightness = value; }
void DisplayManager::resetInactivity() {}
const AnalyzerSettings& uiSettings() { return effective; }
bool submitAnalyzerCommand(app::Command command) {
  ++analyzerCommands;
  lastCommandType = command.type;
  return true;
}
bool cancelAnalyzerCalibration() { ++calibrationCancels; return true; }
void openUiSettings() { settingsOpen = true; ui::syncSettings(effective); }
bool closeUiSettings(const AnalyzerSettings& draft) {
  settingsOpen = false;
  submitted = draft;
  ui::syncSettings(effective);
  return allowSubmit;
}
void messageBox(const char*, float) { ++errors; }
void action_reset_o2_100(lv_event_t*) {}
void action_list_wifi(lv_event_t*) { ++wifiScanActions; }
void action_update_firmware(lv_event_t*) {}

static void flush(lv_display_t* display, const lv_area_t* area, uint8_t* pixels) {
  ++flushes;
  const unsigned bytes = lv_area_get_width(area) * lv_area_get_height(area) * 2;
  for (unsigned index = 0; index < bytes; ++index) renderedPixels |= pixels[index] != 0;
  lv_display_flush_ready(display);
}

void setUp() {}
void tearDown() {}

static void refresh() {
  lv_tick_inc(40);
  lv_timer_handler();
  lv_refr_now(nullptr);
}

static void click(lv_obj_t* object) {
  TEST_ASSERT_NOT_NULL(object);
  lv_obj_send_event(object, LV_EVENT_CLICKED, nullptr);
  refresh();
}

static bool chartContains(lv_obj_t* graph, lv_chart_series_t* series, int32_t value) {
  int32_t* values = lv_chart_get_series_y_array(graph, series);
  for (uint32_t index = 0; index < lv_chart_get_point_count(graph); ++index) {
    if (values[index] == value) return true;
  }
  return false;
}

void test_export_boot_and_navigation() {
  TEST_ASSERT_EQUAL_PTR(mainscr, lv_screen_active());
  TEST_ASSERT_EQUAL_STRING("Unavailable", lv_subject_get_string(&main_o2_text));
  lv_obj_t* logo = lv_obj_find_by_name(mainscr, "phony_divers_logo");
  if (!logo) logo = lv_obj_find_by_name(mainscr, "logo_image");
  TEST_ASSERT_NOT_NULL(logo);
  TEST_ASSERT_EQUAL_PTR(phony_divers_logo, lv_image_get_src(logo));
  refresh();
  TEST_ASSERT_GREATER_THAN(0, flushes);
  TEST_ASSERT_TRUE(renderedPixels);
  click(lv_obj_find_by_name(mainscr, "readings"));
  TEST_ASSERT_EQUAL_PTR(largescr, lv_screen_active());
  click(lv_obj_find_by_name(largescr, "large_he_value"));
  TEST_ASSERT_EQUAL_PTR(mainscr, lv_screen_active());
  click(lv_obj_find_by_name(mainscr, "open_calibration"));
  TEST_ASSERT_NOT_EQUAL(mainscr, lv_screen_active());
  lv_subject_set_int(&calibration_on_start, !effective.calibrateOnStart);
  lv_obj_send_event(lv_obj_find_by_name(lv_screen_active(), "startup_calibration"), LV_EVENT_VALUE_CHANGED, nullptr);
  TEST_ASSERT_EQUAL_UINT(1, analyzerCommands);
  click(lv_obj_find_by_name(lv_screen_active(), "calibrate_air"));
  TEST_ASSERT_EQUAL(app::CommandType::CalibrateAir, lastCommandType);
  TEST_ASSERT_NOT_NULL(lv_obj_find_by_name(lv_screen_active(), "calibration_graph"));
  refresh();
  lv_obj_t* graph = lv_obj_find_by_name(lv_screen_active(), "calibration_graph");
  TEST_ASSERT_GREATER_OR_EQUAL(300, lv_obj_get_width(graph));
  TEST_ASSERT_EQUAL_INT(90, lv_obj_get_height(graph));
  app::Result progress;
  progress.type = app::CommandType::CalibrateAir;
  progress.complete = false;
  progress.calibrationPhase = app::CalibrationPhase::Settling;
  progress.calibrationMillivolts = 10.25f;
  progress.calibrationElapsedMs = 250;
  ui::presentCalibration(progress);
  TEST_ASSERT_EQUAL_STRING("10.25 mV", lv_subject_get_string(&calibration_current_text));
  TEST_ASSERT_EQUAL_STRING("0.2 s", lv_subject_get_string(&calibration_elapsed_text));
  lv_chart_series_t* series = lv_chart_get_series_next(graph, nullptr);
  TEST_ASSERT_TRUE(chartContains(graph, series, 1025));
  app::Result saved = progress;
  saved.complete = true;
  saved.calibrationPhase = app::CalibrationPhase::Saved;
  saved.calibrationMillivolts = 10.3f;
  saved.calibrationElapsedMs = 5000;
  ui::presentCalibration(saved);
  TEST_ASSERT_EQUAL_STRING("Saved", lv_subject_get_string(&calibration_stability_text));
  TEST_ASSERT_TRUE(lv_obj_has_flag(lv_obj_find_by_name(lv_screen_active(), "calibration_cancel"), LV_OBJ_FLAG_HIDDEN));
  TEST_ASSERT_FALSE(lv_obj_has_flag(lv_obj_find_by_name(lv_screen_active(), "calibration_done"), LV_OBJ_FLAG_HIDDEN));
  progress.calibrationMillivolts = 12;
  progress.calibrationElapsedMs = 5250;
  ui::presentCalibration(progress);
  TEST_ASSERT_EQUAL_STRING("10.30 mV", lv_subject_get_string(&calibration_current_text));
  TEST_ASSERT_TRUE(chartContains(graph, series, 1030));
  TEST_ASSERT_FALSE(chartContains(graph, series, 1200));
  click(lv_obj_find_by_name(lv_screen_active(), "calibration_done"));
  TEST_ASSERT_NOT_NULL(lv_obj_find_by_name(lv_screen_active(), "calibrate_o2"));
  click(lv_obj_find_by_name(lv_screen_active(), "calibrate_he"));
  TEST_ASSERT_EQUAL(app::CommandType::CalibrateHe, lastCommandType);
  app::Result invalid;
  invalid.type = app::CommandType::CalibrateHe;
  invalid.failure = app::Failure::Invalid;
  invalid.calibrationPhase = app::CalibrationPhase::Failed;
  invalid.calibrationMillivolts = 399;
  invalid.calibrationElapsedMs = 5000;
  ui::presentCalibration(invalid);
  lv_obj_t* stability = lv_obj_find_by_name(lv_screen_active(), "calibration_stability");
  TEST_ASSERT_EQUAL_STRING("Invalid value range. Check gas or sensor",
                           lv_subject_get_string(&calibration_stability_text));
  TEST_ASSERT_TRUE(lv_obj_has_state(stability, LV_STATE_USER_1));
  TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(COLOR_DANGER),
                           lv_color_to_u32(lv_obj_get_style_text_color(stability, LV_PART_MAIN)));
  click(lv_obj_find_by_name(lv_screen_active(), "calibration_done"));
  click(lv_obj_find_by_name(lv_screen_active(), "calibrate_he"));
  stability = lv_obj_find_by_name(lv_screen_active(), "calibration_stability");
  app::Result unstable = invalid;
  unstable.failure = app::Failure::Sampling;
  ui::presentCalibration(unstable);
  TEST_ASSERT_EQUAL_STRING("Unstable - not saved", lv_subject_get_string(&calibration_stability_text));
  TEST_ASSERT_TRUE(lv_obj_has_state(stability, LV_STATE_USER_1));
  click(lv_obj_find_by_name(lv_screen_active(), "calibration_done"));
  click(lv_obj_find_by_name(lv_screen_active(), "calibrate_he"));
  stability = lv_obj_find_by_name(lv_screen_active(), "calibration_stability");
  app::Result saveFailed = invalid;
  saveFailed.failure = app::Failure::Storage;
  ui::presentCalibration(saveFailed);
  TEST_ASSERT_EQUAL_STRING("Save failed", lv_subject_get_string(&calibration_stability_text));
  TEST_ASSERT_TRUE(lv_obj_has_state(stability, LV_STATE_USER_1));
  click(lv_obj_find_by_name(lv_screen_active(), "calibration_done"));
  click(lv_obj_find_by_name(lv_screen_active(), "calibrate_he"));
  stability = lv_obj_find_by_name(lv_screen_active(), "calibration_stability");
  TEST_ASSERT_EQUAL_STRING("Settling", lv_subject_get_string(&calibration_stability_text));
  TEST_ASSERT_FALSE(lv_obj_has_state(stability, LV_STATE_USER_1));
  click(lv_obj_find_by_name(lv_screen_active(), "calibration_cancel"));
  TEST_ASSERT_EQUAL_UINT(1, calibrationCancels);
  app::Result cancelled;
  cancelled.type = app::CommandType::CalibrateHe;
  cancelled.calibrationPhase = app::CalibrationPhase::Cancelled;
  ui::presentCalibration(cancelled);
  click(lv_obj_find_by_name(lv_screen_active(), "calibration_done"));
  click(lv_obj_find_by_name(lv_screen_active(), "open_diagnostics"));
  TEST_ASSERT_NOT_NULL(lv_obj_find_by_name(lv_screen_active(), "diagnostics_log"));
  click(lv_obj_find_by_name(lv_screen_active(), "diagnostics_back"));
  TEST_ASSERT_NOT_NULL(lv_obj_find_by_name(lv_screen_active(), "calibrate_o2"));
  click(lv_obj_find_by_name(lv_screen_active(), "calibration_back"));
  TEST_ASSERT_EQUAL_PTR(mainscr, lv_screen_active());
  click(lv_obj_find_by_name(mainscr, "readings"));
  TEST_ASSERT_EQUAL_PTR(largescr, lv_screen_active());
  click(largescr);
  TEST_ASSERT_EQUAL_PTR(mainscr, lv_screen_active());
}

void test_export_settings_round_trip_and_rejection() {
  effective.o2Air = 12;
  effective.heCalibration = 600;
  click(lv_obj_find_by_name(mainscr, "open_settings"));
  TEST_ASSERT_TRUE(settingsOpen);
  TEST_ASSERT_NOT_EQUAL(mainscr, lv_screen_active());
  TEST_ASSERT_EQUAL_INT(4, lv_subject_get_int(&settings_po2_bottom_index));
  lv_subject_set_int(&settings_po2_bottom_index, 3);
  lv_subject_set_int(&settings_sleep_index, 4);
  lv_subject_set_int(&settings_brightness, 200);
  lv_obj_send_event(lv_obj_find_by_name(lv_screen_active(), "brightness"), LV_EVENT_VALUE_CHANGED, nullptr);
  TEST_ASSERT_EQUAL_UINT8(200, brightness);
  click(lv_obj_find_by_name(lv_screen_active(), "open_updates"));
  TEST_ASSERT_NOT_NULL(lv_obj_find_by_name(lv_screen_active(), "wifi_names"));
  lv_obj_t* version = lv_obj_find_by_name(lv_screen_active(), "firmware_version");
  TEST_ASSERT_NOT_NULL(version);
  TEST_ASSERT_EQUAL_STRING(FIRMWARE_VERSION_TEXT, lv_label_get_text(version));
  click(lv_obj_find_by_name(lv_screen_active(), "scan_wifi"));
  TEST_ASSERT_EQUAL_UINT(1, wifiScanActions);
  TEST_ASSERT_TRUE(settingsOpen);
  click(lv_obj_find_by_name(lv_screen_active(), "update_back"));
  click(lv_obj_find_by_name(lv_screen_active(), "settings_back"));
  TEST_ASSERT_FALSE(settingsOpen);
  TEST_ASSERT_EQUAL_PTR(mainscr, lv_screen_active());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.3f, submitted.po2Bottom);
  TEST_ASSERT_EQUAL_UINT8(10, submitted.sleepMinutes);
  TEST_ASSERT_EQUAL_FLOAT(12, submitted.o2Air);
  TEST_ASSERT_EQUAL_FLOAT(600, submitted.heCalibration);
  allowSubmit = false;
  click(lv_obj_find_by_name(mainscr, "open_settings"));
  lv_subject_set_int(&settings_brightness, 220);
  click(lv_obj_find_by_name(lv_screen_active(), "settings_back"));
  TEST_ASSERT_EQUAL_UINT(1, errors);
  TEST_ASSERT_FALSE(settingsOpen);
  TEST_ASSERT_EQUAL_INT(effective.brightness, lv_subject_get_int(&settings_brightness));
  allowSubmit = true;
}

void test_export_readings_and_sensor_states() {
  sensorsData data;
  data.o2State = data.heState = data.coState = data.temperatureState = ChannelState::Valid;
  data.O2Level.percentage = 20.9f;
  data.O2Level.millivolts = 10.4f;
  data.HeLevel.percentage = 10.5f;
  data.HeLevel.millivolts = 80;
  data.CoLevel.ppm = 12;
  data.CoLevel.millivolts = 440;
  data.HeTemperature = 25;
  ui::presentReadings(data, effective);
  ui::presentStatus(data, false, true);
  TEST_ASSERT_EQUAL_STRING("20.9", lv_subject_get_string(&main_o2_text));
  lv_obj_t* o2Reading = lv_obj_find_by_name(mainscr, "main_o2_reading");
  lv_obj_t* o2Value = lv_obj_find_by_name(o2Reading, "primary_value");
  TEST_ASSERT_EQUAL_PTR(font_h2, lv_obj_get_style_text_font(o2Value, LV_PART_MAIN));
  lv_span_t* o2Unit = lv_spangroup_get_child(o2Value, 1);
  lv_style_value_t unitFont;
  TEST_ASSERT_NOT_NULL(o2Unit);
  TEST_ASSERT_TRUE(lv_style_get_prop(lv_span_get_style(o2Unit), LV_STYLE_TEXT_FONT, &unitFont));
  TEST_ASSERT_EQUAL_PTR(font_h5, unitFont.ptr);
  TEST_ASSERT_EQUAL_STRING("%", lv_span_get_text(o2Unit));
  TEST_ASSERT_EQUAL_STRING("10.4 mV", lv_subject_get_string(&main_o2_mv_text));
  TEST_ASSERT_EQUAL_STRING("B 56m", lv_subject_get_string(&main_mod_bottom_text));
  lv_obj_t* coReading = lv_obj_find_by_name(mainscr, "main_co_reading");
  TEST_ASSERT_NOT_NULL(coReading);
  lv_obj_t* coValue = lv_obj_find_by_name(coReading, "primary_value");
  lv_span_t* coUnit = lv_spangroup_get_child(coValue, 1);
  TEST_ASSERT_NOT_NULL(coValue);
  TEST_ASSERT_EQUAL_PTR(font_h2, lv_obj_get_style_text_font(coValue, LV_PART_MAIN));
  TEST_ASSERT_NOT_NULL(coUnit);
  TEST_ASSERT_TRUE(lv_obj_has_state(coValue, LV_STATE_USER_1));
  TEST_ASSERT_EQUAL_UINT32(lv_color_to_u32(COLOR_DANGER), lv_color_to_u32(lv_obj_get_style_text_color(coValue, LV_PART_MAIN)));
  lv_obj_t* warning = lv_obj_find_by_name(mainscr, "open_diagnostics");
  TEST_ASSERT_NOT_NULL(warning);
  TEST_ASSERT_TRUE(lv_obj_has_flag(warning, LV_OBJ_FLAG_HIDDEN));
  logUi("Test warning", UiLogLevel::Warning);
  ui::presentStatus(data, false, true);
  TEST_ASSERT_FALSE(lv_obj_has_flag(warning, LV_OBJ_FLAG_HIDDEN));
  click(warning);
  TEST_ASSERT_NOT_NULL(lv_obj_find_by_name(lv_screen_active(), "diagnostics_log"));
  click(lv_obj_find_by_name(lv_screen_active(), "diagnostics_back"));
  TEST_ASSERT_EQUAL_PTR(mainscr, lv_screen_active());
  UiLog::getInstance().clearLog();
  data.CoLevel.ppm = 0.9f;
  ui::presentReadings(data, effective);
  ui::presentStatus(data, false, true);
  TEST_ASSERT_EQUAL_STRING("0", lv_subject_get_string(&main_co_text));
  TEST_ASSERT_TRUE(lv_obj_has_flag(warning, LV_OBJ_FLAG_HIDDEN));
  TEST_ASSERT_FALSE(lv_obj_has_state(coValue, LV_STATE_USER_1));
  TEST_ASSERT_NOT_EQUAL(lv_color_to_u32(COLOR_DANGER), lv_color_to_u32(lv_obj_get_style_text_color(coValue, LV_PART_MAIN)));
  data.coState = ChannelState::Warming;
  data.heState = ChannelState::Warming;
  data.CoLevel.millivolts = 440;
  data.HeLevel.millivolts = 250;
  ui::presentReadings(data, effective);
  TEST_ASSERT_EQUAL_STRING("Warming", lv_subject_get_string(&main_co_text));
  TEST_ASSERT_EQUAL_PTR(font_h4, lv_obj_get_style_text_font(coValue, LV_PART_MAIN));
  TEST_ASSERT_EQUAL_STRING("", lv_span_get_text(coUnit));
  TEST_ASSERT_EQUAL_STRING("440.0 mV", lv_subject_get_string(&main_co_mv_text));
  TEST_ASSERT_EQUAL_STRING("Warming", lv_subject_get_string(&main_he_text));
  TEST_ASSERT_EQUAL_STRING("250.0 mV", lv_subject_get_string(&main_he_mv_text));
  data.coState = ChannelState::Valid;
  data.heState = ChannelState::Valid;
  data.o2State = ChannelState::Invalid;
  ui::presentReadings(data, effective);
  TEST_ASSERT_EQUAL_STRING("10.4 mV", lv_subject_get_string(&main_o2_mv_text));
  TEST_ASSERT_EQUAL_STRING("B --m", lv_subject_get_string(&main_mod_bottom_text));
  ui::presentReadings(data.forDisplay(sensorsData::FRESHNESS_MS), effective);
  TEST_ASSERT_EQUAL_STRING("Stale", lv_subject_get_string(&main_o2_text));
  TEST_ASSERT_EQUAL_PTR(font_h4, lv_obj_get_style_text_font(o2Value, LV_PART_MAIN));
  TEST_ASSERT_EQUAL_STRING("Stale", lv_subject_get_string(&main_o2_mv_text));
  lv_obj_t* value = lv_obj_find_by_name(largescr, "large_o2_value");
  TEST_ASSERT_EQUAL_PTR(font_body, lv_obj_get_style_text_font(value, LV_PART_MAIN));
  TEST_ASSERT_EQUAL_STRING("", lv_span_get_text(o2Unit));
  data.o2State = ChannelState::Valid;
  data.coState = ChannelState::Disabled;
  data.temperatureState = ChannelState::Unavailable;
  ui::presentReadings(data, effective);
  TEST_ASSERT_EQUAL_PTR(font_h2, lv_obj_get_style_text_font(o2Value, LV_PART_MAIN));
  TEST_ASSERT_EQUAL_PTR(font_h1, lv_obj_get_style_text_font(value, LV_PART_MAIN));
  TEST_ASSERT_EQUAL_STRING("%", lv_span_get_text(o2Unit));
  TEST_ASSERT_EQUAL_STRING("Off", lv_subject_get_string(&main_co_text));
  TEST_ASSERT_EQUAL_STRING("Unavailable", lv_subject_get_string(&main_he_temperature_text));
  TEST_ASSERT_EQUAL_STRING("10.5", lv_subject_get_string(&main_he_text));
  refresh();
}

void test_export_settings_lifetime() {
  lv_mem_monitor_t before;
  lv_mem_monitor(&before);
  for (unsigned cycle = 0; cycle < 20; ++cycle) {
    click(lv_obj_find_by_name(mainscr, "open_settings"));
    click(lv_obj_find_by_name(lv_screen_active(), "settings_back"));
  }
  lv_mem_monitor_t after;
  lv_mem_monitor(&after);
  TEST_ASSERT_TRUE(after.free_size >= before.free_size);
  TEST_ASSERT_EQUAL(LV_RESULT_OK, lv_mem_test());
}

int main(int, char**) {
  UNITY_BEGIN();
  lv_init();
  lv_display_t* display = lv_display_create(320, 170);
  alignas(LV_DRAW_BUF_ALIGN) static uint16_t pixels[320 * 17];
  lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
  lv_display_set_buffers(display, pixels, nullptr, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(display, flush);
  ui::init();
  ui::syncSettings(effective);
  RUN_TEST(test_export_boot_and_navigation);
  RUN_TEST(test_export_settings_round_trip_and_rejection);
  RUN_TEST(test_export_readings_and_sensor_states);
  RUN_TEST(test_export_settings_lifetime);
  return UNITY_END();
}