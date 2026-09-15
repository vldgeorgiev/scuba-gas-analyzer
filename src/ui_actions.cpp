#include <lvgl.h>
#include "structs.h"
#include "actions.h"
#include "vars.h"
#include "main.h"
#include "ui-log.h"
#include "pin_config.h"
#include "updater.h"
#include "screens.h"
#include <HTTPClient.h>
#include <Update.h>

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

void action_open_config(lv_event_t * e) {
  setUiSettingsEditing(true);
  syncUiSettings();
}

void action_close_config(lv_event_t * e) {
  setUiSettingsEditing(false);
  app::Command command;
  command.settings = uiSettings();
  command.settings.o2Enabled = flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_O2_ENABLED).getBoolean();
  command.settings.coEnabled = flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_CO_ENABLED).getBoolean();
  command.settings.heEnabled = flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_HE_ENABLED).getBoolean();
  command.settings.po2Bottom = flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_PO2_MAX_BOTTOM).getFloat();
  command.settings.po2Deco = flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_PO2_MAX_DECO).getFloat();
  command.settings.calibrateOnStart = flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_CALIBRATE_ON_START).getBoolean();
  command.settings.brightness = flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_BRIGHTNESS).getUInt8();
  if (!submitAnalyzerCommand(command)) messageBox("Settings not applied - analyzer busy", NAN);
  syncUiSettings();
}

void showAnalyzerResult(const app::Result& result) {
  if (result.sensorError != SensorError::None) {
    logUi(SensorManager::getErrorString(result.sensorError), UiLogLevel::Warning);
  }
  const char* title = nullptr;
  if (result.failure != app::Failure::None) {
    switch (result.failure) {
      case app::Failure::Invalid: title = "Invalid settings or calibration"; break;
      case app::Failure::Storage: title = "Settings could not be saved"; break;
      case app::Failure::Sampling: title = "Calibration read failed"; break;
      case app::Failure::LoadedDefaults: title = "Invalid saved settings - defaults loaded"; break;
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
  if (title) messageBox(title, result.calibration);
}

const char *get_var_ui_log() {
  return UiLog::getInstance().getLogAsCString();
}

void set_var_ui_log(const char *value) {
  // ui_log is read only
}

void action_brightness_change(lv_event_t * e) {
  log_i("Brightness change");
  displayManager.setBrightness(flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_BRIGHTNESS).getUInt8());
}

void action_list_wifi(lv_event_t * e) {
  log_i("Listing WiFi networks");
  std::vector<String> ssidList = scanWifiNetworks();
  for (const auto &ssid : ssidList) {
    log_i("Found WiFi network: %s", ssid.c_str());
  }

  std::string wifiListStr;
  for (const auto &ssid : ssidList) {
    wifiListStr += std::string(ssid.c_str()) + "\n";
  }
  flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_WIFI_LIST, StringValue(wifiListStr.c_str()));
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
  log_i("Updating firmware");
  log_i("Free heap before OTA: %d", ESP.getFreeHeap());
  char selectedSSID[64];
  lv_dropdown_get_selected_str(objects.wifi_name, selectedSSID, sizeof(selectedSSID));

  const char* password = lv_textarea_get_text(objects.wifi_pass);
  log_i("Selected SSID: %s", selectedSSID);
  log_i("Password: %s", password);
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
  }

  updateFromURL("https://vld.ams3.digitaloceanspaces.com/firmware.bin");
}