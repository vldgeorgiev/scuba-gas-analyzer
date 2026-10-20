#include <unity.h>
#include "FirmwareVersion.h"
#include "lvgl_ui_project.h"
#include "display/UiAdapter.h"
#include "display/UiFeedback.h"
#include "main.h"
#include "network/FirmwareUpdate.h"
#include "ui-log.h"
#include <cstdlib>
#include <cstring>
#include <fstream>

DisplayManager displayManager;
static AnalyzerSettings effective;
static AnalyzerSettings submitted;
static bool settingsOpen;
static bool allowSubmit = true;
static unsigned wifiScanActions;
static unsigned analyzerCommands;
static unsigned calibrationCancels;
static app::CommandType lastCommandType;
static unsigned flushes;
static bool renderedPixels;
static uint8_t brightness;
static FirmwareUpdateEvent pendingUpdateEvent;
static bool updateEventPending = false;
static uint16_t renderedFrame[320 * 170];

void DisplayManager::setBrightness(uint8_t value) { brightness = value; }
void DisplayManager::resetInactivity() {}
const AnalyzerSettings& uiSettings() { return effective; }
bool submitAnalyzerCommand(app::Command command) {
  ++analyzerCommands;
  lastCommandType = command.type;
  submitted = command.settings;
  return allowSubmit;
}
bool cancelAnalyzerCalibration() { ++calibrationCancels; return true; }
void setNetworkOperationActive(bool) {}
void openUiSettings() { settingsOpen = true; ui::syncSettings(effective); }
bool closeUiSettings(const AnalyzerSettings& draft) {
  settingsOpen = false;
  submitted = draft;
  ui::syncSettings(effective);
  return allowSubmit;
}
bool firmwareUpdateBusy() { return false; }
bool startWifiScan() { ++wifiScanActions; return true; }
bool startFirmwareUpdate(const char*, const char*) { return true; }
bool pollFirmwareUpdateEvent(FirmwareUpdateEvent& event) {
  if (!updateEventPending) return false;
  event = pendingUpdateEvent;
  updateEventPending = false;
  return true;
}
void finishFirmwareUpdate() {}

static void flush(lv_display_t* display, const lv_area_t* area, uint8_t* pixels) {
  ++flushes;
  const unsigned bytes = lv_area_get_width(area) * lv_area_get_height(area) * 2;
  for (unsigned index = 0; index < bytes; ++index) renderedPixels |= pixels[index] != 0;
  for (int y = area->y1; y <= area->y2; ++y) {
    for (int x = area->x1; x <= area->x2; ++x) {
      const unsigned source = ((y - area->y1) * lv_area_get_width(area) + (x - area->x1)) * 2;
      renderedFrame[y * 320 + x] = static_cast<uint16_t>(pixels[source]) |
                                 (static_cast<uint16_t>(pixels[source + 1]) << 8);
    }
  }
  lv_display_flush_ready(display);
}

void setUp() {}
void tearDown() {}

static void refresh() {
  lv_tick_inc(40);
  lv_timer_handler();
  lv_refr_now(nullptr);
}

static void captureManualScreen(const char* name) {
  if (!std::getenv("MANUAL_SCREENSHOTS")) return;
  lv_obj_invalidate(lv_screen_active());
  refresh();
  char path[128];
  std::snprintf(path, sizeof(path), "docs/images/user-manual/%s.ppm", name);
  std::ofstream output(path, std::ios::binary);
  output << "P6\n320 170\n255\n";
  for (uint16_t pixel : renderedFrame) {
    const char rgb[] = {
        static_cast<char>(((pixel >> 11) & 0x1f) * 255 / 31),
        static_cast<char>(((pixel >> 5) & 0x3f) * 255 / 63),
        static_cast<char>((pixel & 0x1f) * 255 / 31),
    };
    output.write(rgb, sizeof(rgb));
  }
}

static void captureManualScreens() {
  sensorsData data;
  data.o2State = data.heState = data.coState = data.temperatureState = ChannelState::Valid;
  data.O2Level.percentage = 20.9f;
  data.O2Level.millivolts = 10.4f;
  data.HeLevel.percentage = 10.5f;
  data.HeLevel.millivolts = 80;
  data.CoLevel.ppm = 0;
  data.CoLevel.millivolts = 400;
  data.HeTemperature = 25;
  ui::presentReadings(data, effective);
  ui::presentStatus(data, false, true);
  lv_screen_load(mainscr);
  captureManualScreen("readings");

  lv_screen_load(largescr);
  captureManualScreen("large-readings");
  lv_screen_load(mainscr);

  ui::openCalibration(nullptr);
  captureManualScreen("calibration");
  lv_obj_send_event(lv_obj_find_by_name(lv_screen_active(), "calibrate_air"), LV_EVENT_CLICKED, nullptr);
  app::Result saved;
  saved.type = app::CommandType::CalibrateAir;
  saved.complete = true;
  saved.calibrationPhase = app::CalibrationPhase::Saved;
  saved.calibrationMillivolts = 10.3f;
  saved.calibrationElapsedMs = 5000;
  ui::presentCalibration(saved);
  captureManualScreen("calibration-complete");
  lv_obj_send_event(lv_obj_find_by_name(lv_screen_active(), "calibration_done"), LV_EVENT_CLICKED, nullptr);
  lv_obj_send_event(lv_obj_find_by_name(lv_screen_active(), "calibration_back"), LV_EVENT_CLICKED, nullptr);

  lv_obj_send_event(lv_obj_find_by_name(mainscr, "open_settings"), LV_EVENT_CLICKED, nullptr);
  lv_obj_send_event(lv_obj_find_by_name(lv_screen_active(), "open_updates"), LV_EVENT_CLICKED, nullptr);
  captureManualScreen("firmware-update");
}

static void click(lv_obj_t* object) {
  TEST_ASSERT_NOT_NULL(object);
  lv_obj_send_event(object, LV_EVENT_CLICKED, nullptr);
  refresh();
}

