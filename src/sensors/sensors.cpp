#include "sensors.h"
#include "O2Sensor.h"
#include "COSensor.h"
#include "HESensor.h"
#include "pin_config.h"

SensorManager::SensorManager(QueueHandle_t& dataQueue) :
  _adc1(),
  _adc2(),
  _o2Sensor(_adc1),
  _heSensor(_adc1),
  _coSensor(ADC2_CHANNEL_CO, _adc2),
  _dataQueue(dataQueue)
{}

void SensorManager::init() {
  Wire1.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  _adc1.setGain(ADC1_GAIN);
  _adc2.setGain(ADC2_GAIN);
  _adc1.begin(ADC1_ADDRESS, &Wire1);
  _adc2.begin(ADC2_ADDRESS, &Wire1);
}

void SensorManager::setSensorsConfig(bool isO2Enabled, bool isCOEnabled, bool isHeEnabled, float o2Calibration21, float o2Calibration100, float heCalibration100) {
  log_d("Setting sensors config: O2 %d, CO %d, O2 21%% %.2f, O2 100%% %.2f",
      isO2Enabled, isCOEnabled, o2Calibration21, o2Calibration100);
  _isO2Enabled = isO2Enabled;
  _isCOEnabled = isCOEnabled;
  _isHeEnabled = isHeEnabled;
  _o2Calibration21 = o2Calibration21;
  _o2Calibration100 = o2Calibration100;
  _heCalibration100 = heCalibration100;
  _o2Sensor.setCalibrations(_o2Calibration21, _o2Calibration100);
  _heSensor.setCalibrations(_heCalibration100);
}

void SensorManager::readSensors() {
  sensorsData data;
  data.O2Level.millivolts = NAN;
  data.O2Level.percentage = NAN;
  data.CoLevel.millivolts = NAN;
  data.HeLevel.millivolts = NAN;
  data.HeLevel.percentage = NAN;

  if (!_isO2Enabled && !_isCOEnabled && !_isHeEnabled)
    return; // No need to read if no sensors are enabled

  if (_isO2Enabled)
    data.O2Level = _o2Sensor.readLevel();
  if (_isCOEnabled)
    data.CoLevel = _coSensor.readLevel();
  if (_isHeEnabled)
    data.HeLevel = _heSensor.readLevel(data.O2Level.percentage); // He sensor needs correction in high O2 environment

  xQueueSend(_dataQueue, &data, portMAX_DELAY);
  // log_d("Sensors: O2 %.2f%%/%.2fmv, CO %dppm/%.2fmv He %.2f%%/%.2fmv",
  //     data.O2Level.percentage, data.O2Level.millivolts, data.CoLevel.ppm, data.CoLevel.millivolts, data.HeLevel.percentage, data.HeLevel.millivolts);
}

float SensorManager::calibrateO2_21() {
  log_d("Calibrating O2 sensor for 21%% O2...");
  _o2Calibration21 = _o2Sensor.calibrate();
  _o2Sensor.setCalibrations(_o2Calibration21, _o2Calibration100);
  return _o2Calibration21;
}

float SensorManager::calibrateO2_100() {
  log_d("Calibrating O2 sensor for 100%% O2...");
  _o2Calibration100 = _o2Sensor.calibrate();
  _o2Sensor.setCalibrations(_o2Calibration21, _o2Calibration100);
  return _o2Calibration100;
}

float SensorManager::calibrateHe_100() {
  log_d("Calibrating He sensor for 100%% He...");
  _heCalibration100 = _heSensor.calibrate();
  _heSensor.setCalibrations(_heCalibration100);
  return _heCalibration100;
}