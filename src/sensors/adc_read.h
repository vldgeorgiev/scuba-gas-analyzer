#ifndef SENSOR_ADC_READ_H
#define SENSOR_ADC_READ_H

#include <Adafruit_ADS1X15.h>

namespace acquisition {

constexpr uint32_t CONVERSION_TIMEOUT_MS = 25;

inline bool readCounts(Adafruit_ADS1115& adc, uint16_t mux, int16_t& counts) {
  const uint32_t started = ::millis();
  adc.startADCReading(mux, false);
  for (;;) {
    const bool complete = adc.conversionComplete();
    if (static_cast<uint32_t>(::millis() - started) >= CONVERSION_TIMEOUT_MS) return false;
    if (complete) break;
    delay(1);
  }
  const int16_t result = adc.getLastConversionResults();
  if (static_cast<uint32_t>(::millis() - started) >= CONVERSION_TIMEOUT_MS) return false;
  counts = result;
  return true;
}

}

#endif