static void closeDialog(const char* title) {
  TEST_ASSERT_EQUAL_UINT(1, lv_obj_get_child_count(lv_layer_top()));
  lv_obj_t* backdrop = lv_obj_get_child(lv_layer_top(), -1);
  lv_obj_t* dialog = lv_obj_get_child(backdrop, 0);
  TEST_ASSERT_TRUE(lv_obj_check_type(dialog, &lv_msgbox_class));
  TEST_ASSERT_EQUAL_STRING(lv_tr(title), lv_label_get_text(lv_msgbox_get_title(dialog)));
  TEST_ASSERT_EQUAL_PTR(font_body, lv_obj_get_style_text_font(lv_msgbox_get_title(dialog), LV_PART_MAIN));
  lv_area_t titleArea;
  lv_area_t headerArea;
  lv_obj_update_layout(dialog);
  lv_obj_get_coords(lv_msgbox_get_title(dialog), &titleArea);
  lv_obj_get_coords(lv_msgbox_get_header(dialog), &headerArea);
  TEST_ASSERT_GREATER_OR_EQUAL(headerArea.x1, titleArea.x1);
  TEST_ASSERT_LESS_OR_EQUAL(headerArea.x2, titleArea.x2);
  lv_msgbox_close(dialog);
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
  TEST_ASSERT_FALSE(effective.calibrateOnStart);
  TEST_ASSERT_EQUAL_INT(0, lv_subject_get_int(&calibration_on_start));
  lv_obj_t* startup = lv_obj_find_by_name(lv_screen_active(), "startup_calibration");
  lv_subject_set_int(&calibration_on_start, 1);
  lv_obj_send_event(startup, LV_EVENT_VALUE_CHANGED, nullptr);
  TEST_ASSERT_EQUAL_UINT(1, analyzerCommands);
  TEST_ASSERT_TRUE(submitted.calibrateOnStart);
  refresh();
  lv_obj_t* backdrop = lv_obj_get_child(lv_layer_top(), -1);
  lv_obj_t* warning = lv_obj_get_child(backdrop, 0);
  TEST_ASSERT_TRUE(lv_obj_check_type(warning, &lv_msgbox_class));
  lv_obj_t* content = lv_msgbox_get_content(warning);
  lv_obj_t* text = lv_obj_get_child(content, 0);
  TEST_ASSERT_EQUAL_STRING(
      "Make sure the unit is flushed with air and there is no stale gas inside when starting",
      lv_label_get_text(text));
  TEST_ASSERT_GREATER_OR_EQUAL(0, lv_obj_get_x(warning));
  TEST_ASSERT_GREATER_OR_EQUAL(0, lv_obj_get_y(warning));
  TEST_ASSERT_LESS_OR_EQUAL(320, lv_obj_get_x(warning) + lv_obj_get_width(warning));
  TEST_ASSERT_LESS_OR_EQUAL(170, lv_obj_get_y(warning) + lv_obj_get_height(warning));
  TEST_ASSERT_EQUAL_INT(0, lv_obj_get_scroll_bottom(content));
  click(lv_obj_get_child(lv_msgbox_get_header(warning), -1));
  TEST_ASSERT_EQUAL_UINT(0, lv_obj_get_child_count(lv_layer_top()));
  lv_subject_set_int(&calibration_on_start, 0);
  lv_obj_send_event(startup, LV_EVENT_VALUE_CHANGED, nullptr);
  TEST_ASSERT_FALSE(submitted.calibrateOnStart);
  TEST_ASSERT_EQUAL_UINT(0, lv_obj_get_child_count(lv_layer_top()));
  allowSubmit = false;
  lv_subject_set_int(&calibration_on_start, 1);
  lv_obj_send_event(startup, LV_EVENT_VALUE_CHANGED, nullptr);
  TEST_ASSERT_EQUAL_INT(0, lv_subject_get_int(&calibration_on_start));
  closeDialog("Analyzer busy - preference not saved");
  TEST_ASSERT_EQUAL_UINT(0, lv_obj_get_child_count(lv_layer_top()));
  allowSubmit = true;
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
  TEST_ASSERT_EQUAL_INT(2, lv_subject_get_int(&settings_co_correction_index));
  lv_obj_t* coCorrection = lv_obj_find_by_name(lv_screen_active(), "co_correction");
  TEST_ASSERT_NOT_NULL(coCorrection);
  TEST_ASSERT_EQUAL_UINT32(8, lv_dropdown_get_option_count(coCorrection));
  lv_subject_set_int(&settings_po2_bottom_index, 3);
  lv_subject_set_int(&settings_co_correction_index, 5);
  char option[8];
  lv_dropdown_get_selected_str(coCorrection, option, sizeof(option));
  TEST_ASSERT_EQUAL_STRING("-3", option);
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
  TEST_ASSERT_EQUAL_INT8(-3, submitted.coCorrectionPpm);
  TEST_ASSERT_EQUAL_FLOAT(12, submitted.o2Air);
  TEST_ASSERT_EQUAL_FLOAT(600, submitted.heCalibration);
  allowSubmit = false;
  click(lv_obj_find_by_name(mainscr, "open_settings"));
  lv_subject_set_int(&settings_brightness, 220);
  click(lv_obj_find_by_name(lv_screen_active(), "settings_back"));
  closeDialog("Settings not applied - analyzer busy");
  TEST_ASSERT_FALSE(settingsOpen);
  TEST_ASSERT_EQUAL_INT(effective.brightness, lv_subject_get_int(&settings_brightness));
  allowSubmit = true;
}

