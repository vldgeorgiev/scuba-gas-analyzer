#ifndef O2_SENSOR_H
#define O2_SENSOR_H

#include <Adafruit_ADS1X15.h>
#include <RunningAverage.h>
#include "ui-log.h"

struct O2Reading {
  float millivolts;
  float percentage;
};;

class O2Sensor {
public:
  O2Sensor(Adafruit_ADS1115 &adc) : _adc(adc), _average(RUNNING_AVG_SIZE) {}

  void setCalibrations(float calibration21, float calibration100) {
    // The 100% may not be defined, but the 21% must always be valid
    if (calibration21 <= MIN_VALID_O2_MV_21)
      log_e("Invalid O2 air calibration %.2f mv", calibration21);

    if (!isnan(calibration100) && (calibration100 <= calibration21)) {
      log_e("O2 100%% calibration mv less than air %.2f mv/%.2f mv", calibration21, calibration100);
      logUi("O2 100%% calibration mv less than air", UiLogLevel::Error);
    }

    _calibration21 = calibration21;
    _calibration100 = calibration100;
  }

  O2Reading readLevel() {
    if (!_average.clear())
      log_e("Failed to init average");

    for (int i = 0; i < RUNNING_AVG_SIZE; i++)
    {
      int16_t adcValue = _adc.readADC_Differential_0_1();
      _average.addValue(adcValue);
    }
    O2Reading reading;
    reading.millivolts = abs(_adc.computeVolts(_average.getAverage()) * 1000);
    if (!isnan(_calibration100))
      // See https://scubaboard.com/community/threads/nitrox-trimix-co-analyzer.595564/post-9014843 for linear drift correction
      reading.percentage = 20.9 + 79.1*(reading.millivolts - _calibration21)/(_calibration100 - _calibration21);
    else
      reading.percentage = 20.9 / _calibration21 * reading.millivolts;
    return reading;
  }

  float calibrate() {
    // Is taking more reading and the delay needed? Does it increase the accuracy?
    RunningAverage calibrateAvg(CALIBRATION_COUNT * RUNNING_AVG_SIZE);
    for (int i = 0; i < CALIBRATION_COUNT; i++)
    {
      for (int j = 0; j < RUNNING_AVG_SIZE; j++)
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
    const int RUNNING_AVG_SIZE = 20; // Must be before the average object
    RunningAverage _average;
    // The calibration coeficitients for air and 100% O2
    float _calibration21 = NAN;
    float _calibration100 = NAN;
    const float MIN_VALID_O2_MV_21 = 5;
    const int CALIBRATION_COUNT = 5;
    const int CALIBRATION_DELAY = 100;
};

#endif // O2SENSOR_H