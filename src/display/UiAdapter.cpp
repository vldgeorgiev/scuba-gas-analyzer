#include "UiAdapter.h"
#include "UiFeedback.h"
#include "UiPresentation.h"
#include "FirmwareVersion.h"
#include "lvgl_ui_project.h"
#include "main.h"
#include "network/FirmwareUpdate.h"
#include "ui-log.h"
#include "app/SleepPolicy.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace ui {
namespace {
lv_obj_t* settingsScreen = nullptr;
lv_obj_t* calibrationScreen = nullptr;
lv_obj_t* updateScreen = nullptr;
lv_obj_t* diagnosticsScreen = nullptr;
lv_obj_t* calibrationRunScreen = nullptr;
lv_obj_t* calibrationChart = nullptr;
lv_chart_series_t* calibrationSeries = nullptr;
lv_obj_t* calibrationCancel = nullptr;
lv_obj_t* calibrationDone = nullptr;
lv_obj_t* calibrationStability = nullptr;
lv_obj_t* mainWarning = nullptr;
lv_obj_t* mainO2Value = nullptr;
lv_obj_t* mainHeValue = nullptr;
lv_obj_t* mainCoValue = nullptr;
lv_obj_t* largeO2Value = nullptr;
lv_obj_t* largeHeValue = nullptr;
lv_obj_t* largeCoValue = nullptr;
float calibrationGraphMinimum = NAN;
float calibrationGraphMaximum = NAN;
uint32_t calibrationGraphElapsedMs = UINT32_MAX;
bool calibrationGraphFrozen = false;

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
  lv_screen_load(lv_screen_active() == mainscr ? largescr : mainscr);
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

void closeCalibration(lv_event_t*) {
  calibrationScreen = nullptr;
  lv_screen_load_anim(mainscr, LV_SCREEN_LOAD_ANIM_NONE, 0, 0, true);
}

void changeStartupCalibration(lv_event_t*) {
  app::Command command;
  command.settings = uiSettings();
  command.settings.calibrateOnStart = lv_subject_get_int(&calibration_on_start) != 0;
  if (!submitAnalyzerCommand(command)) messageBox("Analyzer busy - preference not saved", NAN);
  lv_subject_set_int(&calibration_on_start, uiSettings().calibrateOnStart);
}

void closeUpdates(lv_event_t*) {
  updateScreen = nullptr;
  lv_screen_load_anim(settingsScreen ? settingsScreen : mainscr, LV_SCREEN_LOAD_ANIM_NONE, 0, 0, true);
}

void closeDiagnostics(lv_event_t*) {
  diagnosticsScreen = nullptr;
  lv_screen_load_anim(calibrationScreen ? calibrationScreen : mainscr, LV_SCREEN_LOAD_ANIM_NONE, 0, 0, true);
}

void finishCalibrationRun(lv_event_t*) {
  lv_obj_t* previous = calibrationRunScreen;
  calibrationRunScreen = nullptr;
  calibrationChart = nullptr;
  calibrationSeries = nullptr;
  calibrationCancel = nullptr;
  calibrationDone = nullptr;
  calibrationStability = nullptr;
  calibrationGraphFrozen = false;
  lv_screen_load(calibrationScreen ? calibrationScreen : mainscr);
  lv_obj_delete(previous);
}

void cancelCalibrationRun(lv_event_t* event) {
  lv_obj_t* button = lv_event_get_target_obj(event);
  lv_obj_add_state(button, LV_STATE_DISABLED);
  copyText(&calibration_stability_text, "Cancelling");
  if (!cancelAnalyzerCalibration()) {
    lv_obj_remove_state(button, LV_STATE_DISABLED);
    copyText(&calibration_stability_text, "Cancel failed");
  }
}

void openCalibrationRun(app::CommandType type, const char* title) {
  app::Command command;
  command.type = type;
  if (!submitAnalyzerCommand(command)) {
    messageBox("Analyzer busy - try again", NAN);
    return;
  }
  copyText(&calibration_run_title_text, title);
  copyText(&calibration_current_text, "-- mV");
  copyText(&calibration_elapsed_text, "0.0 s");
  copyText(&calibration_stability_text, "Settling");
  calibrationRunScreen = calibration_run_create();
  if (!calibrationRunScreen) {
    cancelAnalyzerCalibration();
    messageBox("Could not open calibration graph", NAN);
    return;
  }
  calibrationChart = lv_obj_find_by_name(calibrationRunScreen, "calibration_graph");
  calibrationSeries = calibrationChart ? lv_chart_get_series_next(calibrationChart, nullptr) : nullptr;
  calibrationCancel = lv_obj_find_by_name(calibrationRunScreen, "calibration_cancel");
  calibrationDone = lv_obj_find_by_name(calibrationRunScreen, "calibration_done");
  calibrationStability = lv_obj_find_by_name(calibrationRunScreen, "calibration_stability");
  if (calibrationStability) {
    lv_obj_set_style_text_color(calibrationStability, COLOR_DANGER, LV_PART_MAIN | LV_STATE_USER_1);
    lv_obj_remove_state(calibrationStability, LV_STATE_USER_1);
  }
  calibrationGraphMinimum = NAN;
  calibrationGraphMaximum = NAN;
  calibrationGraphElapsedMs = UINT32_MAX;
  calibrationGraphFrozen = false;
  bindClick(calibrationCancel, cancelCalibrationRun);
  bindClick(calibrationDone, finishCalibrationRun);
  lv_screen_load(calibrationRunScreen);
}

void startAirCalibration(lv_event_t*) {
  openCalibrationRun(app::CommandType::CalibrateAir, "Air calibration");
}

void startPureCalibration(lv_event_t*) {
  openCalibrationRun(app::CommandType::CalibratePure, "Pure O2 calibration");
}

void startHeliumCalibration(lv_event_t*) {
  openCalibrationRun(app::CommandType::CalibrateHe, "Helium calibration");
}

void clearPureCalibration(lv_event_t*) {
  app::Command command;
  command.type = app::CommandType::ClearPure;
  if (!submitAnalyzerCommand(command)) messageBox("Analyzer busy - try again", NAN);
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

void setUnit(lv_obj_t* value, const char* unit, bool visible) {
  lv_span_t* span = value ? lv_spangroup_get_child(value, 1) : nullptr;
  if (span) lv_spangroup_set_span_text_static(value, span, visible ? unit : "");
}

void setRaw(lv_subject_t* subject, float value, ChannelState state) {
  const bool showRaw = std::isfinite(value) && state != ChannelState::Stale &&
                       state != ChannelState::Disabled && state != ChannelState::Unavailable;
  setReading(subject, value, showRaw ? ChannelState::Valid : state, "%.1f mV");
}

void finishFirmwareUpdateOperation() {
  finishFirmwareUpdate();
  setNetworkOperationActive(false);
}

void scanWifi(lv_event_t*) {
  if (firmwareUpdateBusy()) {
    messageBox("Network operation in progress", NAN);
    return;
  }
  if (!lv_obj_find_by_name(lv_screen_active(), "wifi_names")) return;
  if (!startWifiScan()) {
    messageBox("Could not start Wi-Fi scan", NAN);
    return;
  }
  setNetworkOperationActive(true);
  copyText(&update_status_text, "Scanning Wi-Fi...");
  lv_subject_set_int(&update_can_install, 0);
  log_i("Listing WiFi networks");
}

void updateFirmware(lv_event_t*) {
  if (firmwareUpdateBusy()) {
    messageBox("Network operation in progress", NAN);
    return;
  }
  lv_obj_t* screen = lv_screen_active();
  lv_obj_t* wifiNames = lv_obj_find_by_name(screen, "wifi_names");
  lv_obj_t* wifiPassword = lv_obj_find_by_name(screen, "wifi_password");
  if (!wifiNames || !wifiPassword || lv_dropdown_get_option_count(wifiNames) == 0) {
    messageBox("Select a Wi-Fi network first", NAN);
    return;
  }
  char selectedSSID[FIRMWARE_UPDATE_MAX_SSID_LENGTH + 1];
  lv_dropdown_get_selected_str(wifiNames, selectedSSID, sizeof(selectedSSID));
  if (!startFirmwareUpdate(selectedSSID, lv_textarea_get_text(wifiPassword))) {
    messageBox("Could not start firmware update", NAN);
    return;
  }
  log_i("Updating firmware");
  log_i("Free heap before OTA: %d", ESP.getFreeHeap());
  log_i("Selected SSID: %s", selectedSSID);
  setNetworkOperationActive(true);
  lv_subject_set_int(&update_can_install, 0);
  copyText(&update_status_text, "Connecting to Wi-Fi...");
}
}

void init() {
  lvgl_ui_project_set_target(LVGL_UI_PROJECT_TARGET_TARGET1);
  lvgl_ui_project_init("");
  mainWarning = lv_obj_find_by_name(mainscr, "open_diagnostics");
  lv_obj_t* mainO2Reading = lv_obj_find_by_name(mainscr, "main_o2_reading");
  mainO2Value = mainO2Reading ? lv_obj_find_by_name(mainO2Reading, "primary_value") : nullptr;
  lv_obj_t* mainHeReading = lv_obj_find_by_name(mainscr, "main_he_reading");
  mainHeValue = mainHeReading ? lv_obj_find_by_name(mainHeReading, "primary_value") : nullptr;
  lv_obj_t* mainCoReading = lv_obj_find_by_name(mainscr, "main_co_reading");
  mainCoValue = mainCoReading ? lv_obj_find_by_name(mainCoReading, "primary_value") : nullptr;
  largeO2Value = lv_obj_find_by_name(largescr, "large_o2_value");
  largeHeValue = lv_obj_find_by_name(largescr, "large_he_value");
  largeCoValue = lv_obj_find_by_name(largescr, "large_co_value");
  if (mainCoValue) lv_obj_set_style_text_color(mainCoValue, COLOR_DANGER, LV_PART_MAIN | LV_STATE_USER_1);
  if (largeCoValue) lv_obj_set_style_text_color(largeCoValue, COLOR_DANGER, LV_PART_MAIN | LV_STATE_USER_1);
  bindClick(mainscr, switchReadings);
  bindClick(largescr, switchReadings);
  bindClick(mainWarning, openLogs);
  bindClick(lv_obj_find_by_name(mainscr, "open_settings"), openSettings);
  bindClick(lv_obj_find_by_name(mainscr, "open_calibration"), openCalibration);
  presentReadings(sensorsData{}, AnalyzerSettings{});
  presentBattery(NAN);
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

void syncActionSettings(const AnalyzerSettings& settings) {
  lv_subject_set_int(&calibration_on_start, settings.calibrateOnStart);
}

void openCalibration(lv_event_t*) {
  if (calibrationScreen) return;
  syncActionSettings(uiSettings());
  calibrationScreen = calibration_create();
  if (!calibrationScreen) {
    messageBox("Could not open calibration", NAN);
    return;
  }
  bindClick(lv_obj_find_by_name(calibrationScreen, "calibration_back"), closeCalibration);
  bindClick(lv_obj_find_by_name(calibrationScreen, "calibrate_air"), startAirCalibration);
  bindClick(lv_obj_find_by_name(calibrationScreen, "calibrate_o2"), startPureCalibration);
  bindClick(lv_obj_find_by_name(calibrationScreen, "calibrate_he"), startHeliumCalibration);
  bindClick(lv_obj_find_by_name(calibrationScreen, "clear_o2"), clearPureCalibration);
  bindClick(lv_obj_find_by_name(calibrationScreen, "open_diagnostics"), openLogs);
  lv_obj_t* startup = lv_obj_find_by_name(calibrationScreen, "startup_calibration");
  if (startup) lv_obj_add_event_cb(startup, changeStartupCalibration, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_screen_load(calibrationScreen);
}

void openUpdates(lv_event_t*) {
  if (updateScreen) return;
  lv_subject_set_int(&update_can_install, 0);
  lv_subject_set_int(&update_keyboard_visible, 0);
  copyText(&firmware_version_text, FIRMWARE_VERSION_TEXT);
  copyText(&update_status_text, "Not connected");
  updateScreen = firmware_update_create();
  if (!updateScreen) {
    messageBox("Could not open firmware update", NAN);
    return;
  }
  bindClick(lv_obj_find_by_name(updateScreen, "update_back"), closeUpdates);
  bindClick(lv_obj_find_by_name(updateScreen, "scan_wifi"), scanWifi);
  bindClick(lv_obj_find_by_name(updateScreen, "install_firmware"), updateFirmware);
  lv_screen_load(updateScreen);
}

void openLogs(lv_event_t*) {
  if (diagnosticsScreen) return;
  char log[UiLog::kLogSnapshotSize];
  UiLog::getInstance().copyLogTo(log);
  copyText(&diagnostics_log_text, log[0] ? log : "No log entries");
  diagnosticsScreen = diagnostics_create();
  if (!diagnosticsScreen) {
    messageBox("Could not open diagnostics", NAN);
    return;
  }
  bindClick(lv_obj_find_by_name(diagnosticsScreen, "diagnostics_back"), closeDiagnostics);
  lv_screen_load(diagnosticsScreen);
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
  setReading(&main_o2_text, data.O2Level.percentage, oxygen, "%.1f");
  setReading(&main_he_text, data.HeLevel.percentage, helium, "%.1f");
  int co;
  const bool validCo = data.coState == ChannelState::Valid && conversions::toInt(data.CoLevel.ppm, co);
  setReading(&main_co_text, validCo ? static_cast<float>(co) : NAN, data.coState, "%.0f");
  setUnit(mainO2Value, "%", oxygen == ChannelState::Valid);
  setUnit(mainHeValue, "%", helium == ChannelState::Valid);
  setUnit(mainCoValue, "ppm", validCo);
  setUnit(largeO2Value, "%", oxygen == ChannelState::Valid);
  setUnit(largeHeValue, "%", helium == ChannelState::Valid);
  setUnit(largeCoValue, "ppm", validCo);
  if (mainO2Value) lv_obj_set_style_text_font(mainO2Value, oxygen == ChannelState::Valid ? font_h2 : font_h4, 0);
  if (mainHeValue) lv_obj_set_style_text_font(mainHeValue, helium == ChannelState::Valid ? font_h2 : font_h4, 0);
  if (mainCoValue) lv_obj_set_style_text_font(mainCoValue, validCo ? font_h2 : font_h4, 0);
  setRaw(&main_o2_mv_text, data.O2Level.millivolts, data.o2State);
  setRaw(&main_he_mv_text, data.HeLevel.millivolts, data.heState);
  setRaw(&main_co_mv_text, data.CoLevel.millivolts, data.coState);
  setReading(&main_he_temperature_text, data.HeTemperature, data.temperatureState, "%.1f \xC2\xB0" "C");
  char text[32];
  formatMod(text, sizeof(text), "B", settings.po2Bottom, data.O2Level.percentage, oxygen);
  copyText(&main_mod_bottom_text, text);
  formatMod(text, sizeof(text), "D", settings.po2Deco, data.O2Level.percentage, oxygen);
  copyText(&main_mod_deco_text, text);
  if (largeO2Value) lv_obj_set_style_text_font(largeO2Value, oxygen == ChannelState::Valid ? font_h1 : font_body, 0);
  if (largeHeValue) lv_obj_set_style_text_font(largeHeValue, helium == ChannelState::Valid ? font_h1 : font_body, 0);
}

void presentBattery(float voltage) {
  char text[24];
  if (std::isfinite(voltage)) std::snprintf(text, sizeof(text), "%.2f V", static_cast<double>(voltage));
  else std::snprintf(text, sizeof(text), "-- V");
  copyText(&main_battery_text, text);
}

void presentStatus(const sensorsData& data, bool, bool) {
  int displayedCo;
  const bool coWarning = data.coState == ChannelState::Valid &&
                         conversions::toInt(data.CoLevel.ppm, displayedCo) && displayedCo > 0;
  const UiLogLevel level = UiLog::getInstance().getLevel();
  if (mainWarning) lv_obj_set_flag(mainWarning, LV_OBJ_FLAG_HIDDEN, level == UiLogLevel::None);
  if (mainCoValue) {
    if (coWarning) lv_obj_add_state(mainCoValue, LV_STATE_USER_1);
    else lv_obj_remove_state(mainCoValue, LV_STATE_USER_1);
  }
  if (largeCoValue) {
    if (coWarning) lv_obj_add_state(largeCoValue, LV_STATE_USER_1);
    else lv_obj_remove_state(largeCoValue, LV_STATE_USER_1);
  }
}

void presentCalibration(const app::Result& result) {
  if (!calibrationRunScreen || calibrationGraphFrozen || result.calibrationPhase == app::CalibrationPhase::None) return;
  char text[32];
  if (std::isfinite(result.calibrationMillivolts)) {
    std::snprintf(text, sizeof(text), "%.2f mV", static_cast<double>(result.calibrationMillivolts));
  } else {
    std::snprintf(text, sizeof(text), "-- mV");
  }
  copyText(&calibration_current_text, text);
  std::snprintf(text, sizeof(text), "%.1f s", static_cast<double>(result.calibrationElapsedMs) / 1000.0);
  copyText(&calibration_elapsed_text, text);

  if (calibrationChart && calibrationSeries && std::isfinite(result.calibrationMillivolts) &&
      result.calibrationElapsedMs != calibrationGraphElapsedMs) {
    calibrationGraphElapsedMs = result.calibrationElapsedMs;
    calibrationGraphMinimum = std::isfinite(calibrationGraphMinimum)
        ? std::min(calibrationGraphMinimum, result.calibrationMillivolts) : result.calibrationMillivolts;
    calibrationGraphMaximum = std::isfinite(calibrationGraphMaximum)
        ? std::max(calibrationGraphMaximum, result.calibrationMillivolts) : result.calibrationMillivolts;
    const float margin = std::max(0.5f, (calibrationGraphMaximum - calibrationGraphMinimum) * 0.2f);
    lv_chart_set_axis_range(calibrationChart, LV_CHART_AXIS_PRIMARY_Y,
                            static_cast<int32_t>(std::floor((calibrationGraphMinimum - margin) * 100)),
                            static_cast<int32_t>(std::ceil((calibrationGraphMaximum + margin) * 100)));
    lv_chart_set_next_value(calibrationChart, calibrationSeries,
                            static_cast<int32_t>(std::lround(result.calibrationMillivolts * 100)));
  }

  const char* status = "Settling";
  const bool failed = result.calibrationPhase == app::CalibrationPhase::Failed;
  const bool outOfRange = failed && result.failure == app::Failure::Invalid;
  switch (result.calibrationPhase) {
    case app::CalibrationPhase::Stable: status = "Stable"; break;
    case app::CalibrationPhase::Saved: status = "Saved"; break;
    case app::CalibrationPhase::Cancelled: status = "Cancelled"; break;
    case app::CalibrationPhase::Failed:
      if (result.failure == app::Failure::Storage) status = "Save failed";
      else if (outOfRange) status = "Invalid value range. Check gas or sensor";
      else status = "Unstable - not saved";
      break;
    default: break;
  }
  copyText(&calibration_stability_text, status);
  if (calibrationStability) {
    if (failed) lv_obj_add_state(calibrationStability, LV_STATE_USER_1);
    else lv_obj_remove_state(calibrationStability, LV_STATE_USER_1);
  }
  if (result.complete) {
    calibrationGraphFrozen = true;
    if (calibrationCancel) lv_obj_add_flag(calibrationCancel, LV_OBJ_FLAG_HIDDEN);
    if (calibrationDone) lv_obj_remove_flag(calibrationDone, LV_OBJ_FLAG_HIDDEN);
  }
}

void serviceFirmwareUpdate() {
  FirmwareUpdateEvent event;
  if (!pollFirmwareUpdateEvent(event)) return;

  switch (event.type) {
    case FirmwareUpdateEventType::ScanComplete: {
      lv_obj_t* wifiNames = lv_obj_find_by_name(lv_screen_active(), "wifi_names");
      if (wifiNames) {
        lv_dropdown_clear_options(wifiNames);
        for (uint8_t index = 0; index < event.networkCount; ++index) {
          log_i("Found WiFi network: %s", event.networks[index]);
          lv_dropdown_add_option(wifiNames, event.networks[index], LV_DROPDOWN_POS_LAST);
        }
        lv_dropdown_set_text(wifiNames, event.networkCount ? nullptr : "No networks found");
      }
      lv_subject_set_int(&update_network_index, 0);
      lv_subject_set_int(&update_can_install, event.networkCount > 0);
      copyText(&update_status_text,
               event.networkCount ? "Select a network and enter its password" : "No networks found");
      finishFirmwareUpdateOperation();
      break;
    }
    case FirmwareUpdateEventType::TimeSyncStarted:
      copyText(&update_status_text, "Setting device time...");
      break;
    case FirmwareUpdateEventType::DownloadStarted:
      copyText(&update_status_text, "Downloading firmware... 0%");
      break;
    case FirmwareUpdateEventType::DownloadProgress: {
      char status[40];
      std::snprintf(status, sizeof(status), "Downloading... %u%%", event.progress);
      copyText(&update_status_text, status);
      break;
    }
    case FirmwareUpdateEventType::UpdateComplete:
      copyText(&update_status_text, "Update complete - restarting...");
      break;
    case FirmwareUpdateEventType::ConnectionFailed:
      copyText(&update_status_text, "Wi-Fi connection failed");
      lv_subject_set_int(&update_can_install, 1);
      finishFirmwareUpdateOperation();
      messageBox("Wi-Fi connection failed", NAN);
      break;
    case FirmwareUpdateEventType::TimeSyncFailed:
      copyText(&update_status_text, "Could not set time - update cancelled");
      lv_subject_set_int(&update_can_install, 1);
      finishFirmwareUpdateOperation();
      messageBox("Could not set device time", NAN);
      break;
    case FirmwareUpdateEventType::SecureConnectionFailed:
      copyText(&update_status_text, "Secure connection failed");
      lv_subject_set_int(&update_can_install, 1);
      finishFirmwareUpdateOperation();
      messageBox("Secure connection failed", NAN);
      break;
    case FirmwareUpdateEventType::UpdateFailed:
      copyText(&update_status_text, "Firmware update failed");
      lv_subject_set_int(&update_can_install, 1);
      finishFirmwareUpdateOperation();
      messageBox("Firmware update failed", NAN);
      break;
  }
}
}