void test_export_readings_and_sensor_states() {
  lv_obj_t* battery = lv_obj_find_by_name(mainscr, "battery_level");
  TEST_ASSERT_NOT_NULL(battery);
  const float batteryVoltages[] = {4.2f, 4.5f, 3.8f, 3.6f, 3.5f, 3.0f, 2.7f, NAN, INFINITY, 0.0f};
  const int batteryLevels[] = {100, 100, 60, 20, 10, 0, 0, 0, 0, 0};
  const uint32_t batteryColors[] = {0x34C759, 0x34C759, 0x34C759, 0xFF9500, 0xFF3B30,
                                   0xFF3B30, 0xFF3B30, 0x808080, 0x808080, 0x808080};
  for (size_t index = 0; index < sizeof(batteryVoltages) / sizeof(batteryVoltages[0]); ++index) {
    ui::presentBattery(batteryVoltages[index]);
    TEST_ASSERT_EQUAL_INT(batteryLevels[index], lv_bar_get_value(battery));
    TEST_ASSERT_EQUAL_HEX32(lv_color_to_u32(lv_color_hex(batteryColors[index])),
                            lv_color_to_u32(lv_obj_get_style_bg_color(battery, LV_PART_INDICATOR)));
  }
  TEST_ASSERT_EQUAL_STRING("-- V", lv_subject_get_string(&main_battery_text));
  ui::presentBattery(4.1f);
  TEST_ASSERT_EQUAL_STRING("4.10 V", lv_subject_get_string(&main_battery_text));
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
  lv_obj_t* heReading = lv_obj_find_by_name(mainscr, "main_he_reading");
  lv_obj_t* heValue = lv_obj_find_by_name(heReading, "primary_value");
  lv_span_t* heUnit = lv_spangroup_get_child(heValue, 1);
  data.O2Level.percentage = 101.2f;
  data.HeLevel.percentage = 104.6f;
  ui::presentReadings(data, effective);
  TEST_ASSERT_EQUAL_STRING("101.2", lv_subject_get_string(&main_o2_text));
  TEST_ASSERT_EQUAL_STRING("104.6", lv_subject_get_string(&main_he_text));
  TEST_ASSERT_EQUAL_STRING("%", lv_span_get_text(o2Unit));
  TEST_ASSERT_EQUAL_STRING("%", lv_span_get_text(heUnit));
  data.O2Level.percentage = 20.9f;
  data.HeLevel.percentage = 10.5f;
  ui::presentReadings(data, effective);
  lv_obj_t* coReading = lv_obj_find_by_name(mainscr, "main_co_reading");
  TEST_ASSERT_NOT_NULL(coReading);
  lv_obj_t* coValue = lv_obj_find_by_name(coReading, "primary_value");
  lv_span_t* coUnit = lv_spangroup_get_child(coValue, 1);
  TEST_ASSERT_NOT_NULL(coValue);
  TEST_ASSERT_EQUAL_PTR(font_h2, lv_obj_get_style_text_font(coValue, LV_PART_MAIN));
  TEST_ASSERT_NOT_NULL(coUnit);
  TEST_ASSERT_EQUAL_STRING("ppm", lv_span_get_text(coUnit));
  data.CoLevel.ppm = 625;
  ui::presentReadings(data, effective);
  TEST_ASSERT_EQUAL_STRING("625", lv_subject_get_string(&main_co_text));
  TEST_ASSERT_EQUAL_STRING("ppm", lv_span_get_text(coUnit));
  data.CoLevel.ppm = 12;
  ui::presentReadings(data, effective);
  lv_obj_t* largeCoUnit = lv_obj_find_by_name(largescr, "large_co_unit");
  TEST_ASSERT_NOT_NULL(largeCoUnit);
  TEST_ASSERT_EQUAL_STRING("ppm", lv_label_get_text(largeCoUnit));
  lv_obj_t* largeGases = lv_obj_find_by_name(largescr, "large_gases");
  lv_obj_t* largeStatus = lv_obj_find_by_name(largescr, "large_status");
  lv_obj_t* largeCoTitle = lv_obj_find_by_name(largescr, "large_co_title");
  lv_obj_t* largeCoValue = lv_obj_find_by_name(largescr, "large_co_value");
  lv_screen_load(largescr);
  refresh();
  TEST_ASSERT_LESS_OR_EQUAL(lv_obj_get_x(largeStatus), lv_obj_get_x2(largeGases));
  TEST_ASSERT_LESS_THAN(lv_obj_get_y(largeCoValue), lv_obj_get_y(largeCoTitle));
  TEST_ASSERT_LESS_THAN(lv_obj_get_y(largeCoUnit), lv_obj_get_y(largeCoValue));
  lv_screen_load(mainscr);
  refresh();
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
  TEST_ASSERT_TRUE(lv_obj_has_flag(largeCoUnit, LV_OBJ_FLAG_HIDDEN));
  TEST_ASSERT_EQUAL_STRING("440.0 mV", lv_subject_get_string(&main_co_mv_text));
  TEST_ASSERT_EQUAL_STRING("Warming", lv_subject_get_string(&main_he_text));
  TEST_ASSERT_EQUAL_STRING("250.0 mV", lv_subject_get_string(&main_he_mv_text));
  data.coState = ChannelState::Valid;
  data.heState = ChannelState::Valid;
  data.o2State = ChannelState::Invalid;
  ui::presentReadings(data, effective);
  TEST_ASSERT_FALSE(lv_obj_has_flag(largeCoUnit, LV_OBJ_FLAG_HIDDEN));
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
  TEST_ASSERT_TRUE(lv_obj_has_flag(coReading, LV_OBJ_FLAG_HIDDEN));
  TEST_ASSERT_TRUE(lv_obj_has_flag(lv_obj_find_by_name(largescr, "large_status"), LV_OBJ_FLAG_HIDDEN));
  data.coState = ChannelState::Valid;
  ui::presentReadings(data, effective);
  TEST_ASSERT_FALSE(lv_obj_has_flag(coReading, LV_OBJ_FLAG_HIDDEN));
  TEST_ASSERT_FALSE(lv_obj_has_flag(lv_obj_find_by_name(largescr, "large_status"), LV_OBJ_FLAG_HIDDEN));
  TEST_ASSERT_EQUAL_STRING("Unavailable", lv_subject_get_string(&main_he_temperature_text));
  TEST_ASSERT_EQUAL_STRING("10.5", lv_subject_get_string(&main_he_text));
  data.o2State = ChannelState::Disabled;
  data.heState = ChannelState::Disabled;
  ui::presentReadings(data, effective);
  TEST_ASSERT_TRUE(lv_obj_has_flag(o2Reading, LV_OBJ_FLAG_HIDDEN));
  TEST_ASSERT_TRUE(lv_obj_has_flag(lv_obj_find_by_name(mainscr, "main_he_reading"), LV_OBJ_FLAG_HIDDEN));
  TEST_ASSERT_TRUE(lv_obj_has_flag(lv_obj_find_by_name(largescr, "large_o2_row"), LV_OBJ_FLAG_HIDDEN));
  TEST_ASSERT_TRUE(lv_obj_has_flag(lv_obj_find_by_name(largescr, "large_he_row"), LV_OBJ_FLAG_HIDDEN));
  data.o2State = ChannelState::Valid;
  data.heState = ChannelState::Valid;
  ui::presentReadings(data, effective);
  TEST_ASSERT_FALSE(lv_obj_has_flag(o2Reading, LV_OBJ_FLAG_HIDDEN));
  TEST_ASSERT_FALSE(lv_obj_has_flag(lv_obj_find_by_name(mainscr, "main_he_reading"), LV_OBJ_FLAG_HIDDEN));
  refresh();
}

