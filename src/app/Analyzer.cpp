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
    initializeCalibration(startup);
    _startupCalibration = true;
    _startupSensorError = initialization;
    result.complete = false;
    result.calibrationPhase = CalibrationPhase::Settling;
  }
  if (!_settingsStore.ready()) result.failure = Failure::Storage;
  result.sensorError = initialization;
  result.type = CommandType::Startup;
  result.effective = _effective;
  result.generation = _generation;
  result.oxygenCalibrationRequired = _oxygenRequired;
  result.heliumCalibrationRequired = _heliumRequired;
  if (result.failure == Failure::None && result.calibrationRequiredMessage()) result.failure = Failure::CalibrationRequired;
  if (!_startupCalibration) {
    _pending = result;
    _hasPending = true;
  }
  return result;
}

bool Analyzer::service(QueueHandle_t commands, QueueHandle_t results) {
  bool worked = false;
  if (_hasPending) {
    if (xQueueSend(results, &_pending, 0) != pdPASS) return false;
    _hasPending = false;
    worked = true;
  }
  Command command;
  if (xQueueReceive(commands, &command, 0) == pdPASS) {
    worked = true;
    if (_calibrationActive) {
      if (command.type == CommandType::CancelCalibration && command.id == _calibrationCommand.id) {
        _pending = finishCalibration(Failure::Cancelled, CalibrationPhase::Cancelled);
        _hasPending = true;
      }
    } else if (calibrationCommand(command.type)) {
      startCalibration(command, results);
    } else {
      _pending = execute(command);
      _hasPending = true;
    }
  }
  if (_calibrationActive) worked = advanceCalibration(results) || worked;
  return worked;
}

bool Analyzer::calibrationCommand(CommandType type) const {
  return type == CommandType::CalibrateAir || type == CommandType::CalibratePure ||
         type == CommandType::CalibrateHe;
}

void Analyzer::startCalibration(const Command& command, QueueHandle_t results) {
  if (command.type == CommandType::CalibratePure && _oxygenRequired) {
    _pending = Result{};
    _pending.type = command.type;
    _pending.id = command.id;
    _pending.failure = Failure::CalibrationRequired;
    _pending.effective = _effective;
    _pending.generation = _generation;
    _pending.oxygenCalibrationRequired = _oxygenRequired;
    _pending.heliumCalibrationRequired = _heliumRequired;
    _pending.calibrationPhase = CalibrationPhase::Failed;
    _hasPending = true;
    return;
  }
  initializeCalibration(command);
  publishCalibration(results, CalibrationPhase::Settling, NAN, 0);
}

void Analyzer::initializeCalibration(const Command& command) {
  _calibrationActive = true;
  _calibrationCommand = command;
  _calibrationStartedMs = ::millis();
  _calibrationLastSampleMs = _calibrationStartedMs - policy::CALIBRATION_SAMPLE_MS;
  _calibrationSampleCount = 0;
  _calibrationSampleIndex = 0;
}

void Analyzer::publishCalibration(QueueHandle_t results, CalibrationPhase phase, float sample, uint32_t elapsed) {
  if (_startupCalibration) return;
  Result result;
  result.type = _calibrationCommand.type;
  result.id = _calibrationCommand.id;
  result.effective = _effective;
  result.generation = _generation;
  result.calibrationMillivolts = sample;
  result.calibrationElapsedMs = elapsed;
  result.calibrationPhase = phase;
  result.complete = false;
  result.oxygenCalibrationRequired = _oxygenRequired;
  result.heliumCalibrationRequired = _heliumRequired;
  xQueueOverwrite(results, &result);
}

