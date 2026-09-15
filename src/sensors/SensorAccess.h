#ifndef SENSOR_ACCESS_H
#define SENSOR_ACCESS_H

#include <atomic>
#include <Arduino.h>

class SensorAccess {
public:
  bool tryAcquire() {
    bool expected = false;
    return _busy.compare_exchange_strong(expected, true, std::memory_order_acquire);
  }

  bool acquire(uint32_t timeoutMs = 200) {
    const uint32_t started = ::millis();
    if (tryAcquire()) return true;
    while (static_cast<uint32_t>(::millis() - started) < timeoutMs) {
      delay(1);
      if (static_cast<uint32_t>(::millis() - started) >= timeoutMs) break;
      if (tryAcquire()) return true;
    }
    return false;
  }

  void release() { _busy.store(false, std::memory_order_release); }

private:
  std::atomic<bool> _busy{false};
};

#endif