void test_export_translation_switching() {
  TEST_ASSERT_EQUAL_STRING("en", lv_translation_get_language());
  click(lv_obj_find_by_name(mainscr, "open_settings"));
  lv_obj_t* row = lv_obj_find_by_name(lv_screen_active(), "brightness_row");
  TEST_ASSERT_NOT_NULL(row);
  lv_obj_t* label = lv_obj_get_child(row, 0);
  TEST_ASSERT_TRUE(lv_obj_check_type(label, &lv_label_class));
  TEST_ASSERT_EQUAL_STRING("Brightness", lv_label_get_text(label));
  lv_translation_set_language("bg");
  refresh();
  TEST_ASSERT_EQUAL_STRING(u8"\u042f\u0440\u043a\u043e\u0441\u0442", lv_label_get_text(label));
  const lv_font_t* font = lv_obj_get_style_text_font(label, LV_PART_MAIN);
  const uint32_t letters[] = {0x042f, 0x0440, 0x043a, 0x043e, 0x0441, 0x0442};
  for (uint32_t letter : letters) {
    lv_font_glyph_dsc_t glyph = {};
    TEST_ASSERT_TRUE(lv_font_get_glyph_dsc(font, &glyph, letter, 0));
    TEST_ASSERT_FALSE(glyph.is_placeholder);
    TEST_ASSERT_GREATER_THAN(0, glyph.box_w);
    TEST_ASSERT_GREATER_THAN(0, glyph.box_h);
  }
  lv_obj_t* slider = lv_obj_find_by_name(row, "brightness");
  TEST_ASSERT_NOT_NULL(slider);
  TEST_ASSERT_LESS_OR_EQUAL(lv_obj_get_x(slider), lv_obj_get_x(label) + lv_obj_get_width(label));
  click(lv_obj_find_by_name(lv_screen_active(), "settings_back"));
  TEST_ASSERT_EQUAL_STRING("en", lv_translation_get_language());
  effective.language = app::LANGUAGE_BULGARIAN;
  ui::syncSettings(effective);
  click(lv_obj_find_by_name(mainscr, "open_settings"));
  row = lv_obj_find_by_name(lv_screen_active(), "brightness_row");
  TEST_ASSERT_NOT_NULL(row);
  label = lv_obj_get_child(row, 0);
  TEST_ASSERT_EQUAL_STRING(u8"\u042f\u0440\u043a\u043e\u0441\u0442", lv_label_get_text(label));
  effective.language = app::LANGUAGE_ENGLISH;
  ui::syncSettings(effective);
  refresh();
  TEST_ASSERT_EQUAL_STRING("Brightness", lv_label_get_text(label));
  click(lv_obj_find_by_name(lv_screen_active(), "settings_back"));
}

static lv_obj_t* findLabelText(lv_obj_t* object, const char* text) {
  if (lv_obj_check_type(object, &lv_label_class) && std::strcmp(lv_label_get_text(object), text) == 0) {
    return object;
  }
  for (uint32_t index = 0; index < lv_obj_get_child_count(object); ++index) {
    lv_obj_t* label = findLabelText(lv_obj_get_child(object, index), text);
    if (label) return label;
  }
  return nullptr;
}

static void assertTranslatedCaption(lv_obj_t* screen, const char* tag) {
  const char* text = lv_tr(tag);
  TEST_ASSERT_NOT_EQUAL_MESSAGE(0, std::strcmp(text, tag), tag);
  lv_obj_t* label = findLabelText(screen, text);
  TEST_ASSERT_NOT_NULL_MESSAGE(label, tag);
  TEST_ASSERT_FALSE_MESSAGE(lv_obj_has_flag(label, LV_OBJ_FLAG_HIDDEN), tag);
  lv_area_t labelArea;
  lv_area_t parentArea;
  lv_obj_get_coords(label, &labelArea);
  lv_obj_get_coords(lv_obj_get_parent(label), &parentArea);
  TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(parentArea.x1, labelArea.x1, tag);
  TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(parentArea.y1, labelArea.y1, tag);
  TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(parentArea.x2, labelArea.x2, tag);
  TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(parentArea.y2, labelArea.y2, tag);
}

