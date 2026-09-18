#ifndef O2_SENSOR_H
#define O2_SENSOR_H

#include <Adafruit_ADS1X15.h>
#include "app/AnalyzerPolicy.h"
#include <cmath>
#include "conversions.h"
#include "adc_read.h"

struct O2Reading {
  float millivolts = NAN;
  float percentage = NAN;
};;

class O2Sensor {
public:
  O2Sensor(Adafruit_ADS1115 &adc) : _adc(adc) {}

  void setCalibrations(float calibration21, float calibration100) {
    // The 100% may not be defined, but the 21% must always be valid
    if (!std::isfinite(calibration21) || calibration21 < app::policy::O2_AIR_CALIBRATION_MIN_MV ||
      calibration21 > app::policy::O2_AIR_CALIBRATION_MAX_MV)
      log_e("Invalid O2 air calibration %.2f mv", calibration21);

    if (!isnan(calibration100) && (calibration100 <= calibration21)) {
      log_e("O2 100%% calibration mv less than air %.2f mv/%.2f mv", calibration21, calibration100);
    }

    _calibration21 = calibration21;
    _calibration100 = calibration100;
  }

  O2Reading readLevel(bool* timedOut = nullptr) {
    if (timedOut) *timedOut = false;
    O2Reading reading;

    int16_t adcValue;
    if (!acquisition::readCounts(_adc, ADS1X15_REG_CONFIG_MUX_DIFF_2_3, adcValue)) {
      if (timedOut) *timedOut = true;
      return reading;
    }
    reading.millivolts = abs(_adc.computeVolts(adcValue) * 1000);

    // Validate raw reading
    if (reading.millivolts < 0 || reading.millivolts > 100) { // Reasonable range for O2 sensor
      log_w("O2 reading out of range: %.2f mV", reading.millivolts);
      return reading;
    }

    reading.percentage = conversions::o2Percentage(reading.millivolts, _calibration21, _calibration100);

    return reading;
  }

private:
    Adafruit_ADS1115 &_adc;
    // The calibration coeficitients for air and 100% O2
    float _calibration21 = NAN;
    float _calibration100 = NAN;
};

#endif // O2SENSOR_H