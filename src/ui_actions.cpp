#include <lvgl.h>
#include "display/UiAdapter.h"
#include "lvgl_ui_project.h"
#include "main.h"
#include "network/TrustedRoots.h"
#include "ui_actions.h"
#include "ui-log.h"
#include "pin_config.h"
#include <algorithm>
#include <HTTPClient.h>
#include <new>
#include <Update.h>
#include <WiFiClientSecure.h>

namespace {

constexpr size_t MAX_NETWORKS = 16;
constexpr size_t MAX_SSID_LENGTH = 32;
constexpr uint32_t DOWNLOAD_STALL_TIMEOUT_MS = 15000;
constexpr uint32_t TIME_SYNC_TIMEOUT_MS = 10000;

enum class NetworkEventType : uint8_t {
  ScanComplete,
  TimeSyncStarted,
  DownloadStarted,
  DownloadProgress,
  UpdateComplete,
  ConnectionFailed,
  TimeSyncFailed,
  SecureConnectionFailed,
  UpdateFailed,
};

enum class DownloadResult : uint8_t {
  Success,
  SecureConnectionFailed,
  Failed,
};

struct NetworkEvent {
  NetworkEventType type = NetworkEventType::UpdateFailed;
  uint8_t progress = 0;
  uint8_t networkCount = 0;
  char networks[MAX_NETWORKS][MAX_SSID_LENGTH + 1] = {};
};

struct UpdateRequest {
  char ssid[MAX_SSID_LENGTH + 1] = {};
  char password[65] = {};
};

QueueHandle_t networkEventQueue = nullptr;
bool networkOperationBusy = false;

bool ensureNetworkEventQueue() {
  if (!networkEventQueue) networkEventQueue = xQueueCreate(1, sizeof(NetworkEvent));
  return networkEventQueue != nullptr;
}

void publishNetworkEvent(NetworkEventType type, uint8_t progress = 0) {
  NetworkEvent event;
  event.type = type;
  event.progress = progress;
  xQueueOverwrite(networkEventQueue, &event);
}

void finishNetworkOperation() {
  networkOperationBusy = false;
  setNetworkOperationActive(false);
}

void scanNetworkTask(void*) {
  NetworkEvent event;
  event.type = NetworkEventType::ScanComplete;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  const int networkCount = WiFi.scanNetworks();
  event.networkCount = static_cast<uint8_t>(std::min(networkCount > 0 ? static_cast<size_t>(networkCount) : 0,
                                                     MAX_NETWORKS));
  for (uint8_t index = 0; index < event.networkCount; ++index) {
    snprintf(event.networks[index], sizeof(event.networks[index]), "%s", WiFi.SSID(index).c_str());
  }
  WiFi.scanDelete();
  xQueueOverwrite(networkEventQueue, &event);
  vTaskDelete(nullptr);
}

}  // namespace

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
  if (networkOperationBusy) {
    messageBox("Network operation in progress", NAN);
    return;
  }
  if (!lv_obj_find_by_name(lv_screen_active(), "wifi_names")) return;
  if (!ensureNetworkEventQueue()) {
    messageBox("Could not start Wi-Fi scan", NAN);
    return;
  }
  networkOperationBusy = true;
  setNetworkOperationActive(true);
  lv_subject_copy_string(&update_status_text, "Scanning Wi-Fi...");
  lv_subject_set_int(&update_can_install, 0);
  log_i("Listing WiFi networks");
  if (xTaskCreate(scanNetworkTask, "WiFi scan", 4096, nullptr, 1, nullptr) != pdPASS) {
    finishNetworkOperation();
    lv_subject_copy_string(&update_status_text, "Could not start Wi-Fi scan");
    messageBox("Could not start Wi-Fi scan", NAN);
  }
}

DownloadResult updateFromURL(const char* url) {
  WiFiClientSecure client;
  client.setCACert(GITHUB_ROOT_CA_CERTIFICATES);
  client.setHandshakeTimeout(15);
  client.setTimeout(15000);

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.useHTTP10(false);

  Serial.printf("Connecting to: %s\n", url);
  if (!http.begin(client, url)) {
    Serial.println("HTTPClient.begin() failed.");
    return DownloadResult::SecureConnectionFailed;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP GET failed: %d\n", httpCode);
    http.end();
    return httpCode < 0 ? DownloadResult::SecureConnectionFailed : DownloadResult::Failed;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    Serial.println("Invalid content length.");
    http.end();
    return DownloadResult::Failed;
  }

  Serial.printf("Firmware size: %d bytes\n", contentLength);
  if (!Update.begin(contentLength)) {
    Serial.println("Not enough space for OTA update.");
    http.end();
    return DownloadResult::Failed;
  }

  WiFiClient& stream = http.getStream();
  uint8_t buffer[1024];
  size_t written = 0;
  int64_t lastLog = esp_timer_get_time();
  uint32_t lastDataAt = millis();
  uint8_t lastProgress = 0;

  while (written < static_cast<size_t>(contentLength)) {
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
        Update.abort();
        http.end();
        return DownloadResult::Failed;
      }

      written += readLen;
      lastDataAt = millis();
      const uint8_t progress = static_cast<uint8_t>(written * 100 / contentLength);
      if (progress != lastProgress) {
        publishNetworkEvent(NetworkEventType::DownloadProgress, progress);
        lastProgress = progress;
      }

      // Log progress every ~10 seconds
      if (esp_timer_get_time() - lastLog > 10000000) {
        Serial.printf("Written %d / %d bytes...\n", (int)written, contentLength);
        lastLog = esp_timer_get_time();
      }
    } else {
      if (!http.connected() || static_cast<uint32_t>(millis() - lastDataAt) >= DOWNLOAD_STALL_TIMEOUT_MS) break;
      delay(1); // yield
    }
  }

  http.end();

  if (written != contentLength) {
    Serial.printf("Only %d / %d bytes written. OTA failed.\n", (int)written, contentLength);
    Update.abort();
    return DownloadResult::Failed;
  }

  if (!Update.end() || !Update.isFinished()) {
    Serial.printf("Update failed: %s\n", Update.errorString());
    Update.abort();
    return DownloadResult::Failed;
  }

  Serial.println("OTA update successful! Rebooting...");
  publishNetworkEvent(NetworkEventType::UpdateComplete, 100);
  delay(750);
  ESP.restart();
  return DownloadResult::Success;
}

