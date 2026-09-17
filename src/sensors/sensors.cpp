#include "sensors.h"
#include "O2Sensor.h"
#include "COSensor.h"
#include "HESensor.h"
#include "TempSensor.h"
#include "pin_config.h"
#include <cmath>

SensorManager::SensorManager(QueueHandle_t& dataQueue) :
  _adc1(),
  _adc2(),
  _dataQueue(dataQueue),
  _o2Sensor(_adc1),
  _coSensor(ADC2_CHANNEL_CO, _adc2),
  _heSensor(_adc2), // He sensor is on the same ADC as CO and Temp, but uses different channels
  _tempSensor(ADC2_CHANNEL_TEMP, _adc2)
{}

SensorError SensorManager::init() {
  _lastError = SensorError::None;

  Wire1.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire1.setTimeOut(10);
  const bool adc1Ready = initializeDevice(_adc1, _adc1State, ADC1_ADDRESS, ADC1_GAIN);
  const bool adc2Ready = initializeDevice(_adc2, _adc2State, ADC2_ADDRESS, ADC2_GAIN);
  if (!adc1Ready || !adc2Ready) {
    _lastError = SensorError::ADC_Init_Failed;
  }
  return _lastError;
}

bool SensorManager::initializeDevice(Adafruit_ADS1115& adc, DeviceState& state,
                                     uint8_t address, adsGain_t gain) {
  state.ready = adc.begin(address, &Wire1);
  state.lastAttemptMs = ::millis();
  if (state.ready) {
    adc.setGain(gain);
    adc.setDataRate(RATE_ADS1115_128SPS);
    log_i("ADC 0x%02x ready", address);
  } else {
    log_w("ADC 0x%02x unavailable", address);
  }
  return state.ready;
}

void SensorManager::recoverDevice(Adafruit_ADS1115& adc, DeviceState& state,
                                  uint8_t address, adsGain_t gain) {
  if (!state.ready && static_cast<uint32_t>(::millis() - state.lastAttemptMs) >= ADC_RETRY_MS) {
    initializeDevice(adc, state, address, gain);
  }
}

