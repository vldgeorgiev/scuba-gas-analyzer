#ifndef TempSensor_H
#define TempSensor_H

#include <Adafruit_ADS1X15.h>

// Reading for sensor LM35DZ, which is calibratedd for Celsius. 10mV correspond to 1 °C
class TempSensor {
public:
  TempSensor(uint8_t analogChannel, Adafruit_ADS1115& adc) : _analogChannel(analogChannel), _adc(adc) {}

  float readLevel() {
    int16_t adcValue = _adc.readADC_SingleEnded(_analogChannel);
    float millivolts = _adc.computeVolts(adcValue) * 1000;
    return round(millivolts * 10) / 10 / 10; // 10mV correspond to 1 °C. Round to single decimal
  }

private:
  uint8_t _analogChannel;
  Adafruit_ADS1115& _adc;
};

#endif // TempSensor_H
