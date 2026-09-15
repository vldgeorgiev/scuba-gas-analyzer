#ifndef TEST_ARDUINO_H
#define TEST_ARDUINO_H

#include <cstdint>

inline unsigned delayCalls = 0;
inline unsigned delayedMs = 0;
inline uint32_t nowMs = 0;
inline uint32_t millis() { return nowMs; }
inline void delay(unsigned milliseconds) {
  ++delayCalls;
  delayedMs += milliseconds;
  nowMs += milliseconds;
}

#endif