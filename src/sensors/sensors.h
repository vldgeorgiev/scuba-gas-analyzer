#ifndef SENSORS_H
#define SENSORS_H

#include <Adafruit_ADS1X15.h>
#include "O2Sensor.h"
#include "COSensor.h"
#include "HESensor.h"
#include "TempSensor.h"

// Lightweight error handling for ESP32
enum class SensorError : uint8_t {
  None = 0,
  ADC_Init_Failed = 1,
  I2C_Communication_Failed = 2,
  Sensor_Not_Enabled = 3,
  Calibration_Failed = 4,
  Invalid_Reading = 5,
  ADC_Timeout = 6
};

enum class ChannelState : uint8_t {
  Unavailable,
  Disabled,
  Valid,
  Invalid,
  Warming,
  Stale
};

inline const char* channelStateText(ChannelState state) {
  switch (state) {
    case ChannelState::Disabled: return "Off";
    case ChannelState::Valid: return "";
    case ChannelState::Invalid: return "Invalid";
    case ChannelState::Stale: return "Stale";
    case ChannelState::Warming: return "Warming";
    case ChannelState::Unavailable: return "Unavailable";
  }
  return "Unavailable";
}

struct sensorsData {
  static constexpr uint32_t FRESHNESS_MS = 1800;

  O2Reading O2Level;
  COReading CoLevel;
  HEReading HeLevel;
  float HeTemperature = NAN;
  SensorError lastError = SensorError::None;
  uint32_t timestampMs = 0;
  uint32_t generation = 1;
  ChannelState o2State = ChannelState::Unavailable;
  ChannelState coState = ChannelState::Unavailable;
  ChannelState heState = ChannelState::Unavailable;
  ChannelState temperatureState = ChannelState::Unavailable;

  bool isFresh(uint32_t nowMs) const {
    return static_cast<uint32_t>(nowMs - timestampMs) < FRESHNESS_MS;
  }

  sensorsData forDisplay(uint32_t nowMs, uint32_t expectedGeneration = 0) const {
    if (expectedGeneration && generation != expectedGeneration) {
      sensorsData unavailable;
      unavailable.generation = generation;
      return unavailable;
    }
    if (isFresh(nowMs)) return *this;
    sensorsData unavailable;
    unavailable.timestampMs = timestampMs;
    unavailable.generation = generation;
    unavailable.lastError = lastError;
    unavailable.o2State = o2State == ChannelState::Disabled ? ChannelState::Disabled : ChannelState::Stale;
    unavailable.coState = coState == ChannelState::Disabled ? ChannelState::Disabled : ChannelState::Stale;
    unavailable.heState = heState == ChannelState::Disabled ? ChannelState::Disabled : ChannelState::Stale;
    unavailable.temperatureState = temperatureState == ChannelState::Disabled ? ChannelState::Disabled : ChannelState::Stale;
    return unavailable;
  }
};

class SensorManager {
public:
    SensorManager(QueueHandle_t& dataQueue);

    SensorError init();
    void setSensorsConfig(bool isO2Enabled, bool isCOEnabled, bool isHeEnabled, float o2Calibration21, float o2Calibration100, float heCalibration100);
    SensorError readSensors(bool coWarmupWindow = false);
    void discardMeasurements() { xQueueReset(_dataQueue); }
    void setGeneration(uint32_t generation) { _generation = generation; }
    float readO2CalibrationSample();
    float readHeCalibrationSample();

    // Get last error for diagnostics
    SensorError getLastError() const { return _lastError; }
    static const char* getErrorString(SensorError error);

private:
    struct DeviceState {
      bool ready = false;
      uint32_t lastAttemptMs = 0;
    };
    static constexpr uint32_t ADC_RETRY_MS = 1000;
    bool initializeDevice(Adafruit_ADS1115& adc, DeviceState& state, uint8_t address, adsGain_t gain);
    void recoverDevice(Adafruit_ADS1115& adc, DeviceState& state, uint8_t address, adsGain_t gain);
    ChannelState recordRead(DeviceState& state, bool timedOut, bool valid);

    #define ADC2_ADDRESS 0x48 // Addr GND
    #define ADC1_ADDRESS 0x49 // Addr +3.3V
    #define ADC2_CHANNEL_CO 3 // The ADS1115 channel for CO sensor
    #define ADC2_CHANNEL_TEMP 2 // The ADS1115 channel for Temp sensor
    #define ADC1_GAIN GAIN_FOUR // 4x gain   +/- 1.024V  1 bit = 0.03125mV. For O2 (~10-45mv)
    #define ADC2_GAIN GAIN_TWO // 2x gain   +/- 2.048V  1 bit = 0.0625mV. For CO (400-2000mv), He (50-600mv) and Temp (10-100mv)
    Adafruit_ADS1115 _adc1;
    Adafruit_ADS1115 _adc2;
    DeviceState _adc1State;
    DeviceState _adc2State;
    QueueHandle_t& _dataQueue;
    O2Sensor _o2Sensor;
    COSensor _coSensor;
    HESensor _heSensor;
    TempSensor _tempSensor;

    bool _isO2Enabled = false;
    bool _isCOEnabled = false;
    bool _isHeEnabled = false;
    float _o2Calibration21 = NAN;
    float _o2Calibration100 = NAN;
    float _heCalibration100 = NAN;

    // Lightweight error tracking
    SensorError _lastError = SensorError::None;
    uint32_t _generation = 1;
};

#endif // SENSORS_H
