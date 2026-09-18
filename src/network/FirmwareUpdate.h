#pragma once

#include <cstddef>
#include <cstdint>

constexpr size_t FIRMWARE_UPDATE_MAX_NETWORKS = 16;
constexpr size_t FIRMWARE_UPDATE_MAX_SSID_LENGTH = 32;

enum class FirmwareUpdateEventType : uint8_t {
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

struct FirmwareUpdateEvent {
  FirmwareUpdateEventType type = FirmwareUpdateEventType::UpdateFailed;
  uint8_t progress = 0;
  uint8_t networkCount = 0;
  char networks[FIRMWARE_UPDATE_MAX_NETWORKS][FIRMWARE_UPDATE_MAX_SSID_LENGTH + 1] = {};
};

bool firmwareUpdateBusy();
bool startWifiScan();
bool startFirmwareUpdate(const char* ssid, const char* password);
bool pollFirmwareUpdateEvent(FirmwareUpdateEvent& event);
void finishFirmwareUpdate();