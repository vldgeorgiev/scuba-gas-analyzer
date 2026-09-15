#ifndef TEST_ARDUINO_H
#define TEST_ARDUINO_H

#include <cstdint>
#include "ui-log.h"

constexpr int HIGH = 1;
constexpr int LOW = 0;
constexpr int OUTPUT = 1;
inline int pinValues[64] = {};
inline void pinMode(int, int) {}
inline void digitalWrite(int pin, int value) { pinValues[pin] = value; }

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