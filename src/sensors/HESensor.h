#ifndef HE_SENSOR_H
#define HE_SENSOR_H

#include <Adafruit_ADS1X15.h>
#include <RunningAverage.h>
#include "ui-log.h"
#include "conversions.h"

struct HEReading {
  float millivolts;
  float percentage;
};;

class HESensor {
public:
  HESensor(Adafruit_ADS1115 &adc) : _adc(adc) {}

  void setCalibrations(float calibration100) {
    _calibration100 = calibration100;
  }

  HEReading readLevel(float o2Percentage) {
    const int16_t adcValue = _adc.readADC_Differential_0_1();
    HEReading reading;
    reading.millivolts = abs(_adc.computeVolts(adcValue) * 1000);

    // In case of high oxygen percentage, the He sensor also reads higher voltage. E.g. for 100, the He voltage is 4-5mv higher
    // Values based on the code here, which comes from the original French presentation https://scubaboard.com/community/threads/nitrox-trimix-co-analyzer.595564/page-4#post-9084208
    reading.millivolts = conversions::heCorrectedMillivolts(reading.millivolts, o2Percentage);

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

    reading.percentage = conversions::hePercentage(reading.millivolts, _calibration100);

    return reading;
  }

  float calibrate() {
    // Is taking more reading and the delay needed? Does it increase the accuracy?
    RunningAverage calibrateAvg(CALIBRATION_COUNT * CALIBRATION_SAMPLES_PER_BATCH);
    for (int i = 0; i < CALIBRATION_COUNT; i++)
    {
      for (int j = 0; j < CALIBRATION_SAMPLES_PER_BATCH; j++)
      {
        int16_t adcValue = _adc.readADC_Differential_0_1();
        calibrateAvg.addValue(adcValue);
      }
      delay(CALIBRATION_DELAY);
    }
    float millivolts = abs(_adc.computeVolts(calibrateAvg.getAverage()) * 1000);
    return millivolts;
  }

private:
    Adafruit_ADS1115 &_adc;
    const int CALIBRATION_SAMPLES_PER_BATCH = 20;
    // The calibration coeficitients for air and 100% HE
    float _calibration100 = NAN;
    const int CALIBRATION_COUNT = 5;
    const int CALIBRATION_DELAY = 100;
};

#endif // HESENSOR_H