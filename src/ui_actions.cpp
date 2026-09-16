#include <lvgl.h>
#include "display/UiAdapter.h"
#include "main.h"
#include "ui-log.h"
#include "pin_config.h"
#include "updater.h"
#include <HTTPClient.h>
#include <Update.h>

class NetworkOperation {
public:
  NetworkOperation() { setNetworkOperationActive(true); }
  ~NetworkOperation() { setNetworkOperationActive(false); }
};

static lv_obj_t* wifiNames = nullptr;
static lv_obj_t* wifiPassword = nullptr;
static lv_obj_t* startupCalibration = nullptr;
static lv_obj_t* updateKeyboard = nullptr;

void action_list_wifi(lv_event_t* event);
void action_update_firmware(lv_event_t* event);

void messageBox(const char * title, float value) {
  char text[32] = "";
  if (std::isfinite(value)) snprintf(text, sizeof(text), "%.2f mv", value);

  lv_obj_t * mbox = lv_msgbox_create(NULL);
  lv_msgbox_add_close_button(mbox);
  lv_msgbox_add_title(mbox, title);
  lv_msgbox_add_text(mbox, text);
  lv_obj_set_size(mbox, LV_PCT(70), LV_SIZE_CONTENT);
}

void action_calibrate_o2_21(lv_event_t * e) {
  app::Command command;
  command.type = app::CommandType::CalibrateAir;
  if (!submitAnalyzerCommand(command)) messageBox("Analyzer busy - try again", NAN);
}

void action_calibrate_o2_100(lv_event_t * e) {
  app::Command command;
  command.type = app::CommandType::CalibratePure;
  if (!submitAnalyzerCommand(command)) messageBox("Analyzer busy - try again", NAN);
}

void action_calibrate_he(lv_event_t * e) {
  app::Command command;
  command.type = app::CommandType::CalibrateHe;
  if (!submitAnalyzerCommand(command)) messageBox("Analyzer busy - try again", NAN);
}

void action_reset_o2_21(lv_event_t * e) {
  app::Command command;
  command.type = app::CommandType::ResetAir;
  if (!submitAnalyzerCommand(command)) messageBox("Analyzer busy - try again", NAN);
}

void action_reset_o2_100(lv_event_t * e) {
  app::Command command;
  command.type = app::CommandType::ClearPure;
  if (!submitAnalyzerCommand(command)) messageBox("Analyzer busy - try again", NAN);
}

void action_reset_he(lv_event_t * e) {
  app::Command command;
  command.type = app::CommandType::ResetHe;
  if (!submitAnalyzerCommand(command)) messageBox("Analyzer busy - try again", NAN);
}

namespace ui {
namespace {
lv_obj_t* actionDialog(const char* title) {
  lv_obj_t* dialog = lv_msgbox_create(nullptr);
  lv_obj_set_size(dialog, 310, 162);
  lv_msgbox_add_title(dialog, title);
  lv_msgbox_add_close_button(dialog);
  lv_obj_t* content = lv_msgbox_get_content(dialog);
  lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
  return dialog;
}

void addAction(lv_obj_t* content, const char* title, lv_event_cb_t callback) {
  lv_obj_t* button = lv_button_create(content);
  lv_obj_set_width(button, lv_pct(100));
  lv_obj_t* label = lv_label_create(button);
  lv_label_set_text(label, title);
  lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, nullptr);
}

void changeStartupCalibration(lv_event_t* event) {
  app::Command command;
  command.settings = uiSettings();
  command.settings.calibrateOnStart = lv_obj_has_state(lv_event_get_target_obj(event), LV_STATE_CHECKED);
  if (!submitAnalyzerCommand(command)) messageBox("Analyzer busy - preference not saved", NAN);
  lv_obj_set_state(startupCalibration, LV_STATE_CHECKED, uiSettings().calibrateOnStart);
}

void closeCalibration(lv_event_t*) { startupCalibration = nullptr; }

void closeUpdates(lv_event_t*) {
  wifiNames = nullptr;
  wifiPassword = nullptr;
  if (updateKeyboard) lv_obj_delete(updateKeyboard);
  updateKeyboard = nullptr;
}

void hideKeyboard(lv_event_t*) {
  lv_obj_add_flag(updateKeyboard, LV_OBJ_FLAG_HIDDEN);
}

void editPassword(lv_event_t*) {
  lv_keyboard_set_textarea(updateKeyboard, wifiPassword);
  lv_obj_remove_flag(updateKeyboard, LV_OBJ_FLAG_HIDDEN);
}
}

void syncActionSettings(const AnalyzerSettings& settings) {
  if (startupCalibration) lv_obj_set_state(startupCalibration, LV_STATE_CHECKED, settings.calibrateOnStart);
}