void test_export_translated_screens() {
  const lv_font_t* fonts[] = {font_control, font_body, font_h4, font_h5};
  for (const lv_font_t* font : fonts) {
    for (uint32_t letter = 0x0410; letter <= 0x044f; ++letter) {
      lv_font_glyph_dsc_t glyph = {};
      TEST_ASSERT_TRUE(lv_font_get_glyph_dsc(font, &glyph, letter, 0));
      TEST_ASSERT_FALSE(glyph.is_placeholder);
    }
  }
  struct ScreenTranslations {
    lv_obj_t* (*create)();
    const char* tags[8];
  };
  const ScreenTranslations screens[] = {
      {settings_create, {"settings_language", "settings_co_correction", "settings_po2_bottom", "settings_po2_deco",
                         "settings_brightness", "settings_sleep", "settings_firmware_update", nullptr}},
      {calibration_create, {"calibration_title", "calibration_on_start", "calibration_air",
                            "calibration_clear_o2", nullptr}},
      {calibration_run_create, {"calibration_cancel", "calibration_done", nullptr}},
      {diagnostics_create, {"diagnostics_title", nullptr}},
      {firmware_update_create, {"update_install", nullptr}},
  };
  for (const ScreenTranslations& entry : screens) {
    lv_obj_t* screen = entry.create();
    TEST_ASSERT_NOT_NULL(screen);
    lv_screen_load(screen);
    refresh();
    const char* english[8] = {};
    for (unsigned index = 0; entry.tags[index]; ++index) {
      english[index] = lv_tr(entry.tags[index]);
      assertTranslatedCaption(screen, entry.tags[index]);
    }
    lv_translation_set_language("bg");
    refresh();
    for (unsigned index = 0; entry.tags[index]; ++index) {
      TEST_ASSERT_NOT_EQUAL_MESSAGE(0, std::strcmp(english[index], lv_tr(entry.tags[index])), entry.tags[index]);
      assertTranslatedCaption(screen, entry.tags[index]);
    }
    lv_translation_set_language("en");
    refresh();
    for (unsigned index = 0; entry.tags[index]; ++index) {
      assertTranslatedCaption(screen, entry.tags[index]);
    }
    lv_screen_load(mainscr);
    lv_obj_delete(screen);
    refresh();
  }
}

void test_effective_language_sync() {
  AnalyzerSettings settings = effective;
  settings.language = app::LANGUAGE_BULGARIAN;
  ui::syncSettings(settings);
  TEST_ASSERT_EQUAL_STRING("bg", lv_translation_get_language());
  TEST_ASSERT_EQUAL_INT(app::LANGUAGE_BULGARIAN, lv_subject_get_int(&settings_language_index));
  lv_obj_t* screen = settings_create();
  TEST_ASSERT_NOT_NULL(screen);
  lv_screen_load(screen);
  refresh();
  assertTranslatedCaption(screen, "settings_brightness");
  ui::syncSettings(effective);
  TEST_ASSERT_EQUAL_STRING("en", lv_translation_get_language());
  TEST_ASSERT_EQUAL_INT(app::LANGUAGE_ENGLISH, lv_subject_get_int(&settings_language_index));
  assertTranslatedCaption(screen, "settings_brightness");
  lv_screen_load(mainscr);
  lv_obj_delete(screen);
  refresh();
}

void test_export_language_selection_save_and_rejection() {
  click(lv_obj_find_by_name(mainscr, "open_settings"));
  lv_obj_t* language = lv_obj_find_by_name(lv_screen_active(), "language");
  TEST_ASSERT_NOT_NULL(language);
  TEST_ASSERT_EQUAL_UINT32(2, lv_dropdown_get_option_count(language));
  TEST_ASSERT_EQUAL_UINT32(app::LANGUAGE_ENGLISH, lv_dropdown_get_selected(language));
  lv_dropdown_set_selected(language, app::LANGUAGE_BULGARIAN);
  lv_obj_send_event(language, LV_EVENT_VALUE_CHANGED, nullptr);
  refresh();
  TEST_ASSERT_EQUAL_STRING("bg", lv_translation_get_language());
  TEST_ASSERT_EQUAL_INT(app::LANGUAGE_BULGARIAN, lv_subject_get_int(&settings_language_index));
  TEST_ASSERT_EQUAL_UINT8(app::LANGUAGE_ENGLISH, effective.language);
  assertTranslatedCaption(lv_screen_active(), "settings_language");
  lv_obj_t* row = lv_obj_find_by_name(lv_screen_active(), "language_row");
  TEST_ASSERT_NOT_NULL(row);
  lv_obj_t* label = lv_obj_get_child(row, 0);
  TEST_ASSERT_LESS_OR_EQUAL(lv_obj_get_x(language), lv_obj_get_x(label) + lv_obj_get_width(label));
  char option[32];
  lv_dropdown_get_selected_str(language, option, sizeof(option));
  TEST_ASSERT_EQUAL_STRING(u8"\u0411\u044a\u043b\u0433\u0430\u0440\u0441\u043a\u0438", option);
  click(lv_obj_find_by_name(lv_screen_active(), "settings_back"));
  TEST_ASSERT_EQUAL_UINT8(app::LANGUAGE_BULGARIAN, submitted.language);
  TEST_ASSERT_EQUAL_STRING("en", lv_translation_get_language());
  TEST_ASSERT_EQUAL_INT(app::LANGUAGE_ENGLISH, lv_subject_get_int(&settings_language_index));
  effective = submitted;
  ui::syncSettings(effective);
  TEST_ASSERT_EQUAL_STRING("bg", lv_translation_get_language());
  click(lv_obj_find_by_name(mainscr, "open_settings"));
  language = lv_obj_find_by_name(lv_screen_active(), "language");
  TEST_ASSERT_NOT_NULL(language);
  TEST_ASSERT_EQUAL_UINT32(app::LANGUAGE_BULGARIAN, lv_dropdown_get_selected(language));
  lv_dropdown_set_selected(language, app::LANGUAGE_ENGLISH);
  lv_obj_send_event(language, LV_EVENT_VALUE_CHANGED, nullptr);
  refresh();
  TEST_ASSERT_EQUAL_STRING("en", lv_translation_get_language());
  allowSubmit = false;
  click(lv_obj_find_by_name(lv_screen_active(), "settings_back"));
  allowSubmit = true;
  closeDialog("Settings not applied - analyzer busy");
  TEST_ASSERT_EQUAL_UINT8(app::LANGUAGE_ENGLISH, submitted.language);
  TEST_ASSERT_EQUAL_UINT8(app::LANGUAGE_BULGARIAN, effective.language);
  TEST_ASSERT_EQUAL_STRING("bg", lv_translation_get_language());
  TEST_ASSERT_EQUAL_INT(app::LANGUAGE_BULGARIAN, lv_subject_get_int(&settings_language_index));
  click(lv_obj_find_by_name(mainscr, "open_settings"));
  language = lv_obj_find_by_name(lv_screen_active(), "language");
  TEST_ASSERT_NOT_NULL(language);
  TEST_ASSERT_EQUAL_UINT32(app::LANGUAGE_BULGARIAN, lv_dropdown_get_selected(language));
  lv_dropdown_set_selected(language, app::LANGUAGE_ENGLISH);
  lv_obj_send_event(language, LV_EVENT_VALUE_CHANGED, nullptr);
  click(lv_obj_find_by_name(lv_screen_active(), "settings_back"));
  TEST_ASSERT_EQUAL_STRING("bg", lv_translation_get_language());
  ui::syncSettings(effective);
  TEST_ASSERT_EQUAL_STRING("bg", lv_translation_get_language());
  effective = submitted;
  ui::syncSettings(effective);
  TEST_ASSERT_EQUAL_STRING("en", lv_translation_get_language());
  TEST_ASSERT_EQUAL_INT(app::LANGUAGE_ENGLISH, lv_subject_get_int(&settings_language_index));
}

