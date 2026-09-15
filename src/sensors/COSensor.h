#ifndef COSensor_H
#define COSensor_H

#include <Adafruit_ADS1X15.h>
#include <RunningAverage.h>
#include "conversions.h"

struct COReading {
  float millivolts;
  int ppm;
};

// Readings are based on the usage of the ZE07-CO sensor, which outputs a voltage between 0.4V and 2V, corresponding to 0 and 500 ppm
// The ZE07-CO sensor provides a UART digital output for reading PPM directly, but since we are using the ADS, it is easer
// to use the analog voltage rather than wait for the UART data to be ready
class COSensor {
public:
  COSensor(uint8_t analogChannel, Adafruit_ADS1115& adc) : _analogChannel(analogChannel), _adc(adc), _average(RUNNING_AVG_SIZE) {}

  COReading readLevel() {
    if (!_average.clear())
      log_e("Failed to init average");

    for (int i = 0; i < RUNNING_AVG_SIZE; i++)
    {
      int16_t adcValue = _adc.readADC_SingleEnded(_analogChannel);
      _average.addValue(adcValue);
    }
    COReading reading;
    reading.millivolts = _adc.computeVolts(_average.getAverage()) * 1000;

    reading.ppm = conversions::coPpm(reading.millivolts);
    return reading;
  }

private:
  uint8_t _analogChannel;
  Adafruit_ADS1115& _adc;
  const int RUNNING_AVG_SIZE = 20; // Must be before the average object
  RunningAverage _average;
};

#endif // COSensor_H
