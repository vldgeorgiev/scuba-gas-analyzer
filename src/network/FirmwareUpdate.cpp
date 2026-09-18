#include "FirmwareUpdate.h"
#include "TrustedRoots.h"
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include <algorithm>
#include <new>

namespace {

constexpr uint32_t DOWNLOAD_STALL_TIMEOUT_MS = 15000;
constexpr uint32_t TIME_SYNC_TIMEOUT_MS = 10000;

enum class DownloadResult : uint8_t {
  Success,
  SecureConnectionFailed,
  Failed,
};

struct UpdateRequest {
  char ssid[FIRMWARE_UPDATE_MAX_SSID_LENGTH + 1] = {};
  char password[65] = {};
};

QueueHandle_t firmwareUpdateEventQueue = nullptr;
bool firmwareUpdateOperationBusy = false;

bool ensureFirmwareUpdateEventQueue() {
  if (!firmwareUpdateEventQueue) firmwareUpdateEventQueue = xQueueCreate(1, sizeof(FirmwareUpdateEvent));
  return firmwareUpdateEventQueue != nullptr;
}

void publishFirmwareUpdateEvent(FirmwareUpdateEventType type, uint8_t progress = 0) {
  FirmwareUpdateEvent event;
  event.type = type;
  event.progress = progress;
  xQueueOverwrite(firmwareUpdateEventQueue, &event);
}

void scanNetworkTask(void*) {
  FirmwareUpdateEvent event;
  event.type = FirmwareUpdateEventType::ScanComplete;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  const int networkCount = WiFi.scanNetworks();
  event.networkCount = static_cast<uint8_t>(std::min(networkCount > 0 ? static_cast<size_t>(networkCount) : 0,
                                                     FIRMWARE_UPDATE_MAX_NETWORKS));
  for (uint8_t index = 0; index < event.networkCount; ++index) {
    snprintf(event.networks[index], sizeof(event.networks[index]), "%s", WiFi.SSID(index).c_str());
  }
  WiFi.scanDelete();
  xQueueOverwrite(firmwareUpdateEventQueue, &event);
  vTaskDelete(nullptr);
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

  const int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP GET failed: %d\n", httpCode);
    http.end();
    return httpCode < 0 ? DownloadResult::SecureConnectionFailed : DownloadResult::Failed;
  }

  const int contentLength = http.getSize();
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
      const int readLen = stream.readBytes(buffer, available);
      if (readLen <= 0) {
        Serial.println("Read failed or connection closed.");
        break;
      }

      if (Update.write(buffer, readLen) != static_cast<size_t>(readLen)) {
        Serial.printf("Write failed at %d bytes\n", static_cast<int>(written));
        Update.printError(Serial);
        Update.abort();
        http.end();
        return DownloadResult::Failed;
      }

      written += readLen;
      lastDataAt = millis();
      const uint8_t progress = static_cast<uint8_t>(written * 100 / contentLength);
      if (progress != lastProgress) {
        publishFirmwareUpdateEvent(FirmwareUpdateEventType::DownloadProgress, progress);
        lastProgress = progress;
      }

      if (esp_timer_get_time() - lastLog > 10000000) {
        Serial.printf("Written %d / %d bytes...\n", static_cast<int>(written), contentLength);
        lastLog = esp_timer_get_time();
      }
    } else {
      if (!http.connected() || static_cast<uint32_t>(millis() - lastDataAt) >= DOWNLOAD_STALL_TIMEOUT_MS) break;
      delay(1);
    }
  }

  http.end();

  if (written != static_cast<size_t>(contentLength)) {
    Serial.printf("Only %d / %d bytes written. OTA failed.\n", static_cast<int>(written), contentLength);
    Update.abort();
    return DownloadResult::Failed;
  }

  if (!Update.end() || !Update.isFinished()) {
    Serial.printf("Update failed: %s\n", Update.errorString());
    Update.abort();
    return DownloadResult::Failed;
  }

  Serial.println("OTA update successful! Rebooting...");
  publishFirmwareUpdateEvent(FirmwareUpdateEventType::UpdateComplete, 100);
  delay(750);
  ESP.restart();
  return DownloadResult::Success;
}

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
    publishFirmwareUpdateEvent(FirmwareUpdateEventType::ConnectionFailed);
    vTaskDelete(nullptr);
    return;
  }

  log_i("Connected to WiFi");
  publishFirmwareUpdateEvent(FirmwareUpdateEventType::TimeSyncStarted);
  configTime(0, 0, "time.cloudflare.com", "pool.ntp.org", "time.google.com");
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo, TIME_SYNC_TIMEOUT_MS)) {
    log_i("Failed to synchronize time");
    publishFirmwareUpdateEvent(FirmwareUpdateEventType::TimeSyncFailed);
    vTaskDelete(nullptr);
    return;
  }

  publishFirmwareUpdateEvent(FirmwareUpdateEventType::DownloadStarted);
  const DownloadResult result =
      updateFromURL("https://github.com/vldgeorgiev/scuba-gas-analyzer/releases/latest/download/firmware.bin");
  if (result == DownloadResult::SecureConnectionFailed) {
    publishFirmwareUpdateEvent(FirmwareUpdateEventType::SecureConnectionFailed);
  } else if (result == DownloadResult::Failed) {
    publishFirmwareUpdateEvent(FirmwareUpdateEventType::UpdateFailed);
  }
  vTaskDelete(nullptr);
}

}  // namespace

bool firmwareUpdateBusy() { return firmwareUpdateOperationBusy; }

bool startWifiScan() {
  if (firmwareUpdateOperationBusy || !ensureFirmwareUpdateEventQueue()) return false;
  firmwareUpdateOperationBusy = true;
  if (xTaskCreate(scanNetworkTask, "WiFi scan", 4096, nullptr, 1, nullptr) == pdPASS) return true;
  firmwareUpdateOperationBusy = false;
  return false;
}

bool startFirmwareUpdate(const char* ssid, const char* password) {
  if (firmwareUpdateOperationBusy || !ensureFirmwareUpdateEventQueue()) return false;
  UpdateRequest* request = new (std::nothrow) UpdateRequest;
  if (!request) return false;
  snprintf(request->ssid, sizeof(request->ssid), "%s", ssid);
  snprintf(request->password, sizeof(request->password), "%s", password);
  firmwareUpdateOperationBusy = true;
  if (xTaskCreate(updateNetworkTask, "Firmware update", 1024 * 10, request, 1, nullptr) == pdPASS) return true;
  delete request;
  firmwareUpdateOperationBusy = false;
  return false;
}

bool pollFirmwareUpdateEvent(FirmwareUpdateEvent& event) {
  return firmwareUpdateEventQueue && xQueueReceive(firmwareUpdateEventQueue, &event, 0) == pdPASS;
}

void finishFirmwareUpdate() { firmwareUpdateOperationBusy = false; }