void openCalibration(lv_event_t*) {
  if (startupCalibration) return;
  lv_obj_t* dialog = actionDialog("Calibration");
  lv_obj_add_event_cb(dialog, closeCalibration, LV_EVENT_DELETE, nullptr);
  lv_obj_t* content = lv_msgbox_get_content(dialog);
  startupCalibration = lv_checkbox_create(content);
  lv_checkbox_set_text(startupCalibration, "Calibrate on start");
  syncActionSettings(uiSettings());
  lv_obj_add_event_cb(startupCalibration, changeStartupCalibration, LV_EVENT_VALUE_CHANGED, nullptr);
  addAction(content, "Calibrate air (20.9%)", action_calibrate_o2_21);
  addAction(content, "Calibrate O2 (100%)", action_calibrate_o2_100);
  addAction(content, "Calibrate He (100%)", action_calibrate_he);
  addAction(content, "Reset air calibration", action_reset_o2_21);
  addAction(content, "Clear pure O2 calibration", action_reset_o2_100);
  addAction(content, "Reset He calibration", action_reset_he);
  addAction(content, "Diagnostics", openLogs);
}

void openUpdates(lv_event_t*) {
  if (wifiNames) return;
  lv_obj_t* dialog = actionDialog("Firmware update");
  lv_obj_add_event_cb(dialog, closeUpdates, LV_EVENT_DELETE, nullptr);
  lv_obj_t* content = lv_msgbox_get_content(dialog);
  wifiNames = lv_dropdown_create(content);
  lv_obj_set_width(wifiNames, lv_pct(100));
  lv_dropdown_clear_options(wifiNames);
  lv_dropdown_set_text(wifiNames, "Scan for Wi-Fi");
  addAction(content, "Scan Wi-Fi", action_list_wifi);
  wifiPassword = lv_textarea_create(content);
  lv_obj_set_width(wifiPassword, lv_pct(100));
  lv_textarea_set_one_line(wifiPassword, true);
  lv_textarea_set_password_mode(wifiPassword, true);
  lv_textarea_set_max_length(wifiPassword, 64);
  lv_textarea_set_placeholder_text(wifiPassword, "Wi-Fi password");
  updateKeyboard = lv_keyboard_create(lv_layer_top());
  lv_obj_set_size(updateKeyboard, 320, 100);
  lv_obj_align(updateKeyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_add_flag(updateKeyboard, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(updateKeyboard, hideKeyboard, LV_EVENT_READY, nullptr);
  lv_obj_add_event_cb(updateKeyboard, hideKeyboard, LV_EVENT_CANCEL, nullptr);
  lv_obj_add_event_cb(wifiPassword, editPassword, LV_EVENT_CLICKED, nullptr);
  addAction(content, "Install firmware", action_update_firmware);
}

void openLogs(lv_event_t*) {
  lv_obj_t* dialog = actionDialog("Diagnostics");
  const char* log = UiLog::getInstance().getLogAsCString();
  lv_msgbox_add_text(dialog, log && *log ? log : "No log entries");
}
}

void showAnalyzerResult(const app::Result& result) {
  if (result.sensorError != SensorError::None) {
    logUi(SensorManager::getErrorString(result.sensorError), UiLogLevel::Warning);
  }
  const char* title = nullptr;
  const char* required = result.calibrationRequiredMessage();
  if (required && result.failure != app::Failure::CalibrationRequired &&
      result.type != app::CommandType::PrepareSleep && result.type != app::CommandType::Resume) {
    logUi(required, UiLogLevel::Warning);
  }
  if (result.failure != app::Failure::None) {
    switch (result.failure) {
      case app::Failure::Invalid: title = "Invalid settings or calibration"; break;
      case app::Failure::Storage: title = "Settings could not be saved"; break;
      case app::Failure::Sampling: title = "Calibration read failed"; break;
      case app::Failure::LoadedDefaults: title = "Invalid saved settings - defaults loaded"; break;
      case app::Failure::Busy: title = "Analyzer is preparing for sleep"; break;
      case app::Failure::CalibrationRequired:
        title = result.calibrationRequiredMessage();
        if (!title) title = "O2 calibration required";
        break;
      default: title = "Operation failed"; break;
    }
    logUi(title, UiLogLevel::Error);
  } else {
    switch (result.type) {
      case app::CommandType::CalibrateAir: title = "O2 Air Calibrated"; break;
      case app::CommandType::CalibratePure: title = "O2 100% Calibrated"; break;
      case app::CommandType::CalibrateHe: title = "He 100% Calibrated"; break;
      default: break;
    }
  }
  if (!title && required && result.type == app::CommandType::ApplySettings) title = required;
  if (title) messageBox(title, result.calibration);
}

void action_list_wifi(lv_event_t * e) {
  if (!wifiNames) return;
  NetworkOperation operation;
  log_i("Listing WiFi networks");
  std::vector<String> ssidList = scanWifiNetworks();
  for (const auto &ssid : ssidList) {
    log_i("Found WiFi network: %s", ssid.c_str());
  }

  lv_dropdown_clear_options(wifiNames);
  for (const auto &ssid : ssidList) {
    lv_dropdown_add_option(wifiNames, ssid.c_str(), LV_DROPDOWN_POS_LAST);
  }
  lv_dropdown_set_text(wifiNames, ssidList.empty() ? "No networks found" : nullptr);
  WiFi.scanDelete();
}

// bool updateFromURL(const char* url) {

//   HTTPClient http;
//   WiFiClientSecure client;
//   client.setInsecure();  // allows skipping cert validation
//   client.setTimeout(30000);
//   http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
//   http.begin(client, url);

//   int httpCode = http.GET();
//   if (httpCode != HTTP_CODE_OK) {
//     Serial.printf("HTTP GET failed: %d\n", httpCode);
//     http.end();
//     return false;
//   }

//   int contentLength = http.getSize();
//   if (!Update.begin(contentLength)) {
//     Serial.println("Not enough space for OTA update.");
//     http.end();
//     return false;
//   }

//   WiFiClient& stream = http.getStream();
//   size_t written = Update.writeStream(stream);

//   if (written != contentLength) {
//     Serial.printf("Only %d/%d bytes written. Update failed.\n", (int)written, contentLength);
//     http.end();
//     return false;
//   }

//   if (!Update.end() || !Update.isFinished()) {
//     Serial.printf("Update error: %s\n", Update.errorString());
//     http.end();
//     return false;
//   }

//   Serial.println("Update complete. Rebooting...");
//   http.end();
//   delay(1000);
//   ESP.restart();
//   return true;
// }

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>

bool updateFromURL(const char* url) {
  WiFiClientSecure client;
  client.setInsecure();             // ❗ Skip SSL cert verification (insecure, OK for testing)
  client.setTimeout(15000);         // ⏱ Increase timeout

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.useHTTP10(false);            // 📡 Enable keep-alive

  Serial.printf("Connecting to: %s\n", url);
  if (!http.begin(client, url)) {
    Serial.println("HTTPClient.begin() failed.");
    return false;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP GET failed: %d\n", httpCode);
    http.end();
    return false;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    Serial.println("Invalid content length.");
    http.end();
    return false;
  }

  Serial.printf("Firmware size: %d bytes\n", contentLength);
  if (!Update.begin(contentLength)) {
    Serial.println("Not enough space for OTA update.");
    http.end();
    return false;
  }

  WiFiClient& stream = http.getStream();
  uint8_t buffer[1024];
  size_t written = 0;
  float lastLog = esp_timer_get_time();

  while (http.connected() && written < contentLength) {
    size_t available = stream.available();
    if (available) {
      if (available > sizeof(buffer)) available = sizeof(buffer);
      int readLen = stream.readBytes(buffer, available);
      if (readLen <= 0) {
        Serial.println("Read failed or connection closed.");
        break;
      }

      if (Update.write(buffer, readLen) != (size_t)readLen) {
        Serial.printf("Write failed at %d bytes\n", written);
        Update.printError(Serial);
        http.end();
        return false;
      }

      written += readLen;

      // Log progress every ~10 seconds
      if (esp_timer_get_time() - lastLog > 10000) {
        Serial.printf("Written %d / %d bytes...\n", (int)written, contentLength);
        lastLog = esp_timer_get_time();
      }
    } else {
      delay(1); // yield
    }
  }

  http.end();

  if (written != contentLength) {
    Serial.printf("Only %d / %d bytes written. OTA failed.\n", (int)written, contentLength);
    return false;
  }

  if (!Update.end() || !Update.isFinished()) {
    Serial.printf("Update failed: %s\n", Update.errorString());
    return false;
  }

  Serial.println("OTA update successful! Rebooting...");
  delay(1000);
  ESP.restart();
  return true;
}

void action_update_firmware(lv_event_t * e) {
  if (!wifiNames || !wifiPassword || lv_dropdown_get_option_count(wifiNames) == 0) {
    messageBox("Select a Wi-Fi network first", NAN);
    return;
  }
  NetworkOperation operation;
  log_i("Updating firmware");
  log_i("Free heap before OTA: %d", ESP.getFreeHeap());
  char selectedSSID[64];
  lv_dropdown_get_selected_str(wifiNames, selectedSSID, sizeof(selectedSSID));

  const char* password = lv_textarea_get_text(wifiPassword);
  log_i("Selected SSID: %s", selectedSSID);
  WiFi.begin(selectedSSID, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(1000);
    log_i("Connecting to WiFi...");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    log_i("Connected to WiFi");
    // Start the OTA update process
  }
  else {
    log_i("Failed to connect to WiFi");
    messageBox("Wi-Fi connection failed", NAN);
    return;
  }

  if (!updateFromURL("https://vld.ams3.digitaloceanspaces.com/firmware.bin"))
    messageBox("Firmware update failed", NAN);
}