void test_runtime_reading_and_calibration_translations() {
  for (uint8_t language : {app::LANGUAGE_ENGLISH, app::LANGUAGE_BULGARIAN}) {
    effective.language = language;
    ui::syncSettings(effective);
    sensorsData data;
    for (ChannelState state : {ChannelState::Disabled, ChannelState::Warming, ChannelState::Unavailable,
                               ChannelState::Invalid, ChannelState::Stale}) {
      data.o2State = data.heState = data.coState = data.temperatureState = state;
      ui::presentReadings(data, effective);
      const char* expected = lv_tr(channelStateText(state));
      TEST_ASSERT_EQUAL_STRING(expected, lv_subject_get_string(&main_o2_text));
      TEST_ASSERT_EQUAL_STRING(expected, lv_subject_get_string(&main_he_text));
      TEST_ASSERT_EQUAL_STRING(expected, lv_subject_get_string(&main_co_text));
      TEST_ASSERT_EQUAL_STRING(expected, lv_subject_get_string(&main_he_temperature_text));
    }
    ui::openCalibration(nullptr);
    const app::Failure failures[] = {app::Failure::None, app::Failure::Invalid,
                                     app::Failure::Storage, app::Failure::Sampling, app::Failure::Cancelled};
    const char* statuses[] = {"Saved", "Invalid value range. Check gas or sensor", "Save failed",
                              "Unstable - not saved", "Cancelled"};
    for (unsigned index = 0; index < sizeof(failures) / sizeof(failures[0]); ++index) {
      click(lv_obj_find_by_name(lv_screen_active(), "calibrate_air"));
      TEST_ASSERT_EQUAL_STRING(lv_tr("Air calibration"), lv_subject_get_string(&calibration_run_title_text));
      TEST_ASSERT_EQUAL_STRING(lv_tr("Settling"), lv_subject_get_string(&calibration_stability_text));
      app::Result result;
      result.type = app::CommandType::CalibrateAir;
      result.complete = false;
      result.calibrationPhase = app::CalibrationPhase::Stable;
      result.calibrationMillivolts = 10;
      result.calibrationElapsedMs = 5000;
      ui::presentCalibration(result);
      TEST_ASSERT_EQUAL_STRING(lv_tr("Stable"), lv_subject_get_string(&calibration_stability_text));
      result.complete = true;
      result.failure = failures[index];
      result.calibrationPhase = result.failure == app::Failure::None ? app::CalibrationPhase::Saved
          : result.failure == app::Failure::Cancelled ? app::CalibrationPhase::Cancelled : app::CalibrationPhase::Failed;
      ui::presentCalibration(result);
      TEST_ASSERT_EQUAL_STRING(lv_tr(statuses[index]), lv_subject_get_string(&calibration_stability_text));
      AnalyzerSettings preview = effective;
      preview.language = language == app::LANGUAGE_ENGLISH ? app::LANGUAGE_BULGARIAN : app::LANGUAGE_ENGLISH;
      ui::syncSettings(preview);
      TEST_ASSERT_EQUAL_STRING(lv_tr(statuses[index]), lv_subject_get_string(&calibration_stability_text));
      TEST_ASSERT_EQUAL_STRING(lv_tr("Stale"), lv_subject_get_string(&main_o2_text));
      ui::syncSettings(effective);
      click(lv_obj_find_by_name(lv_screen_active(), "calibration_done"));
    }
    click(lv_obj_find_by_name(lv_screen_active(), "calibration_back"));
  }
  effective.language = app::LANGUAGE_ENGLISH;
  ui::syncSettings(effective);
}

