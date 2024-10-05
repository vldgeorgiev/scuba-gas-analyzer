#ifndef UILOG_H
#define UILOG_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "string.h"
#include <esp32-hal.h>

enum class UiLogLevel {
  None,     // No warning/error icon shown in the UI
  Warning,
  Error,
  Unchanged // Indicates no change to the log level
};

// TODO the buffer is slow combined with the UI and a bit buggy, improve it later
class UiLog {
public:
  static UiLog& getInstance() {
    static UiLog instance;
    return instance;
  }

  void addLogEntry(const char* txt, UiLogLevel level = UiLogLevel::Unchanged) {
    if (xSemaphoreTake(mutex, portMAX_DELAY)) {
      if (level != UiLogLevel::Unchanged) {
        this->level = level;
      }

      float timeInSeconds = esp_timer_get_time() / 1000000.0;
      uint16_t newEntryLen = snprintf(nullptr, 0, "[%.2f] %s\n", timeInSeconds, txt);

      if (newEntryLen >= bufferSize) {
        newEntryLen = bufferSize - 1;
      }

      // If there's not enough space at the end of the buffer, wrap around
      if (bufferPointer + newEntryLen >= bufferSize) {
        bufferPointer = 0;
      }

      snprintf(logBuffer + bufferPointer, newEntryLen + 1, "[%.2f] %s\n", timeInSeconds, txt);
      bufferPointer = (bufferPointer + newEntryLen) % bufferSize;

      xSemaphoreGive(mutex);
    }
  }

  void clearLog() {
    if (xSemaphoreTake(mutex, portMAX_DELAY)) {
      clearLogNoLock();
      xSemaphoreGive(mutex);
    }
  }

  const char* getLogAsCString() const {
    if (xSemaphoreTake(mutex, portMAX_DELAY)) {
      uint16_t lengthToEnd = bufferSize - bufferPointer;
      if (lengthToEnd > 0 && logBuffer[bufferPointer] == '\x00') {
        xSemaphoreGive(mutex);
        return logBuffer; // Buffer hasn't wrapped yet
      } else {
        // Buffer wrapped around, needs to concatenate parts
        static char concatenatedBuffer[bufferSize];
        snprintf(concatenatedBuffer, bufferSize, "%s%s", logBuffer + bufferPointer, logBuffer);
        xSemaphoreGive(mutex);
        return concatenatedBuffer;
      }
    }
    return nullptr;
  }

  UiLogLevel getLevel() const {
    UiLogLevel currentLevel;
    if (xSemaphoreTake(mutex, portMAX_DELAY)) {
      currentLevel = level;
      xSemaphoreGive(mutex);
    }
    return currentLevel;
  }

  void setLevel(UiLogLevel newLevel) {
    if (xSemaphoreTake(mutex, portMAX_DELAY)) {
      level = newLevel;
      xSemaphoreGive(mutex);
    }
  }

private:
  UiLog() {
    mutex = xSemaphoreCreateMutex();
    clearLogNoLock();
  }

  ~UiLog() {
    vSemaphoreDelete(mutex);
  }

  UiLog(const UiLog&) = delete;
  UiLog& operator=(const UiLog&) = delete;

  void clearLogNoLock() {
    bufferPointer = 0;
    logBuffer[0] = '\x00';
    level = UiLogLevel::None;
  }

  static const uint16_t bufferSize = 500;
  char logBuffer[bufferSize];
  uint16_t bufferPointer = 0;
  mutable SemaphoreHandle_t mutex;
  UiLogLevel level = UiLogLevel::None;
};

// Shorthand function for logging to the UI
inline void logUi(const char* txt, UiLogLevel level = UiLogLevel::Unchanged) {
  UiLog::getInstance().addLogEntry(txt, level);
}

#endif // UILOG_H
