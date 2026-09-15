#ifndef TEST_ADAFRUIT_ADS1X15_H
#define TEST_ADAFRUIT_ADS1X15_H

#include <cmath>
#include <cstdint>
#include "FreeRTOS.h"

using std::abs;
using std::isnan;

inline unsigned delayCalls = 0;
inline unsigned delayedMs = 0;
inline uint32_t nowMs = 0;
inline uint32_t millis() { return nowMs; }
inline void delay(unsigned milliseconds) {
  ++delayCalls;
  delayedMs += milliseconds;
}

enum adsGain_t { GAIN_TWO, GAIN_FOUR };
struct TwoWire {
  void begin(int, int) {}
};
inline TwoWire Wire1;

class Adafruit_ADS1115 {
public:
  inline static Adafruit_ADS1115* devices[2] = {};
  int16_t counts = 0;
  float millivoltsPerCount = 0.0625f;
  unsigned differential23Reads = 0;
  unsigned differential01Reads = 0;
  unsigned singleEndedReads = 0;
  uint8_t lastChannel = 0;

  bool begin(uint8_t address, TwoWire*) {
    devices[address == 0x49 ? 0 : 1] = this;
    return true;
  }
  void setGain(adsGain_t gain) {
    millivoltsPerCount = gain == GAIN_FOUR ? 0.03125f : 0.0625f;
  }

  int16_t readADC_Differential_2_3() {
    ++differential23Reads;
    return counts;
  }
  int16_t readADC_Differential_0_1() {
    ++differential01Reads;
    return counts;
  }
  int16_t readADC_SingleEnded(uint8_t channel) {
    ++singleEndedReads;
    lastChannel = channel;
    return counts;
  }
  float computeVolts(int16_t value) { return value * millivoltsPerCount / 1000.0f; }
};

#endif