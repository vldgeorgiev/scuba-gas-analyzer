#ifndef TEST_ADAFRUIT_ADS1X15_H
#define TEST_ADAFRUIT_ADS1X15_H

#include <cmath>
#include <cstdint>

using std::abs;
using std::isnan;

inline unsigned delayCalls = 0;
inline unsigned delayedMs = 0;
inline void delay(unsigned milliseconds) {
  ++delayCalls;
  delayedMs += milliseconds;
}

class Adafruit_ADS1115 {
public:
  int16_t counts = 0;
  float millivoltsPerCount = 0.0625f;
  unsigned differential23Reads = 0;
  unsigned differential01Reads = 0;
  unsigned singleEndedReads = 0;
  uint8_t lastChannel = 0;

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