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
    O2Reading reading;
    reading.millivolts = NAN;
    reading.percentage = NAN;

    // Validate calibration before reading
    if (isnan(_calibration21) || _calibration21 < MIN_VALID_O2_MV_21) {
      log_e("Invalid O2 calibration: %.2f mV", _calibration21);
      return reading;
    }

    if (!_average.clear()) {
      log_e("Failed to init running average");
      return reading;
    }

    // Take multiple readings for stability
    for (int i = 0; i < RUNNING_AVG_SIZE; i++) {
      int16_t adcValue = _adc.readADC_Differential_0_1();
      _average.addValue(adcValue);
    }

    reading.millivolts = abs(_adc.computeVolts(_average.getAverage()) * 1000);

    // Validate raw reading
    if (reading.millivolts < 0 || reading.millivolts > 100) { // Reasonable range for O2 sensor
      log_w("O2 reading out of range: %.2f mV", reading.millivolts);
      return reading;
    }

    // Calculate percentage based on calibration
    if (!isnan(_calibration100)) {
      // Linear drift correction with 100% calibration
      reading.percentage = 20.9 + 79.1*(reading.millivolts - _calibration21)/(_calibration100 - _calibration21);
    } else {
      // Simple linear scaling with air calibration only
      reading.percentage = 20.9 / _calibration21 * reading.millivolts;
    }

    // Clamp percentage to reasonable range
    if (reading.percentage < 0) reading.percentage = 0;
    if (reading.percentage > 100) reading.percentage = 100;

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