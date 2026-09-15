#ifndef COSensor_H
#define COSensor_H

#include <Adafruit_ADS1X15.h>
#include "conversions.h"
#include "adc_read.h"

struct COReading {
  float millivolts = NAN;
  float ppm = NAN;
};

// Readings are based on the usage of the ZE07-CO sensor, which outputs a voltage between 0.4V and 2V, corresponding to 0 and 500 ppm
// The ZE07-CO sensor provides a UART digital output for reading PPM directly, but since we are using the ADS, it is easer
// to use the analog voltage rather than wait for the UART data to be ready
class COSensor {
public:
  COSensor(uint8_t analogChannel, Adafruit_ADS1115& adc) : _analogChannel(analogChannel), _adc(adc) {}

  COReading readLevel(bool* timedOut = nullptr) {
    if (timedOut) *timedOut = false;
    COReading reading;
    if (_analogChannel > 3) return reading;
    int16_t adcValue;
    if (!acquisition::readCounts(_adc, MUX_BY_CHANNEL[_analogChannel], adcValue)) {
      if (timedOut) *timedOut = true;
      return reading;
    }
    reading.millivolts = _adc.computeVolts(adcValue) * 1000;

    reading.ppm = conversions::coPpm(reading.millivolts);
    return reading;
  }

private:
  uint8_t _analogChannel;
  Adafruit_ADS1115& _adc;
};

#endif // COSensor_H