void test_runtime_network_and_option_translations() {
  for (uint8_t language : {app::LANGUAGE_ENGLISH, app::LANGUAGE_BULGARIAN}) {
    effective.language = language;
    ui::syncSettings(effective);
    click(lv_obj_find_by_name(mainscr, "open_settings"));
    lv_obj_t* sleep = lv_obj_find_by_name(lv_screen_active(), "sleep_timeout");
    TEST_ASSERT_NOT_NULL(sleep);
    TEST_ASSERT_EQUAL_STRING(lv_tr("sleep_options"), lv_dropdown_get_options(sleep));
    TEST_ASSERT_EQUAL_INT(3, lv_dropdown_get_selected(sleep));
    click(lv_obj_find_by_name(lv_screen_active(), "open_updates"));
    lv_obj_t* password = lv_obj_find_by_name(lv_screen_active(), "wifi_password");
    lv_obj_t* networks = lv_obj_find_by_name(lv_screen_active(), "wifi_names");
    TEST_ASSERT_NOT_NULL(password);
    TEST_ASSERT_NOT_NULL(networks);
    TEST_ASSERT_EQUAL_STRING(lv_tr("Wi-Fi password"), lv_textarea_get_placeholder_text(password));
    TEST_ASSERT_EQUAL_STRING(lv_tr("No networks"), lv_dropdown_get_text(networks));
    TEST_ASSERT_EQUAL_UINT(0, lv_dropdown_get_option_count(networks));
    TEST_ASSERT_EQUAL_STRING(lv_tr("Not connected"), lv_subject_get_string(&update_status_text));
    click(lv_obj_find_by_name(lv_screen_active(), "scan_wifi"));
    TEST_ASSERT_EQUAL_STRING(lv_tr("Scanning Wi-Fi..."), lv_subject_get_string(&update_status_text));
    pendingUpdateEvent = {};
    pendingUpdateEvent.type = FirmwareUpdateEventType::ScanComplete;
    updateEventPending = true;
    ui::serviceFirmwareUpdate();
    TEST_ASSERT_EQUAL_STRING(lv_tr("No networks found"), lv_dropdown_get_text(networks));
    pendingUpdateEvent.networkCount = 1;
    std::snprintf(pendingUpdateEvent.networks[0], sizeof(pendingUpdateEvent.networks[0]), "Test network");
    updateEventPending = true;
    ui::serviceFirmwareUpdate();
    TEST_ASSERT_EQUAL_STRING("Test network", lv_dropdown_get_options(networks));
    TEST_ASSERT_EQUAL_STRING(lv_tr("Select a network and enter its password"), lv_subject_get_string(&update_status_text));
    const FirmwareUpdateEventType events[] = {FirmwareUpdateEventType::TimeSyncStarted,
        FirmwareUpdateEventType::DownloadStarted, FirmwareUpdateEventType::UpdateComplete,
        FirmwareUpdateEventType::ConnectionFailed, FirmwareUpdateEventType::TimeSyncFailed,
        FirmwareUpdateEventType::SecureConnectionFailed, FirmwareUpdateEventType::UpdateFailed};
    const char* statuses[] = {"Setting device time...", "Downloading firmware... 0%", "Update complete - restarting...",
        "Wi-Fi connection failed", "Could not set time - update cancelled", "Secure connection failed", "Firmware update failed"};
    const char* dialogs[] = {nullptr, nullptr, nullptr, "Wi-Fi connection failed", "Could not set device time",
                             "Secure connection failed", "Firmware update failed"};
    for (unsigned index = 0; index < sizeof(events) / sizeof(events[0]); ++index) {
      pendingUpdateEvent.type = events[index];
      updateEventPending = true;
      ui::serviceFirmwareUpdate();
      refresh();
      TEST_ASSERT_EQUAL_STRING(lv_tr(statuses[index]), lv_subject_get_string(&update_status_text));
      if (dialogs[index]) closeDialog(dialogs[index]);
    }
    pendingUpdateEvent.type = FirmwareUpdateEventType::DownloadProgress;
    pendingUpdateEvent.progress = 37;
    updateEventPending = true;
    ui::serviceFirmwareUpdate();
    char expected[160];
    std::snprintf(expected, sizeof(expected), lv_tr("Downloading... %u%%"), 37U);
    TEST_ASSERT_EQUAL_STRING(expected, lv_subject_get_string(&update_status_text));
    AnalyzerSettings preview = effective;
    preview.language = language == app::LANGUAGE_ENGLISH ? app::LANGUAGE_BULGARIAN : app::LANGUAGE_ENGLISH;
    ui::syncSettings(preview);
    std::snprintf(expected, sizeof(expected), lv_tr("Downloading... %u%%"), 37U);
    TEST_ASSERT_EQUAL_STRING(expected, lv_subject_get_string(&update_status_text));
    TEST_ASSERT_EQUAL_STRING(lv_tr("Wi-Fi password"), lv_textarea_get_placeholder_text(password));
    TEST_ASSERT_EQUAL_STRING("Test network", lv_dropdown_get_options(networks));
    ui::syncSettings(effective);
    click(lv_obj_find_by_name(lv_screen_active(), "update_back"));
    click(lv_obj_find_by_name(lv_screen_active(), "settings_back"));
  }
  effective.language = app::LANGUAGE_ENGLISH;
  ui::syncSettings(effective);
}

void test_runtime_feedback_and_diagnostic_translations() {
  for (uint8_t language : {app::LANGUAGE_ENGLISH, app::LANGUAGE_BULGARIAN}) {
    effective.language = language;
    ui::syncSettings(effective);
    messageBox("Invalid saved settings - defaults loaded", 10.3f);
    refresh();
    lv_obj_t* backdrop = lv_obj_get_child(lv_layer_top(), -1);
    lv_obj_t* dialog = lv_obj_get_child(backdrop, 0);
    lv_obj_t* content = lv_msgbox_get_content(dialog);
    TEST_ASSERT_EQUAL_STRING("10.30 mV", lv_label_get_text(lv_obj_get_child(content, 0)));
    AnalyzerSettings dialogPreview = effective;
    dialogPreview.language = language == app::LANGUAGE_ENGLISH ? app::LANGUAGE_BULGARIAN : app::LANGUAGE_ENGLISH;
    ui::syncSettings(dialogPreview);
    closeDialog("Invalid saved settings - defaults loaded");
    ui::syncSettings(effective);
    const app::Failure failures[] = {app::Failure::Invalid, app::Failure::Storage, app::Failure::Sampling,
                                     app::Failure::LoadedDefaults, app::Failure::Busy, app::Failure::CalibrationRequired};
    const char* titles[] = {"Invalid settings or calibration", "Settings could not be saved", "Calibration read failed",
        "Invalid saved settings - defaults loaded", "Analyzer is preparing for sleep", "O2 calibration required"};
    for (unsigned index = 0; index < sizeof(failures) / sizeof(failures[0]); ++index) {
      app::Result result;
      result.type = app::CommandType::ApplySettings;
      result.failure = failures[index];
      showAnalyzerResult(result);
      refresh();
      closeDialog(titles[index]);
    }
    for (unsigned required = 1; required <= 3; ++required) {
      app::Result result;
      result.failure = app::Failure::CalibrationRequired;
      result.oxygenCalibrationRequired = (required & 1) != 0;
      result.heliumCalibrationRequired = (required & 2) != 0;
      showAnalyzerResult(result);
      closeDialog(result.calibrationRequiredMessage());
    }
    const app::CommandType commands[] = {app::CommandType::CalibrateAir, app::CommandType::CalibratePure,
                                         app::CommandType::CalibrateHe, app::CommandType::ClearPure};
    const char* successes[] = {"O2 Air Calibrated", "O2 100% Calibrated", "He 100% Calibrated", "Pure O2 calibration cleared"};
    for (unsigned index = 0; index < sizeof(commands) / sizeof(commands[0]); ++index) {
      app::Result result;
      result.type = commands[index];
      showAnalyzerResult(result);
      closeDialog(successes[index]);
    }
    app::Result cancelled;
    cancelled.failure = app::Failure::Cancelled;
    showAnalyzerResult(cancelled);
    TEST_ASSERT_EQUAL_UINT(0, lv_obj_get_child_count(lv_layer_top()));
    UiLog::getInstance().clearLog();
    ui::openLogs(nullptr);
    TEST_ASSERT_EQUAL_STRING(lv_tr("No log entries"), lv_subject_get_string(&diagnostics_log_text));
    click(lv_obj_find_by_name(lv_screen_active(), "diagnostics_back"));
    logUi("ADC unavailable", UiLogLevel::Warning);
    ui::openLogs(nullptr);
    char expected[256];
    std::snprintf(expected, sizeof(expected), "[0] %s\n", lv_tr("ADC unavailable"));
    TEST_ASSERT_EQUAL_STRING(expected, lv_subject_get_string(&diagnostics_log_text));
    AnalyzerSettings preview = effective;
    preview.language = language == app::LANGUAGE_ENGLISH ? app::LANGUAGE_BULGARIAN : app::LANGUAGE_ENGLISH;
    ui::syncSettings(preview);
    std::snprintf(expected, sizeof(expected), "[0] %s\n", lv_tr("ADC unavailable"));
    TEST_ASSERT_EQUAL_STRING(expected, lv_subject_get_string(&diagnostics_log_text));
    click(lv_obj_find_by_name(lv_screen_active(), "diagnostics_back"));
    ui::syncSettings(effective);
    logUi("O2 invalid: 1.0 mV; He temperature invalid: unavailable", UiLogLevel::Warning);
    ui::openLogs(nullptr);
    std::snprintf(expected, sizeof(expected), "[0] O2 %s: 1.0 mV; %s %s: %s\n",
        lv_tr("invalid"), lv_tr("He temperature"), lv_tr("invalid"), lv_tr("unavailable"));
    TEST_ASSERT_EQUAL_STRING(expected, lv_subject_get_string(&diagnostics_log_text));
    click(lv_obj_find_by_name(lv_screen_active(), "diagnostics_back"));
    UiLog::getInstance().clearLog();
  }
  effective.language = app::LANGUAGE_ENGLISH;
  ui::syncSettings(effective);
}

