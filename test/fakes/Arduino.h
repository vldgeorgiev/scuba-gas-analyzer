#ifndef TEST_ARDUINO_H
#define TEST_ARDUINO_H

#include <cstdint>
#include <array>
#include "ui-log.h"

constexpr int HIGH = 1;
constexpr int LOW = 0;
constexpr int OUTPUT = 1;
constexpr int INPUT_PULLUP = 2;
inline int pinValues[64] = {};
inline std::array<int, 64> pinInputs = [] {
  std::array<int, 64> values;
  values.fill(HIGH);
  return values;
}();
inline void pinMode(int, int) {}
inline void digitalWrite(int pin, int value) { pinValues[pin] = value; }
inline int digitalRead(int pin) { return pinInputs[pin]; }

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