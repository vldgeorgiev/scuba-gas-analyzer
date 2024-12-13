#ifndef HE_SENSOR_H
#define HE_SENSOR_H

#include <Adafruit_ADS1X15.h>
#include <RunningAverage.h>
#include "ui-log.h"

struct HEReading {
  float millivolts;
  float percentage;
};;

class HESensor {
public:
  HESensor(Adafruit_ADS1115 &adc) : _adc(adc), _average(RUNNING_AVG_SIZE) {}

  void setCalibrations(float calibration100) {
    _calibration100 = calibration100;
  }

  HEReading readLevel(float o2Percentage) {
    if (!_average.clear())
      log_e("Failed to init average");

    for (int i = 0; i < RUNNING_AVG_SIZE; i++)
    {
      int16_t adcValue = _adc.readADC_Differential_2_3();
      _average.addValue(adcValue);
    }
    HEReading reading;
    reading.millivolts = abs(_adc.computeVolts(_average.getAverage()) * 1000);

    // In case of high oxygen percentage, the He sensor also reads higher voltage. E.g. for 100, the He voltage is 4-5mv higher
    // Values based on the code here, which comes from the original French presentation https://scubaboard.com/community/threads/nitrox-trimix-co-analyzer.595564/page-4#post-9084208
    if (o2Percentage > 40) {
      if (o2Percentage > 89) { reading.millivolts = reading.millivolts - 17; }
      else if (o2Percentage > 82) { reading.millivolts -= 16;}
      else if (o2Percentage > 75) { reading.millivolts -= 15;}
      else if (o2Percentage > 71) { reading.millivolts -= 14;}
      else if (o2Percentage > 66) { reading.millivolts -= 13;}
      else if (o2Percentage > 62) { reading.millivolts -= 12;}
      else if (o2Percentage > 57) { reading.millivolts -= 11;}
      else if (o2Percentage > 52) { reading.millivolts -= 10;}
      else if (o2Percentage > 48) { reading.millivolts -= 9;}
    }

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

    // This is max reading from the polynomial formula for 100% He. The polynomial was derived for specific milivolts, so when
    // calibrating, the whole curve has to be adjusted before calculating the polynomial.
    const double FIXED_MAX_MV_100 = 621.2;
    double adjustmentFactor = FIXED_MAX_MV_100 / _calibration100;
    double adjustedMv = reading.millivolts * adjustmentFactor;

    reading.percentage =
      + 1.098e-7 * pow(adjustedMv, 3)
      - 4.584e-5 * pow(adjustedMv, 2)
      + (0.1471 * adjustedMv);

    return reading;
  }

  float calibrate() {
    // Is taking more reading and the delay needed? Does it increase the accuracy?
    RunningAverage calibrateAvg(CALIBRATION_COUNT * RUNNING_AVG_SIZE);
    for (int i = 0; i < CALIBRATION_COUNT; i++)
    {
      for (int j = 0; j < RUNNING_AVG_SIZE; j++)
      {
        int16_t adcValue = _adc.readADC_Differential_2_3();
        calibrateAvg.addValue(adcValue);
      }
      delay(CALIBRATION_DELAY);
    }
    float millivolts = abs(_adc.computeVolts(calibrateAvg.getAverage()) * 1000);
    return millivolts;
  }

private:
    Adafruit_ADS1115 &_adc;
    const int RUNNING_AVG_SIZE = 20; // Must be before the average object
    RunningAverage _average;
    // The calibration coeficitients for air and 100% HE
    float _calibration100 = NAN;
    const int CALIBRATION_COUNT = 5;
    const int CALIBRATION_DELAY = 100;
};

#endif // HESENSOR_H