void test_compact_controls_and_calibration_fit() {
  for (uint8_t language : {app::LANGUAGE_ENGLISH, app::LANGUAGE_BULGARIAN}) {
    effective.language = language;
    ui::syncSettings(effective);
    ui::openCalibration(nullptr);
    refresh();
    lv_obj_t* screen = lv_screen_active();
    lv_obj_t* list = lv_obj_find_by_name(screen, "calibration_list");
    lv_obj_t* button = lv_obj_find_by_name(screen, "calibrate_he");
    TEST_ASSERT_NOT_NULL(list);
    TEST_ASSERT_NOT_NULL(button);
    TEST_ASSERT_EQUAL_INT(0, lv_obj_get_scroll_bottom(list));
    TEST_ASSERT_EQUAL_INT(0, lv_obj_get_scroll_bottom(screen));
    lv_area_t screenArea;
    lv_area_t buttonArea;
    lv_obj_get_coords(screen, &screenArea);
    lv_obj_get_coords(button, &buttonArea);
    TEST_ASSERT_LESS_OR_EQUAL(screenArea.y2, buttonArea.y2);
    lv_obj_t* caption = findLabelText(button, "He (100%)");
    TEST_ASSERT_NOT_NULL(caption);
    TEST_ASSERT_EQUAL_PTR(font_control, lv_obj_get_style_text_font(caption, LV_PART_MAIN));
    click(lv_obj_find_by_name(screen, "calibration_back"));
    click(lv_obj_find_by_name(mainscr, "open_settings"));
    lv_obj_t* dropdown = lv_obj_find_by_name(lv_screen_active(), "language");
    TEST_ASSERT_NOT_NULL(dropdown);
    TEST_ASSERT_EQUAL_PTR(font_control, lv_obj_get_style_text_font(dropdown, LV_PART_MAIN));
    lv_obj_t* options = lv_dropdown_get_list(dropdown);
    TEST_ASSERT_NOT_NULL(options);
    TEST_ASSERT_EQUAL_PTR(font_control, lv_obj_get_style_text_font(options, LV_PART_MAIN));
    lv_dropdown_open(dropdown);
    refresh();
    TEST_ASSERT_EQUAL_PTR(font_control, lv_obj_get_style_text_font(options, LV_PART_SELECTED));
    lv_dropdown_close(dropdown);
    click(lv_obj_find_by_name(lv_screen_active(), "open_updates"));
    lv_obj_t* password = lv_obj_find_by_name(lv_screen_active(), "wifi_password");
    TEST_ASSERT_NOT_NULL(password);
    TEST_ASSERT_EQUAL_PTR(font_control, lv_obj_get_style_text_font(password, LV_PART_MAIN));
    TEST_ASSERT_EQUAL_PTR(font_control, lv_obj_get_style_text_font(password, LV_PART_TEXTAREA_PLACEHOLDER));
    click(lv_obj_find_by_name(lv_screen_active(), "update_back"));
    click(lv_obj_find_by_name(lv_screen_active(), "settings_back"));
  }
  effective.language = app::LANGUAGE_ENGLISH;
  ui::syncSettings(effective);
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
  RUN_TEST(test_export_translation_switching);
  RUN_TEST(test_export_translated_screens);
  RUN_TEST(test_effective_language_sync);
  RUN_TEST(test_export_language_selection_save_and_rejection);
  RUN_TEST(test_runtime_reading_and_calibration_translations);
  RUN_TEST(test_runtime_network_and_option_translations);
  RUN_TEST(test_runtime_feedback_and_diagnostic_translations);
  RUN_TEST(test_compact_controls_and_calibration_fit);
  RUN_TEST(test_export_settings_lifetime);
  captureManualScreens();
  return UNITY_END();
}