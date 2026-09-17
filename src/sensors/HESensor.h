#ifndef HE_SENSOR_H
#define HE_SENSOR_H

#include <Adafruit_ADS1X15.h>
#include "conversions.h"
#include "adc_read.h"

struct HEReading {
  float millivolts = NAN;
  float percentage = NAN;
};;

class HESensor {
public:
  HESensor(Adafruit_ADS1115 &adc) : _adc(adc) {}

  void setCalibrations(float calibration100) {
    _calibration100 = calibration100;
  }

  HEReading readLevel(float o2Percentage, bool* timedOut = nullptr) {
    if (timedOut) *timedOut = false;
    HEReading reading;
    int16_t adcValue;
    if (!acquisition::readCounts(_adc, ADS1X15_REG_CONFIG_MUX_DIFF_0_1, adcValue)) {
      if (timedOut) *timedOut = true;
      return reading;
    }
    reading.millivolts = abs(_adc.computeVolts(adcValue) * 1000);

    // In case of high oxygen percentage, the He sensor also reads higher voltage. E.g. for 100, the He voltage is 4-5mv higher
    // Values based on the code here, which comes from the original French presentation https://scubaboard.com/community/threads/nitrox-trimix-co-analyzer.595564/page-4#post-9084208
    const float correctedMillivolts = conversions::heCorrectedMillivolts(reading.millivolts, o2Percentage);

    // ================ Sensor calibration ===================
    // The sensor is not linear, so the reading has to be corrected. The polynomial formula below is derived from multiple readings
    // comparing each one to a Divesoft SOLO analyzer as a reference. The formula looks like a good approximation.

    // Readings data
    // mv,      MDD61 %,  Divesoft %
    // 51.56,   8.6,      8.1
    // 65.19,   10.9,     9.5
    // 109.25,  18.3,     15.5
    // 139.63,  23.4,     19.7
    // 177.94,  29.8,     25.2
    // 211.75,  35.4,     30
    // 245.63,  41.1,     34.9
    // 279.19,  46.7,     39.9
    // 311.5,   52.1,     45
    // 344.56,  57.6,     50
    // 376.75,  63,       54.9
    // 408.19,  68.3,     59.9
    // 439.81,  73.6,     65.1
    // 468.5,   78.4,     69.9
    // 497.56,  83.2,     74.9
    // 524.06,  87.7,     80
    // 543.69,  90.9,     84.9
    // 575.81,  96.3,     90.3
    // 597.5,   99.9,     95.2
    // 624.25,  104.4,    100.5

    reading.percentage = conversions::hePercentage(correctedMillivolts, _calibration100);

    return reading;
  }

private:
    Adafruit_ADS1115 &_adc;
    // The calibration coeficitients for air and 100% HE
    float _calibration100 = NAN;
};

#endif // HESENSOR_H