namespace {

void updateNetworkTask(void* parameter) {
  UpdateRequest* request = static_cast<UpdateRequest*>(parameter);
  WiFi.begin(request->ssid, request->password);
  delete request;

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(1000);
    log_i("Connecting to WiFi...");
    ++attempts;
  }
  if (WiFi.status() != WL_CONNECTED) {
    log_i("Failed to connect to WiFi");
    publishNetworkEvent(NetworkEventType::ConnectionFailed);
    vTaskDelete(nullptr);
    return;
  }

  log_i("Connected to WiFi");
  publishNetworkEvent(NetworkEventType::TimeSyncStarted);
  configTime(0, 0, "time.cloudflare.com", "pool.ntp.org", "time.google.com");
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo, TIME_SYNC_TIMEOUT_MS)) {
    log_i("Failed to synchronize time");
    publishNetworkEvent(NetworkEventType::TimeSyncFailed);
    vTaskDelete(nullptr);
    return;
  }

  publishNetworkEvent(NetworkEventType::DownloadStarted);
  const DownloadResult result =
      updateFromURL("https://github.com/vldgeorgiev/scuba-gas-analyzer/releases/latest/download/firmware.bin");
  if (result == DownloadResult::SecureConnectionFailed) {
    publishNetworkEvent(NetworkEventType::SecureConnectionFailed);
  } else if (result == DownloadResult::Failed) {
    publishNetworkEvent(NetworkEventType::UpdateFailed);
  }
  vTaskDelete(nullptr);
}

}  // namespace

void action_update_firmware(lv_event_t * e) {
  if (networkOperationBusy) {
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
  if (!ensureNetworkEventQueue()) {
    messageBox("Could not start firmware update", NAN);
    return;
  }
  log_i("Updating firmware");
  log_i("Free heap before OTA: %d", ESP.getFreeHeap());
  char selectedSSID[MAX_SSID_LENGTH + 1];
  lv_dropdown_get_selected_str(wifiNames, selectedSSID, sizeof(selectedSSID));

  const char* password = lv_textarea_get_text(wifiPassword);
  UpdateRequest* request = new (std::nothrow) UpdateRequest;
  if (!request) {
    messageBox("Could not start firmware update", NAN);
    return;
  }
  snprintf(request->ssid, sizeof(request->ssid), "%s", selectedSSID);
  snprintf(request->password, sizeof(request->password), "%s", password);
  networkOperationBusy = true;
  setNetworkOperationActive(true);
  lv_subject_set_int(&update_can_install, 0);
  lv_subject_copy_string(&update_status_text, "Connecting to Wi-Fi...");
  log_i("Selected SSID: %s", selectedSSID);
  if (xTaskCreate(updateNetworkTask, "Firmware update", 1024 * 10, request, 1, nullptr) != pdPASS) {
    delete request;
    finishNetworkOperation();
    lv_subject_copy_string(&update_status_text, "Could not start firmware update");
    messageBox("Could not start firmware update", NAN);
  }
}

void serviceNetworkActions() {
  if (!networkEventQueue) return;
  NetworkEvent event;
  if (xQueueReceive(networkEventQueue, &event, 0) != pdPASS) return;

  switch (event.type) {
    case NetworkEventType::ScanComplete: {
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
      lv_subject_copy_string(&update_status_text,
                             event.networkCount ? "Select a network and enter its password" : "No networks found");
      finishNetworkOperation();
      break;
    }
    case NetworkEventType::TimeSyncStarted:
      lv_subject_copy_string(&update_status_text, "Setting device time...");
      break;
    case NetworkEventType::DownloadStarted:
      lv_subject_copy_string(&update_status_text, "Downloading firmware... 0%");
      break;
    case NetworkEventType::DownloadProgress: {
      char status[40];
      snprintf(status, sizeof(status), "Downloading... %u%%", event.progress);
      lv_subject_copy_string(&update_status_text, status);
      break;
    }
    case NetworkEventType::UpdateComplete:
      lv_subject_copy_string(&update_status_text, "Update complete - restarting...");
      break;
    case NetworkEventType::ConnectionFailed:
      lv_subject_copy_string(&update_status_text, "Wi-Fi connection failed");
      lv_subject_set_int(&update_can_install, 1);
      finishNetworkOperation();
      messageBox("Wi-Fi connection failed", NAN);
      break;
    case NetworkEventType::TimeSyncFailed:
      lv_subject_copy_string(&update_status_text, "Could not set time - update cancelled");
      lv_subject_set_int(&update_can_install, 1);
      finishNetworkOperation();
      messageBox("Could not set device time", NAN);
      break;
    case NetworkEventType::SecureConnectionFailed:
      lv_subject_copy_string(&update_status_text, "Secure connection failed");
      lv_subject_set_int(&update_can_install, 1);
      finishNetworkOperation();
      messageBox("Secure connection failed", NAN);
      break;
    case NetworkEventType::UpdateFailed:
      lv_subject_copy_string(&update_status_text, "Firmware update failed");
      lv_subject_set_int(&update_can_install, 1);
      finishNetworkOperation();
      messageBox("Firmware update failed", NAN);
      break;
  }
}