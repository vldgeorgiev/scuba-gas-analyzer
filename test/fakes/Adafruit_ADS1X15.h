#ifndef TEST_ADAFRUIT_ADS1X15_H
#define TEST_ADAFRUIT_ADS1X15_H

#include <cmath>
#include <cstdint>
#include "FreeRTOS.h"
#include "Arduino.h"

using std::abs;
using std::isnan;

constexpr uint16_t ADS1X15_REG_CONFIG_MUX_DIFF_0_1 = 0x0000;
constexpr uint16_t ADS1X15_REG_CONFIG_MUX_DIFF_2_3 = 0x3000;
constexpr uint16_t RATE_ADS1115_128SPS = 0x0080;
constexpr uint16_t MUX_BY_CHANNEL[4] = {0x4000, 0x5000, 0x6000, 0x7000};

enum adsGain_t { GAIN_TWO, GAIN_FOUR };
struct TwoWire {
  uint16_t timeoutMs = 0;
  void begin(int, int) {}
  void setTimeOut(uint16_t timeout) { timeoutMs = timeout; }
};
inline TwoWire Wire1;

class Adafruit_ADS1115 {
public:
  inline static Adafruit_ADS1115* devices[2] = {};
  inline static bool present[2] = {true, true};
  unsigned beginCalls = 0;
  uint16_t dataRate = 0;
  int16_t counts = 0;
  float millivoltsPerCount = 0.0625f;
  unsigned differential23Reads = 0;
  unsigned differential01Reads = 0;
  unsigned singleEndedReads = 0;
  uint8_t lastChannel = 0;
  bool conversionCompletes = true;
  unsigned successfulConversions = UINT32_MAX;
  unsigned resultReads = 0;
  uint32_t completeAfterMs = 0;
  uint32_t resultDelayMs = 0;
  uint32_t startedMs = 0;
  bool continuous = true;

  void startADCReading(uint16_t mux, bool continuousMode) {
    continuous = continuousMode;
    startedMs = nowMs;
    if (mux == ADS1X15_REG_CONFIG_MUX_DIFF_2_3) ++differential23Reads;
    else if (mux == ADS1X15_REG_CONFIG_MUX_DIFF_0_1) ++differential01Reads;
    else {
      ++singleEndedReads;
      lastChannel = static_cast<uint8_t>((mux - 0x4000) / 0x1000);
    }
  }
  bool conversionComplete() {
        return conversionCompletes && resultReads < successfulConversions &&
          static_cast<uint32_t>(nowMs - startedMs) >= completeAfterMs;
  }
  int16_t getLastConversionResults() {
    ++resultReads;
    nowMs += resultDelayMs;
    return counts;
  }

  bool begin(uint8_t address, TwoWire*) {
    ++beginCalls;
    devices[address == 0x49 ? 0 : 1] = this;
    return present[address == 0x49 ? 0 : 1];
  }
  void setDataRate(uint16_t rate) { dataRate = rate; }
  void setGain(adsGain_t gain) {
    millivoltsPerCount = gain == GAIN_FOUR ? 0.03125f : 0.0625f;
  }

  float computeVolts(int16_t value) { return value * millivoltsPerCount / 1000.0f; }
};

#endif