#ifndef COSensor_H
#define COSensor_H

#include <Adafruit_ADS1X15.h>
#include <RunningAverage.h>

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

    // Convert voltage to ppm
    // if (reading.millivolts < V_MIN) reading.millivolts = V_MIN;  // Ensure the voltage is within expected range. Disable during testing to monitor deviations
    // if (reading.millivolts > V_MAX) reading.millivolts = V_MAX;
    reading.ppm = (reading.millivolts - V_MIN) * (PPM_MAX / (V_MAX - V_MIN));
    return reading;
  }

private:
  uint8_t _analogChannel;
  Adafruit_ADS1115& _adc;
  const float V_MIN = 400;  // Minimum output voltage mv (corresponding to 0 ppm)
  const float V_MAX = 2000;  // Maximum output voltage mv (corresponding to full scale, e.g., 500 ppm)
  const float PPM_MAX = 500.0;  // Maximum ppm the sensor can read
  const int RUNNING_AVG_SIZE = 20; // Must be before the average object
  RunningAverage _average;
};

#endif // COSensor_H
