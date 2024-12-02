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

    // Converts millivolt readings to gas percentage using a polynomial formula, because the sensor is not linear.
    // The formula was derived from about 30 spread calibration readings compared to a Divesoft gas analyzer.
    // It provides an approximation of gas content percentage based on millivolt input.
    // TODO: Test that formula and adjust it if needed. See if the 100% calibration is needed.
    reading.percentage = - 0.59648617
                         + 0.172264821 * reading.millivolts
                         - 0.000435123885 * pow(reading.millivolts, 2)
                         + 2.42978296e-6 * pow(reading.millivolts, 3)
                         - 5.39338629e-9 * pow(reading.millivolts, 4)
                         + 4.27338394e-12 * pow(reading.millivolts, 5);

    // reading.percentage = 100 * reading.millivolts / _calibration100; // Formula which ignores non-linearity
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