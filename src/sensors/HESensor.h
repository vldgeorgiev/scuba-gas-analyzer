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

  HEReading readLevel() {
    if (!_average.clear())
      log_e("Failed to init average");

    for (int i = 0; i < RUNNING_AVG_SIZE; i++)
    {
      int16_t adcValue = _adc.readADC_Differential_2_3();
      _average.addValue(adcValue);
    }
    HEReading reading;
    reading.millivolts = abs(_adc.computeVolts(_average.getAverage()) * 1000);
    reading.percentage = 100 * reading.millivolts / _calibration100;
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