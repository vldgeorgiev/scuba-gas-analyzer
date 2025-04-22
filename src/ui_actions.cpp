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
  char text[32];
  snprintf(text, sizeof(text), "%.2f mv", value);

  lv_obj_t * mbox = lv_msgbox_create(NULL);
  lv_msgbox_add_close_button(mbox);
  lv_msgbox_add_title(mbox, title);
  lv_msgbox_add_text(mbox, text);
  lv_obj_set_size(mbox, LV_PCT(70), LV_SIZE_CONTENT);
}

void action_calibrate_o2_21(lv_event_t * e) {
  float value = sensors.calibrateO2_21();
  config.setO2Calibration21(value);
  messageBox("O2 Air", value);
}

void action_calibrate_o2_100(lv_event_t * e) {
  float value = sensors.calibrateO2_100();
  config.setO2Calibration100(value);
  messageBox("O2 100%", value);
}

void action_calibrate_he(lv_event_t * e) {
  float value = sensors.calibrateHe_100();
  config.setHeCalibration100(value);
  messageBox("He 100%", value);
}

void action_reset_o2_21(lv_event_t * e) {
  log_i("Resetting O2 21 calibration");
  config.setO2Calibration21(config.O2_CALIBRATION_21_DEFAULT);
}

void action_reset_o2_100(lv_event_t * e) {
  log_i("Resetting O2 100 calibration");
  config.setO2Calibration100(config.O2_CALIBRATION_100_DEFAULT);
}

void action_reset_he(lv_event_t * e) {
  log_i("Resetting He calibration");
  config.setHeCalibration100(config.HE_CALIBRATION_100_DEFAULT);
}

void action_open_config(lv_event_t * e) {
  log_i("Opening config");
  configOpen = true;
}

void action_close_config(lv_event_t * e) {
  log_i("Closing config");
  config.setO2Enabled(flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_O2_ENABLED).getBoolean());
  config.setCOEnabled(flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_CO_ENABLED).getBoolean());
  config.setHeEnabled(flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_HE_ENABLED).getBoolean());

  config.setPO2Bottom(flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_PO2_MAX_BOTTOM).getFloat());
  config.setPO2Deco(flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_PO2_MAX_DECO).getFloat());

  config.setCalibrateOnStart(flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_CALIBRATE_ON_START).getBoolean());
  config.setBrightness(flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_BRIGHTNESS).getUInt8());

  digitalWrite(PIN_HE_ENABLE, config.getHeEnabled());
  digitalWrite(PIN_CO_ENABLE, config.getCOEnabled());

  configOpen = false;
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