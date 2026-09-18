#ifndef APP_ANALYZER_H
#define APP_ANALYZER_H

#include "app/AnalyzerPolicy.h"
#include "settings/SettingsStore.h"
#include "sensors/sensors.h"
#include <type_traits>

namespace app {

enum class CommandType : uint8_t {
  Startup, ApplySettings, CalibrateAir, CalibratePure, CalibrateHe, ResetAir, ClearPure, ResetHe,
  PrepareSleep, Resume, CancelCalibration
};
enum class Failure : uint8_t { None, Invalid, Storage, Sampling, LoadedDefaults, Busy, CalibrationRequired, Cancelled };
enum class CalibrationPhase : uint8_t { None, Settling, Stable, Saved, Failed, Cancelled };
enum class SleepPhase : uint8_t { Awake, Preparing, Prepared, Resuming };

struct Command {
  CommandType type = CommandType::ApplySettings;
  uint32_t id = 0;
  AnalyzerSettings settings;
};

struct Result {
  CommandType type = CommandType::Startup;
  uint32_t id = 0;
  Failure failure = Failure::None;
  AnalyzerSettings effective;
  uint32_t generation = 1;
  float calibration = NAN;
  float calibrationMillivolts = NAN;
  uint32_t calibrationElapsedMs = 0;
  CalibrationPhase calibrationPhase = CalibrationPhase::None;
  bool complete = true;
  SensorError sensorError = SensorError::None;
  bool oxygenCalibrationRequired = false;
  bool heliumCalibrationRequired = false;

  const char* calibrationRequiredMessage() const {
    const bool oxygen = oxygenCalibrationRequired && (effective.o2Enabled || effective.heEnabled);
    const bool helium = heliumCalibrationRequired && effective.heEnabled;
    if (oxygen && helium) return "O2 and He calibration required";
    if (oxygen) return "O2 calibration required";
    if (helium) return "He calibration required";
    return nullptr;
  }
};

static_assert(std::is_trivially_copyable<Command>::value, "Commands must be queue-copyable");
static_assert(std::is_trivially_copyable<Result>::value, "Results must be queue-copyable");

struct UiState {
  static constexpr uint32_t PREPARE_TIMEOUT_MS = 2000;
  static constexpr uint32_t RESUME_RETRY_MS = 1000;
  AnalyzerSettings effective;
  uint32_t generation = 1;
  uint32_t pendingId = 0;
  bool settingsEditing = false;
  bool ready = false;
  uint32_t nextId = 1;
  SleepPhase sleepPhase = SleepPhase::Awake;
  uint32_t prepareStarted = 0;
  bool resumeQueued = false;
  bool resumeRetry = false;
  uint32_t resumeFailedAt = 0;

