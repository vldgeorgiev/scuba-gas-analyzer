#include "sensors.h"
#include "O2Sensor.h"
#include "COSensor.h"
#include "HESensor.h"
#include "TempSensor.h"
#include "pin_config.h"

SensorManager::SensorManager(QueueHandle_t& dataQueue) :
  _adc1(),
  _adc2(),
  _o2Sensor(_adc1),
  _heSensor(_adc2), // He sensor is on the same ADC as CO and Temp, but uses different channels
  _coSensor(ADC2_CHANNEL_CO, _adc2),
  _tempSensor(ADC2_CHANNEL_TEMP, _adc2),
  _dataQueue(dataQueue)
{}

SensorError SensorManager::init() {
  _lastError = SensorError::None;

  Wire1.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  _adc1.setGain(ADC1_GAIN);
  _adc2.setGain(ADC2_GAIN);

  if (!_adc1.begin(ADC1_ADDRESS, &Wire1)) {
    log_e("Failed to initialize ADC1");
    _lastError = SensorError::ADC_Init_Failed;
    return _lastError;
  }

  if (!_adc2.begin(ADC2_ADDRESS, &Wire1)) {
    log_e("Failed to initialize ADC2");
    _lastError = SensorError::ADC_Init_Failed;
    return _lastError;
  }

  log_i("Sensors initialized successfully");
  return SensorError::None;
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

SensorError SensorManager::readSensors() {
  sensorsData data;
  data.O2Level.millivolts = NAN;
  data.O2Level.percentage = NAN;
  data.CoLevel.millivolts = NAN;
  data.HeLevel.millivolts = NAN;
  data.HeLevel.percentage = NAN;
  data.lastError = SensorError::None;

  if (!_isO2Enabled && !_isCOEnabled && !_isHeEnabled) {
    _lastError = SensorError::Sensor_Not_Enabled;
    data.lastError = _lastError;
    xQueueSend(_dataQueue, &data, portMAX_DELAY);
    return _lastError;
  }

  // Read sensors with basic validation
  if (_isO2Enabled) {
    data.O2Level = _o2Sensor.readLevel();
    if (isnan(data.O2Level.millivolts) || data.O2Level.millivolts < 0) {
      log_w("Invalid O2 reading: %.2f mV", data.O2Level.millivolts);
      _lastError = SensorError::Invalid_Reading;
    }
  }

  if (_isCOEnabled) {
    data.CoLevel = _coSensor.readLevel();
    if (isnan(data.CoLevel.millivolts) || data.CoLevel.millivolts < 0) {
      log_w("Invalid CO reading: %.2f mV", data.CoLevel.millivolts);
      _lastError = SensorError::Invalid_Reading;
    }
  }

  if (_isHeEnabled) {
    data.HeLevel = _heSensor.readLevel(data.O2Level.percentage);
    if (isnan(data.HeLevel.millivolts) || data.HeLevel.millivolts < 0) {
      log_w("Invalid He reading: %.2f mV", data.HeLevel.millivolts);
      _lastError = SensorError::Invalid_Reading;
    }
  }

  data.HeTemperature = _tempSensor.readLevel();
  data.lastError = _lastError;

  xQueueSend(_dataQueue, &data, portMAX_DELAY);
  return _lastError;
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

const char* SensorManager::getErrorString(SensorError error) const {
  switch (error) {
    case SensorError::None: return "No error";
    case SensorError::ADC_Init_Failed: return "ADC initialization failed";
    case SensorError::I2C_Communication_Failed: return "I2C communication failed";
    case SensorError::Sensor_Not_Enabled: return "No sensors enabled";
    case SensorError::Calibration_Failed: return "Calibration failed";
    case SensorError::Invalid_Reading: return "Invalid sensor reading";
    default: return "Unknown error";
  }
}