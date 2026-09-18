#ifndef TempSensor_H
#define TempSensor_H

#include <Adafruit_ADS1X15.h>
#include "conversions.h"
#include "adc_read.h"

// Reading for sensor LM35DZ, which is calibratedd for Celsius. 10mV correspond to 1 °C
class TempSensor {
public:
  TempSensor(uint8_t analogChannel, Adafruit_ADS1115& adc) : _analogChannel(analogChannel), _adc(adc) {}

  float readLevel(bool* timedOut = nullptr) {
    if (timedOut) *timedOut = false;
    if (_analogChannel > 3) return NAN;
    int16_t adcValue;
    if (!acquisition::readCounts(_adc, MUX_BY_CHANNEL[_analogChannel], adcValue)) {
      if (timedOut) *timedOut = true;
      return NAN;
    }
    float millivolts = _adc.computeVolts(adcValue) * 1000;
    return conversions::temperatureCelsius(millivolts);
  }

private:
  uint8_t _analogChannel;
  Adafruit_ADS1115& _adc;
};

#endif // TempSensor_H