  bool busy() const { return !ready || pendingId != 0; }
  bool shouldSyncSettings(const Result& result) const {
    return !settingsEditing || result.type == CommandType::Startup;
  }
  bool closeSettings(const AnalyzerSettings& draft, QueueHandle_t queue) {
    if (!settingsEditing) return false;
    settingsEditing = false;
    Command command;
    command.settings = draft;
    return submit(command, queue);
  }
  bool submit(Command command, QueueHandle_t queue) {
    if (busy() || command.type == CommandType::Startup || command.type == CommandType::Resume) return false;
    command.id = nextId;
    if (xQueueSend(queue, &command, 0) != pdPASS) return false;
    pendingId = command.id;
    if (command.type == CommandType::PrepareSleep) {
      sleepPhase = SleepPhase::Preparing;
      prepareStarted = ::millis();
      resumeQueued = false;
      resumeRetry = false;
    }
    if (++nextId == 0) nextId = 1;
    return true;
  }
  bool cancelCalibration(QueueHandle_t queue) const {
    if (!ready || pendingId == 0) return false;
    Command command;
    command.type = CommandType::CancelCalibration;
    command.id = pendingId;
    return xQueueSend(queue, &command, 0) == pdPASS;
  }
  bool preparationExpired(uint32_t now) const {
    return sleepPhase == SleepPhase::Preparing &&
           static_cast<uint32_t>(now - prepareStarted) >= PREPARE_TIMEOUT_MS;
  }
  bool requestResume(QueueHandle_t queue) {
    if (sleepPhase == SleepPhase::Awake) return false;
    sleepPhase = SleepPhase::Resuming;
    if (resumeQueued) return true;
    if (resumeRetry && static_cast<uint32_t>(::millis() - resumeFailedAt) < RESUME_RETRY_MS) return false;
    Command resume;
    resume.type = CommandType::Resume;
    resume.id = pendingId;
    if (xQueueSend(queue, &resume, 0) != pdPASS) return false;
    resumeQueued = true;
    resumeRetry = false;
    return true;
  }
  bool accept(const Result& result) {
    if (result.type == CommandType::Startup) {
      if (ready) return false;
      ready = true;
    } else {
      if (!ready || pendingId == 0 || result.id != pendingId) return false;
      if (sleepPhase != SleepPhase::Awake) {
        if (result.type == CommandType::PrepareSleep && sleepPhase == SleepPhase::Preparing) {
          if (result.failure == Failure::None) sleepPhase = SleepPhase::Prepared;
          else {
            sleepPhase = SleepPhase::Awake;
            pendingId = 0;
          }
        } else if (result.type == CommandType::Resume && sleepPhase == SleepPhase::Resuming && resumeQueued) {
          if (result.failure == Failure::None) {
            sleepPhase = SleepPhase::Awake;
            pendingId = 0;
            resumeQueued = false;
            resumeRetry = false;
          } else {
            resumeQueued = false;
            resumeRetry = true;
            resumeFailedAt = ::millis();
          }
        } else return false;
      } else {
        if (result.type == CommandType::PrepareSleep || result.type == CommandType::Resume) return false;
        if (result.complete) pendingId = 0;
      }
    }
    effective = result.effective;
    generation = result.generation;
    return true;
  }
};

class Analyzer {
public:
  Analyzer(SettingsStore& settingsStore, SensorManager& sensors) : _settingsStore(settingsStore), _sensors(sensors) {}
  Result begin(bool applicationWake = false);
  Result execute(const Command& command);
  bool service(QueueHandle_t commands, QueueHandle_t results);
  SensorError measure() { return preparedForSleep() ? SensorError::None : _sensors.readSensors(coWarming()); }
  bool calibrating() const { return _calibrationActive; }
  bool preparedForSleep() const { return _sleepId != 0; }
  bool coWarming() const {
    return _effective.coEnabled && _coPowered &&
        static_cast<uint32_t>(::millis() - _coPoweredAt) < policy::CO_WARMUP_MS;
  }
  const AnalyzerSettings& effective() const { return _effective; }

private:
  void apply();
  void advanceGeneration() { if (++_generation == 0) _generation = 1; }
  bool calibrationCommand(CommandType type) const;
  void initializeCalibration(const Command& command);
  void startCalibration(const Command& command, QueueHandle_t results);
  bool advanceCalibration(QueueHandle_t results);
  Result finishCalibration(Failure failure, CalibrationPhase phase, float candidate = NAN);
  void publishCalibration(QueueHandle_t results, CalibrationPhase phase, float sample, uint32_t elapsed);
  SettingsStore& _settingsStore;
  SensorManager& _sensors;
  AnalyzerSettings _effective;
  uint32_t _generation = 1;
  Result _pending;
  bool _hasPending = false;
  bool _coPowered = false;
  uint32_t _coPoweredAt = 0;
  uint32_t _sleepId = 0;
  bool _oxygenRequired = false;
  bool _heliumRequired = false;
  bool _calibrationActive = false;
  bool _startupCalibration = false;
  SensorError _startupSensorError = SensorError::None;
  Command _calibrationCommand;
  uint32_t _calibrationStartedMs = 0;
  uint32_t _calibrationLastSampleMs = 0;
  float _calibrationSamples[policy::CALIBRATION_WINDOW_SAMPLES] = {};
  uint8_t _calibrationSampleCount = 0;
  uint8_t _calibrationSampleIndex = 0;
};

}

#endif