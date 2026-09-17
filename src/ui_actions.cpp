#include <lvgl.h>
#include "display/UiAdapter.h"
#include "lvgl_ui_project.h"
#include "main.h"
#include "ui_actions.h"
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

void messageBox(const char * title, float value) {
  char text[32] = "";
  if (std::isfinite(value)) snprintf(text, sizeof(text), "%.2f mv", value);

  lv_obj_t * mbox = lv_msgbox_create(NULL);
  lv_msgbox_add_close_button(mbox);
  lv_msgbox_add_title(mbox, title);
  lv_msgbox_add_text(mbox, text);
  lv_obj_set_size(mbox, LV_PCT(70), LV_SIZE_CONTENT);
}

void action_reset_o2_100(lv_event_t * e) {
  app::Command command;
  command.type = app::CommandType::ClearPure;
  if (!submitAnalyzerCommand(command)) messageBox("Analyzer busy - try again", NAN);
}

void showAnalyzerResult(const app::Result& result) {
  if (result.sensorError != SensorError::None) {
    logUi(SensorManager::getErrorString(result.sensorError), UiLogLevel::Warning);
  }
  if (result.calibrationPhase != app::CalibrationPhase::None && !result.complete) return;
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
      case app::Failure::Cancelled: break;
      default: title = "Operation failed"; break;
    }
    logUi(title, UiLogLevel::Error);
  } else {
    switch (result.type) {
      case app::CommandType::CalibrateAir: title = "O2 Air Calibrated"; break;
      case app::CommandType::CalibratePure: title = "O2 100% Calibrated"; break;
      case app::CommandType::CalibrateHe: title = "He 100% Calibrated"; break;
      case app::CommandType::ClearPure: title = "Pure O2 calibration cleared"; break;
      default: break;
    }
  }
  if (!title && required && result.type == app::CommandType::ApplySettings) title = required;
  if (title && result.calibrationPhase == app::CalibrationPhase::None) messageBox(title, result.calibration);
}

void action_list_wifi(lv_event_t * e) {
  lv_obj_t* wifiNames = lv_obj_find_by_name(lv_screen_active(), "wifi_names");
  if (!wifiNames) return;
  NetworkOperation operation;
  lv_subject_copy_string(&update_status_text, "Scanning Wi-Fi...");
  lv_subject_set_int(&update_can_install, 0);
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
  lv_subject_set_int(&update_network_index, 0);
  lv_subject_set_int(&update_can_install, !ssidList.empty());
  lv_subject_copy_string(&update_status_text, ssidList.empty() ? "No networks found" : "Select a network and enter its password");
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
  lv_obj_t* screen = lv_screen_active();
  lv_obj_t* wifiNames = lv_obj_find_by_name(screen, "wifi_names");
  lv_obj_t* wifiPassword = lv_obj_find_by_name(screen, "wifi_password");
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
  lv_subject_copy_string(&update_status_text, "Connecting to Wi-Fi...");
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
    lv_subject_copy_string(&update_status_text, "Downloading firmware...");
    // Start the OTA update process
  }
  else {
    log_i("Failed to connect to WiFi");
    lv_subject_copy_string(&update_status_text, "Wi-Fi connection failed");
    messageBox("Wi-Fi connection failed", NAN);
    return;
  }

  if (!updateFromURL("https://vld.ams3.digitaloceanspaces.com/firmware.bin")) {
    lv_subject_copy_string(&update_status_text, "Firmware update failed");
    messageBox("Firmware update failed", NAN);
  }
}