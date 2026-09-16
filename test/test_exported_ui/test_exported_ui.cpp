#include <unity.h>
#include "lvgl_ui_project.h"
#include "display/UiAdapter.h"
#include "main.h"

DisplayManager displayManager;
static AnalyzerSettings effective;
static AnalyzerSettings submitted;
static bool settingsOpen;
static bool allowSubmit = true;
static unsigned errors;
static unsigned calibrationOpens;
static unsigned updatesOpens;
static unsigned flushes;
static bool renderedPixels;
static uint8_t brightness;

void DisplayManager::setBrightness(uint8_t value) { brightness = value; }
void DisplayManager::resetInactivity() {}
const AnalyzerSettings& uiSettings() { return effective; }
void openUiSettings() { settingsOpen = true; ui::syncSettings(effective); }
bool closeUiSettings(const AnalyzerSettings& draft) {
  settingsOpen = false;
  submitted = draft;
  ui::syncSettings(effective);
  return allowSubmit;
}
void messageBox(const char*, float) { ++errors; }
namespace ui {
void openCalibration(lv_event_t*) { ++calibrationOpens; }
void openUpdates(lv_event_t*) { ++updatesOpens; }
void openLogs(lv_event_t*) {}
void syncActionSettings(const AnalyzerSettings&) {}
}

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
  TEST_ASSERT_EQUAL_PTR(largscr, lv_screen_active());
  click(lv_obj_find_by_name(largscr, "large_he_row"));
  TEST_ASSERT_EQUAL_PTR(mainscr, lv_screen_active());
  click(lv_obj_find_by_name(mainscr, "open_calibration"));
  TEST_ASSERT_EQUAL_UINT(1, calibrationOpens);
  TEST_ASSERT_EQUAL_PTR(mainscr, lv_screen_active());
  click(lv_obj_find_by_name(mainscr, "status"));
  TEST_ASSERT_EQUAL_PTR(largscr, lv_screen_active());
  click(largscr);
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
  TEST_ASSERT_EQUAL_UINT(1, updatesOpens);
  TEST_ASSERT_TRUE(settingsOpen);
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
  TEST_ASSERT_EQUAL_STRING("20.9%", lv_subject_get_string(&main_o2_text));
  TEST_ASSERT_EQUAL_STRING("10.4 mV", lv_subject_get_string(&main_o2_mv_text));
  TEST_ASSERT_EQUAL_STRING("CO warning", lv_subject_get_string(&main_status_text));
  TEST_ASSERT_EQUAL_STRING("B 56 m", lv_subject_get_string(&main_mod_bottom_text));
  data.o2State = ChannelState::Invalid;
  ui::presentReadings(data, effective);
  TEST_ASSERT_EQUAL_STRING("10.4 mV", lv_subject_get_string(&main_o2_mv_text));
  TEST_ASSERT_EQUAL_STRING("B -- m", lv_subject_get_string(&main_mod_bottom_text));
  ui::presentReadings(data.forDisplay(sensorsData::FRESHNESS_MS), effective);
  TEST_ASSERT_EQUAL_STRING("Stale", lv_subject_get_string(&main_o2_text));
  TEST_ASSERT_EQUAL_STRING("Stale", lv_subject_get_string(&main_o2_mv_text));
  lv_obj_t* value = lv_obj_find_by_name(largscr, "large_o2_value");
  TEST_ASSERT_EQUAL_PTR(font_body, lv_obj_get_style_text_font(value, LV_PART_MAIN));
  data.o2State = ChannelState::Valid;
  data.coState = ChannelState::Disabled;
  data.temperatureState = ChannelState::Unavailable;
  ui::presentReadings(data, effective);
  TEST_ASSERT_EQUAL_PTR(font_h1, lv_obj_get_style_text_font(value, LV_PART_MAIN));
  TEST_ASSERT_EQUAL_STRING("Off", lv_subject_get_string(&main_co_text));
  TEST_ASSERT_EQUAL_STRING("Unavailable", lv_subject_get_string(&main_he_temperature_text));
  TEST_ASSERT_EQUAL_STRING("10.5%", lv_subject_get_string(&main_he_text));
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