bool Analyzer::advanceCalibration(QueueHandle_t results) {
  const uint32_t now = ::millis();
  const uint32_t elapsed = static_cast<uint32_t>(now - _calibrationStartedMs);
  if (static_cast<uint32_t>(now - _calibrationLastSampleMs) < policy::CALIBRATION_SAMPLE_MS) return false;
  _calibrationLastSampleMs = now;
  const float sample = _calibrationCommand.type == CommandType::CalibrateHe
      ? _sensors.readHeCalibrationSample() : _sensors.readO2CalibrationSample();
  if (std::isfinite(sample)) {
    _calibrationSamples[_calibrationSampleIndex] = sample;
    _calibrationSampleIndex = (_calibrationSampleIndex + 1) % policy::CALIBRATION_WINDOW_SAMPLES;
    if (_calibrationSampleCount < policy::CALIBRATION_WINDOW_SAMPLES) ++_calibrationSampleCount;
  } else {
    _calibrationSampleCount = 0;
    _calibrationSampleIndex = 0;
  }
  bool stable = _calibrationSampleCount == policy::CALIBRATION_WINDOW_SAMPLES;
  float minimum = INFINITY;
  float maximum = -INFINITY;
  float total = 0;
  float olderTotal = 0;
  float newerTotal = 0;
  constexpr uint8_t halfWindow = policy::CALIBRATION_WINDOW_SAMPLES / 2;
  for (uint8_t index = 0; stable && index < _calibrationSampleCount; ++index) {
    const float value = _calibrationSamples[(_calibrationSampleIndex + index) % policy::CALIBRATION_WINDOW_SAMPLES];
    minimum = std::min(minimum, value);
    maximum = std::max(maximum, value);
    total += value;
    if (index < halfWindow) olderTotal += value;
    else if (index >= policy::CALIBRATION_WINDOW_SAMPLES - halfWindow) newerTotal += value;
  }
  const float threshold = _calibrationCommand.type == CommandType::CalibrateHe
      ? policy::HE_CALIBRATION_STABILITY_MV : policy::O2_CALIBRATION_STABILITY_MV;
  const float driftThreshold = _calibrationCommand.type == CommandType::CalibrateHe
      ? policy::HE_CALIBRATION_DRIFT_MV_PER_SECOND : policy::O2_CALIBRATION_DRIFT_MV_PER_SECOND;
    const float halfCenterSeconds = (halfWindow + 1) * policy::CALIBRATION_SAMPLE_MS / 1000.0f;
  const float drift = std::fabs(newerTotal / halfWindow - olderTotal / halfWindow) / halfCenterSeconds;
  stable = stable && maximum - minimum <= threshold && drift <= driftThreshold;
  if (stable && elapsed >= policy::CALIBRATION_MINIMUM_MS) {
    _pending = finishCalibration(Failure::None, CalibrationPhase::Saved,
                                 total / _calibrationSampleCount);
    _hasPending = true;
    return true;
  }
  if (elapsed >= policy::CALIBRATION_TIMEOUT_MS) {
    _pending = finishCalibration(Failure::Sampling, CalibrationPhase::Failed);
    _hasPending = true;
    return true;
  }
  publishCalibration(results, stable ? CalibrationPhase::Stable : CalibrationPhase::Settling, sample, elapsed);
  return true;
}

Result Analyzer::finishCalibration(Failure failure, CalibrationPhase phase, float candidateValue) {
  Result result;
  result.type = _calibrationCommand.type;
  result.id = _calibrationCommand.id;
  result.calibration = candidateValue;
  result.calibrationMillivolts = _calibrationSampleCount
      ? _calibrationSamples[(_calibrationSampleIndex + policy::CALIBRATION_WINDOW_SAMPLES - 1) %
                policy::CALIBRATION_WINDOW_SAMPLES] : NAN;
  result.calibrationElapsedMs = static_cast<uint32_t>(::millis() - _calibrationStartedMs);
  result.calibrationPhase = phase;
  AnalyzerSettings candidate = _effective;
  const bool acceptOxygen = result.type == CommandType::CalibrateAir;
  const bool acceptHelium = result.type == CommandType::CalibrateHe;
  if (failure == Failure::None) {
    if (result.type == CommandType::CalibrateAir) candidate.o2Air = candidateValue;
    else if (result.type == CommandType::CalibratePure) candidate.o2Pure = candidateValue;
    else candidate.heCalibration = candidateValue;
    if (!candidate.valid()) failure = Failure::Invalid;
    else if (!_settingsStore.save(candidate, _effective, acceptOxygen, acceptHelium)) failure = Failure::Storage;
  }
  if (failure == Failure::None) {
    const bool calibrationRestored = (acceptOxygen && _oxygenRequired) || (acceptHelium && _heliumRequired);
    if (acceptOxygen) _oxygenRequired = false;
    if (acceptHelium) _heliumRequired = false;
    if (calibrationRestored || !candidate.sameMeasurementSettings(_effective)) advanceGeneration();
    _effective = candidate;
  } else if (phase == CalibrationPhase::Saved) {
    result.calibrationPhase = CalibrationPhase::Failed;
  }
  _calibrationActive = false;
  apply();
  result.failure = failure;
  result.effective = _effective;
  result.generation = _generation;
  result.oxygenCalibrationRequired = _oxygenRequired;
  result.heliumCalibrationRequired = _heliumRequired;
  if (_startupCalibration) {
    result.type = CommandType::Startup;
    result.id = 0;
    result.sensorError = _startupSensorError;
    if (result.failure == Failure::None && result.calibrationRequiredMessage()) {
      result.failure = Failure::CalibrationRequired;
    }
    _startupCalibration = false;
  }
  return result;
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
    case CommandType::ResetAir:
      candidate.o2Air = AnalyzerSettings{}.o2Air;
      if (candidate.o2Pure <= candidate.o2Air) candidate.o2Pure = NAN;
      break;
    case CommandType::ClearPure: candidate.o2Pure = NAN; break;
    case CommandType::ResetHe: candidate.heCalibration = AnalyzerSettings{}.heCalibration; break;
    default: result.failure = Failure::Invalid; break;
  }
  if (result.failure == Failure::None && !candidate.valid()) result.failure = Failure::Invalid;
  const bool acceptOxygen = command.type == CommandType::ResetAir;
  const bool acceptHelium = command.type == CommandType::ResetHe;
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