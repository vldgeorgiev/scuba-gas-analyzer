#include "Analyzer.h"
#include "pin_config.h"

namespace app {

void Analyzer::apply() {
  digitalWrite(PIN_HE_ENABLE, _effective.heEnabled ? HIGH : LOW);
  digitalWrite(PIN_CO_ENABLE, _effective.coEnabled ? HIGH : LOW);
  _sensors.setSensorsConfig(_effective.o2Enabled, _effective.coEnabled, _effective.heEnabled,
                           _effective.o2Air, _effective.o2Pure, _effective.heCalibration);
  _sensors.setGeneration(_generation);
}

Result Analyzer::begin() {
  pinMode(PIN_HE_ENABLE, OUTPUT);
  pinMode(PIN_CO_ENABLE, OUTPUT);
  digitalWrite(PIN_HE_ENABLE, LOW);
  digitalWrite(PIN_CO_ENABLE, LOW);
  _config.begin();
  _effective = _config.load();
  Result result;
  if (!_effective.valid()) {
    _effective = AnalyzerSettings{};
    result.failure = Failure::LoadedDefaults;
  }
  const SensorError initialization = _sensors.init();
  apply();
  if (result.failure == Failure::None && _effective.calibrateOnStart) {
    Command startup;
    startup.type = CommandType::CalibrateAir;
    result = execute(startup);
  }
  if (!_config.ready() && result.failure == Failure::None) result.failure = Failure::Storage;
  result.sensorError = initialization;
  result.type = CommandType::Startup;
  result.effective = _effective;
  result.generation = _generation;
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
  if (calibration && !std::isfinite(result.calibration)) result.failure = Failure::Sampling;
  if (result.failure == Failure::None && !candidate.valid()) result.failure = Failure::Invalid;
  if (result.failure == Failure::None && !_config.save(candidate, _effective)) result.failure = Failure::Storage;
  if (result.failure == Failure::None) {
    if (!candidate.sameMeasurementSettings(_effective)) {
      if (++_generation == 0) _generation = 1;
    }
    _effective = candidate;
  }
  apply();
  result.effective = _effective;
  result.generation = _generation;
  return result;
}

}