ChannelState SensorManager::recordRead(DeviceState& state, bool timedOut, bool valid) {
  if (timedOut) {
    state.ready = false;
    state.lastAttemptMs = ::millis();
    _lastError = SensorError::ADC_Timeout;
    return ChannelState::Unavailable;
  } else if (!valid && _lastError == SensorError::None) {
    _lastError = SensorError::Invalid_Reading;
  }
  return valid ? ChannelState::Valid : ChannelState::Invalid;
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

SensorError SensorManager::readSensors(bool coWarmupWindow) {
  sensorsData data;
  data.timestampMs = ::millis();
  data.generation = _generation;
  data.o2State = _isO2Enabled ? ChannelState::Unavailable : ChannelState::Disabled;
  data.coState = _isCOEnabled ? ChannelState::Unavailable : ChannelState::Disabled;
  data.heState = _isHeEnabled ? ChannelState::Unavailable : ChannelState::Disabled;
  data.temperatureState = _isHeEnabled ? ChannelState::Unavailable : ChannelState::Disabled;
  _lastError = SensorError::None;

  if (!_isO2Enabled && !_isCOEnabled && !_isHeEnabled) {
    xQueueOverwrite(_dataQueue, &data);
    return _lastError;
  }

  const bool coActive = _isCOEnabled;
  if (_isO2Enabled) recoverDevice(_adc1, _adc1State, ADC1_ADDRESS, ADC1_GAIN);
  if (coActive || _isHeEnabled) recoverDevice(_adc2, _adc2State, ADC2_ADDRESS, ADC2_GAIN);
  if ((_isO2Enabled && !_adc1State.ready) ||
      ((coActive || _isHeEnabled) && !_adc2State.ready)) {
    _lastError = SensorError::ADC_Init_Failed;
  }

  bool timedOut = false;
  if (_isO2Enabled && _adc1State.ready) {
    data.O2Level = _o2Sensor.readLevel(&timedOut);
    data.o2State = recordRead(_adc1State, timedOut, std::isfinite(data.O2Level.percentage));
  }

  if (coActive && _adc2State.ready) {
    data.CoLevel = _coSensor.readLevel(&timedOut);
    data.coState = recordRead(_adc2State, timedOut, std::isfinite(data.CoLevel.ppm));
    if (data.coState == ChannelState::Valid && coWarmupWindow &&
        data.CoLevel.ppm > CO_WARMUP_PPM) data.coState = ChannelState::Warming;
  }

  if (_isHeEnabled && _adc2State.ready) {
    data.HeLevel = _heSensor.readLevel(data.O2Level.percentage, &timedOut);
    data.heState = recordRead(_adc2State, timedOut, std::isfinite(data.HeLevel.percentage));
  }

  if (_isHeEnabled && _adc2State.ready) {
    data.HeTemperature = _tempSensor.readLevel(&timedOut);
    data.temperatureState = recordRead(_adc2State, timedOut, std::isfinite(data.HeTemperature));
    if (data.heState == ChannelState::Valid && data.temperatureState == ChannelState::Valid &&
        data.HeTemperature < HE_WARMUP_TEMPERATURE_C) data.heState = ChannelState::Warming;
  }
  data.lastError = _lastError;

  xQueueOverwrite(_dataQueue, &data);
  return _lastError;
}

float SensorManager::calibrateO2_21() {
  log_d("Calibrating O2 sensor for 21%% O2...");
  recoverDevice(_adc1, _adc1State, ADC1_ADDRESS, ADC1_GAIN);
  if (!_adc1State.ready) return NAN;
  const float candidate = _o2Sensor.calibrate();
  if (!std::isfinite(candidate)) {
    recordRead(_adc1State, true, false);
    return NAN;
  }
  _o2Calibration21 = candidate;
  _o2Sensor.setCalibrations(_o2Calibration21, _o2Calibration100);
  return _o2Calibration21;
}

float SensorManager::calibrateO2_100() {
  log_d("Calibrating O2 sensor for 100%% O2...");
  recoverDevice(_adc1, _adc1State, ADC1_ADDRESS, ADC1_GAIN);
  if (!_adc1State.ready) return NAN;
  const float candidate = _o2Sensor.calibrate();
  if (!std::isfinite(candidate)) {
    recordRead(_adc1State, true, false);
    return NAN;
  }
  _o2Calibration100 = candidate;
  _o2Sensor.setCalibrations(_o2Calibration21, _o2Calibration100);
  return _o2Calibration100;
}

float SensorManager::calibrateHe_100() {
  log_d("Calibrating He sensor for 100%% He...");
  recoverDevice(_adc2, _adc2State, ADC2_ADDRESS, ADC2_GAIN);
  if (!_adc2State.ready) return NAN;
  const float candidate = _heSensor.calibrate();
  if (!std::isfinite(candidate)) {
    recordRead(_adc2State, true, false);
    return NAN;
  }
  _heCalibration100 = candidate;
  _heSensor.setCalibrations(_heCalibration100);
  return _heCalibration100;
}

float SensorManager::readO2CalibrationSample() {
  recoverDevice(_adc1, _adc1State, ADC1_ADDRESS, ADC1_GAIN);
  if (!_adc1State.ready) return NAN;
  bool timedOut = false;
  const float sample = _o2Sensor.readLevel(&timedOut).millivolts;
  recordRead(_adc1State, timedOut, std::isfinite(sample));
  return sample;
}

float SensorManager::readHeCalibrationSample() {
  recoverDevice(_adc2, _adc2State, ADC2_ADDRESS, ADC2_GAIN);
  if (!_adc2State.ready) return NAN;
  bool timedOut = false;
  const float sample = _heSensor.readLevel(0, &timedOut).millivolts;
  recordRead(_adc2State, timedOut, std::isfinite(sample));
  return sample;
}

const char* SensorManager::getErrorString(SensorError error) {
  switch (error) {
    case SensorError::None: return "No error";
    case SensorError::ADC_Init_Failed: return "ADC unavailable";
    case SensorError::ADC_Timeout: return "ADC conversion timed out";
    case SensorError::I2C_Communication_Failed: return "I2C communication failed";
    case SensorError::Sensor_Not_Enabled: return "No sensors enabled";
    case SensorError::Calibration_Failed: return "Calibration failed";
    case SensorError::Invalid_Reading: return "Invalid sensor reading";
    default: return "Unknown error";
  }
}