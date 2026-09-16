#include "UiAdapter.h"
#include "UiPresentation.h"
#include "lvgl_ui_project.h"
#include "main.h"
#include "ui-log.h"
#include "app/SleepPolicy.h"
#include <cstring>

namespace ui {
namespace {
lv_obj_t* settingsScreen = nullptr;

void copyText(lv_subject_t* subject, const char* text) {
  if (std::strcmp(lv_subject_get_string(subject), text) != 0) lv_subject_copy_string(subject, text);
}

void bindClick(lv_obj_t* object, lv_event_cb_t callback) {
  if (!object) {
    log_e("Exported UI object missing");
    return;
  }
  for (uint32_t index = lv_obj_get_event_count(object); index > 0; --index) {
    lv_event_dsc_t* descriptor = lv_obj_get_event_dsc(object, index - 1);
    if (lv_event_dsc_get_cb(descriptor) != lv_event_free_user_data_cb) lv_obj_remove_event(object, index - 1);
  }
  lv_obj_add_event_cb(object, callback, LV_EVENT_CLICKED, nullptr);
}

void switchReadings(lv_event_t* event) {
  lv_event_stop_bubbling(event);
  lv_screen_load(lv_screen_active() == mainscr ? largscr : mainscr);
  displayManager.resetInactivity();
}

void closeSettings(lv_event_t*) {
  AnalyzerSettings draft = uiSettings();
  const bool valid = readSettings(draft);
  const bool submitted = closeUiSettings(valid ? draft : uiSettings());
  lv_obj_t* previous = settingsScreen;
  settingsScreen = nullptr;
  lv_screen_load(mainscr);
  lv_obj_delete_async(previous);
  if (!valid) messageBox("Invalid settings - restored", NAN);
  else if (!submitted) messageBox("Settings not applied - analyzer busy", NAN);
}

void previewBrightness(lv_event_t*) {
  displayManager.setBrightness(static_cast<uint8_t>(lv_subject_get_int(&settings_brightness)));
}

void openSettings(lv_event_t*) {
  if (settingsScreen) return;
  openUiSettings();
  settingsScreen = settings_create();
  if (!settingsScreen) {
    closeUiSettings(uiSettings());
    messageBox("Could not open settings", NAN);
    return;
  }
  bindClick(lv_obj_find_by_name(settingsScreen, "settings_back"), closeSettings);
  bindClick(lv_obj_find_by_name(settingsScreen, "open_updates"), openUpdates);
  lv_obj_t* brightness = lv_obj_find_by_name(settingsScreen, "brightness");
  if (brightness) lv_obj_add_event_cb(brightness, previewBrightness, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_screen_load(settingsScreen);
}

ChannelState percentageState(float value, ChannelState state) {
  return state == ChannelState::Valid && (!std::isfinite(value) || value < 0 || value > 100)
      ? ChannelState::Invalid : state;
}

void setReading(lv_subject_t* subject, float value, ChannelState state, const char* format) {
  char text[48];
  formatValue(text, sizeof(text), value, state, format);
  copyText(subject, text);
}

void setRaw(lv_subject_t* subject, float value, ChannelState state) {
  const bool showRaw = std::isfinite(value) && state != ChannelState::Stale &&
                       state != ChannelState::Disabled && state != ChannelState::Unavailable;
  setReading(subject, value, showRaw ? ChannelState::Valid : state, "%.1f mV");
}
}

void init() {
  lvgl_ui_project_set_target(LVGL_UI_PROJECT_TARGET_TARGET1);
  lvgl_ui_project_init("");
  bindClick(mainscr, switchReadings);
  bindClick(largscr, switchReadings);
  bindClick(lv_obj_find_by_name(mainscr, "open_settings"), openSettings);
  bindClick(lv_obj_find_by_name(mainscr, "open_calibration"), openCalibration);
  presentReadings(sensorsData{}, AnalyzerSettings{});
  presentBattery(NAN);
  copyText(&main_status_text, "Starting");
  lv_screen_load(mainscr);
}

void syncSettings(const AnalyzerSettings& settings) {
  syncActionSettings(settings);
  lv_subject_set_int(&settings_o2_enabled, settings.o2Enabled);
  lv_subject_set_int(&settings_he_enabled, settings.heEnabled);
  lv_subject_set_int(&settings_co_enabled, settings.coEnabled);
  lv_subject_set_int(&settings_po2_bottom_index, po2Selection(settings.po2Bottom));
  lv_subject_set_int(&settings_po2_deco_index, po2Selection(settings.po2Deco));
  lv_subject_set_int(&settings_brightness, settings.brightness);
  lv_subject_set_int(&settings_sleep_index, app::sleepSelectionForMinutes(settings.sleepMinutes));
}

bool readSettings(AnalyzerSettings& settings) {
  AnalyzerSettings candidate = settings;
  candidate.o2Enabled = lv_subject_get_int(&settings_o2_enabled) != 0;
  candidate.heEnabled = lv_subject_get_int(&settings_he_enabled) != 0;
  candidate.coEnabled = lv_subject_get_int(&settings_co_enabled) != 0;
  if (!applySelections(candidate, lv_subject_get_int(&settings_po2_bottom_index),
                       lv_subject_get_int(&settings_po2_deco_index),
                       lv_subject_get_int(&settings_brightness), lv_subject_get_int(&settings_sleep_index))) return false;
  settings = candidate;
  return true;
}

void presentReadings(const sensorsData& data, const AnalyzerSettings& settings) {
  const ChannelState oxygen = percentageState(data.O2Level.percentage, data.o2State);
  const ChannelState helium = percentageState(data.HeLevel.percentage, data.heState);
  setReading(&main_o2_text, data.O2Level.percentage, oxygen, "%.1f%%");
  setReading(&main_he_text, data.HeLevel.percentage, helium, "%.1f%%");
  int co;
  const bool validCo = data.coState == ChannelState::Valid && conversions::toInt(data.CoLevel.ppm, co);
  setReading(&main_co_text, validCo ? static_cast<float>(co) : NAN, data.coState, "%.0f ppm");
  setRaw(&main_o2_mv_text, data.O2Level.millivolts, data.o2State);
  setRaw(&main_he_mv_text, data.HeLevel.millivolts, data.heState);
  setRaw(&main_co_mv_text, data.CoLevel.millivolts, data.coState);
  setReading(&main_he_temperature_text, data.HeTemperature, data.temperatureState, "%.1f \xC2\xB0" "C");
  char text[32];
  formatMod(text, sizeof(text), "B", settings.po2Bottom, data.O2Level.percentage, oxygen);
  copyText(&main_mod_bottom_text, text);
  formatMod(text, sizeof(text), "D", settings.po2Deco, data.O2Level.percentage, oxygen);
  copyText(&main_mod_deco_text, text);
  lv_obj_t* oxygenLabel = lv_obj_find_by_name(largscr, "large_o2_value");
  lv_obj_t* heliumLabel = lv_obj_find_by_name(largscr, "large_he_value");
  if (oxygenLabel) lv_obj_set_style_text_font(oxygenLabel, oxygen == ChannelState::Valid ? font_h1 : font_body, 0);
  if (heliumLabel) lv_obj_set_style_text_font(heliumLabel, helium == ChannelState::Valid ? font_h1 : font_body, 0);
}

void presentBattery(float voltage) {
  char text[24];
  if (std::isfinite(voltage)) std::snprintf(text, sizeof(text), "%.2f V", static_cast<double>(voltage));
  else std::snprintf(text, sizeof(text), "-- V");
  copyText(&main_battery_text, text);
}

void presentStatus(const sensorsData& data, bool busy, bool ready) {
  const bool coWarning = data.coState == ChannelState::Valid && std::isfinite(data.CoLevel.ppm) && data.CoLevel.ppm > 0;
  const UiLogLevel level = UiLog::getInstance().getLevel();
  const ChannelState oxygen = percentageState(data.O2Level.percentage, data.o2State);
  const ChannelState helium = percentageState(data.HeLevel.percentage, data.heState);
  const bool fault = (oxygen != ChannelState::Valid && oxygen != ChannelState::Disabled) ||
                     (helium != ChannelState::Valid && helium != ChannelState::Disabled) ||
                     (data.coState != ChannelState::Valid && data.coState != ChannelState::Disabled) ||
                     (data.heState != ChannelState::Disabled && data.temperatureState != ChannelState::Valid);
  const char* text = coWarning ? "CO warning" : !ready ? "Starting" : busy ? "Busy" :
                     level == UiLogLevel::Error ? "Error" : fault || level == UiLogLevel::Warning ? "Check sensors" : "Ready";
  copyText(&main_status_text, text);
  lv_obj_t* coLabel = lv_obj_find_by_name(largscr, "large_co_value");
  if (coLabel) lv_obj_set_style_text_color(coLabel, coWarning ? lv_palette_main(LV_PALETTE_RED) : COLOR_LIGHT_TEXT, 0);
}
}