#include "Analyzer.h"
#include "pin_config.h"

namespace app {

void Analyzer::apply() {
  digitalWrite(PIN_HE_ENABLE, _effective.heEnabled && !preparedForSleep() ? HIGH : LOW);
  const bool coPower = _effective.coEnabled && !preparedForSleep();
  if (coPower != _coPowered) {
    digitalWrite(PIN_CO_ENABLE, coPower ? HIGH : LOW);
    _coPowered = coPower;
    if (_coPowered) _coPoweredAt = ::millis();
  }
  _sensors.setSensorsConfig(_effective.o2Enabled, _effective.coEnabled, _effective.heEnabled,
                           _oxygenRequired ? NAN : _effective.o2Air, _effective.o2Pure,
                           _heliumRequired ? NAN : _effective.heCalibration);
  _sensors.setGeneration(_generation);
}

Result Analyzer::begin(bool applicationWake) {
  pinMode(PIN_HE_ENABLE, OUTPUT);
  pinMode(PIN_CO_ENABLE, OUTPUT);
  digitalWrite(PIN_HE_ENABLE, LOW);
  digitalWrite(PIN_CO_ENABLE, LOW);
  _settingsStore.begin();
  _effective = _settingsStore.load();
  Result result;
  if (_settingsStore.loadedDefaults()) result.failure = Failure::LoadedDefaults;
  if (!_effective.valid()) {
    _effective = AnalyzerSettings{};
    result.failure = Failure::LoadedDefaults;
  }
  _oxygenRequired = !_settingsStore.hasOxygenCalibration();
  _heliumRequired = !_settingsStore.hasHeliumCalibration();
  const SensorError initialization = _sensors.init();
  apply();
  if (!applicationWake && result.failure == Failure::None && _effective.calibrateOnStart) {
    Command startup;
    startup.type = CommandType::CalibrateAir;
    result = execute(startup);
  }
  if (!_settingsStore.ready()) result.failure = Failure::Storage;
  result.sensorError = initialization;
  result.type = CommandType::Startup;
  result.effective = _effective;
  result.generation = _generation;
  result.oxygenCalibrationRequired = _oxygenRequired;
  result.heliumCalibrationRequired = _heliumRequired;
  if (result.failure == Failure::None && result.calibrationRequiredMessage()) result.failure = Failure::CalibrationRequired;
  _pending = result;
  _hasPending = true;
  return result;
}

bool Analyzer::service(QueueHandle_t commands, QueueHandle_t results) {
  if (_hasPending) {
    if (xQueueSend(results, &_pending, 0) != pdPASS) return false;
    _hasPending = false;
  }
  Command command;
  if (xQueueReceive(commands, &command, 0) != pdPASS) return false;
  _pending = execute(command);
  _hasPending = true;
  return true;
}

Result Analyzer::execute(const Command& command) {
  Result result;
  result.type = command.type;
  result.id = command.id;
  result.effective = _effective;
  result.generation = _generation;
  result.oxygenCalibrationRequired = _oxygenRequired;
  result.heliumCalibrationRequired = _heliumRequired;
  if (command.type == CommandType::PrepareSleep || command.type == CommandType::Resume) {
    if (command.id == 0) result.failure = Failure::Invalid;
    else if (command.type == CommandType::PrepareSleep) {
      if (_sleepId != 0 && _sleepId != command.id) result.failure = Failure::Busy;
      else if (_sleepId == 0) {
        _sleepId = command.id;
        advanceGeneration();
        apply();
        _sensors.discardMeasurements();
      }
    } else if (_sleepId == command.id) {
      _sleepId = 0;
      advanceGeneration();
      apply();
    } else if (_sleepId != 0) {
      result.failure = Failure::Invalid;
    }
    result.generation = _generation;
    return result;
  }
  if (preparedForSleep()) {
    result.failure = Failure::Busy;
    return result;
  }
  AnalyzerSettings candidate = _effective;
  switch (command.type) {
    case CommandType::ApplySettings:
      candidate = command.settings;
      candidate.o2Air = _effective.o2Air;
      candidate.o2Pure = _effective.o2Pure;
      candidate.heCalibration = _effective.heCalibration;
      break;
    case CommandType::CalibrateAir:
      result.calibration = _sensors.calibrateO2_21();
      candidate.o2Air = result.calibration;
      break;
    case CommandType::CalibratePure:
      if (_oxygenRequired) {
        result.failure = Failure::CalibrationRequired;
        break;
      }
      result.calibration = _sensors.calibrateO2_100();
      candidate.o2Pure = result.calibration;
      break;
    case CommandType::CalibrateHe:
      result.calibration = _sensors.calibrateHe_100();
      candidate.heCalibration = result.calibration;
      break;
    case CommandType::ResetAir:
      candidate.o2Air = AnalyzerSettings{}.o2Air;
      if (candidate.o2Pure <= candidate.o2Air) candidate.o2Pure = NAN;
      break;
    case CommandType::ClearPure: candidate.o2Pure = NAN; break;
    case CommandType::ResetHe: candidate.heCalibration = AnalyzerSettings{}.heCalibration; break;
    default: result.failure = Failure::Invalid; break;
  }
  const bool calibration = command.type == CommandType::CalibrateAir ||
      command.type == CommandType::CalibratePure || command.type == CommandType::CalibrateHe;
  if (calibration && result.failure == Failure::None && !std::isfinite(result.calibration)) result.failure = Failure::Sampling;
  if (result.failure == Failure::None && !candidate.valid()) result.failure = Failure::Invalid;
  const bool acceptOxygen = command.type == CommandType::CalibrateAir || command.type == CommandType::ResetAir;
  const bool acceptHelium = command.type == CommandType::CalibrateHe || command.type == CommandType::ResetHe;
  if (result.failure == Failure::None && !_settingsStore.save(candidate, _effective, acceptOxygen, acceptHelium)) {
    result.failure = Failure::Storage;
  }
  if (result.failure == Failure::None) {
    const bool calibrationRestored = (acceptOxygen && _oxygenRequired) || (acceptHelium && _heliumRequired);
    if (acceptOxygen) _oxygenRequired = false;
    if (acceptHelium) _heliumRequired = false;
    if (calibrationRestored || !candidate.sameMeasurementSettings(_effective)) {
      advanceGeneration();
    }
    _effective = candidate;
  }
  apply();
  result.effective = _effective;
  result.generation = _generation;
  result.oxygenCalibrationRequired = _oxygenRequired;
  result.heliumCalibrationRequired = _heliumRequired;
  return result;
}

}