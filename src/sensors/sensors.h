#ifndef SENSORS_H
#define SENSORS_H

#include <Adafruit_ADS1X15.h>
#include "O2Sensor.h"
#include "COSensor.h"
#include "HESensor.h"
#include "TempSensor.h"

struct sensorsData {
  O2Reading O2Level;
  COReading CoLevel;
  HEReading HeLevel;
  float HeTemperature;
};

class SensorManager {
public:
    SensorManager(QueueHandle_t& dataQueue);

    void init();
    void setSensorsConfig(bool isO2Enabled, bool isCOEnabled, bool isHeEnabled, float o2Calibration21, float o2Calibration100, float heCalibration100);
    void readSensors();
    float calibrateO2_21();
    float calibrateO2_100();
    float calibrateHe_100();

private:
    #define ADC1_ADDRESS 0x48 // Default one
    #define ADC2_ADDRESS 0x49 // Default one
    #define ADC2_CHANNEL_CO 3 // The ADS1115 channel for CO sensor
    #define ADC2_CHANNEL_TEMP 2 // The ADS1115 channel for Temp sensor
    #define ADC1_GAIN GAIN_FOUR // 4x gain   +/- 1.024V  1 bit = 0.03125mV. For O2 (~10-45mv) and He (~0-600mv) outputs
    #define ADC2_GAIN GAIN_TWO // 2x gain   +/- 2.048V  1 bit = 0.0625mV. For CO (400-2000mv) output
    Adafruit_ADS1115 _adc1;
    Adafruit_ADS1115 _adc2;
    QueueHandle_t& _dataQueue;
    O2Sensor _o2Sensor;
    COSensor _coSensor;
    HESensor _heSensor;
    TempSensor _tempSensor;

    bool _isO2Enabled;
    bool _isCOEnabled;
    bool _isHeEnabled;
    float _o2Calibration21 = NAN;
    float _o2Calibration100 = NAN;
    float _heCalibration100 = NAN;
};

#endif // SENSORS_H
