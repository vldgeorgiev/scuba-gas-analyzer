#ifndef APP_ANALYZER_H
#define APP_ANALYZER_H

#include "config.h"
#include "sensors/sensors.h"
#include <type_traits>

namespace app {

enum class CommandType : uint8_t {
  Startup, ApplySettings, CalibrateAir, CalibratePure, CalibrateHe, ResetAir, ClearPure, ResetHe
};
enum class Failure : uint8_t { None, Invalid, Storage, Sampling, LoadedDefaults };

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
  SensorError sensorError = SensorError::None;
};

static_assert(std::is_trivially_copyable<Command>::value, "Commands must be queue-copyable");
static_assert(std::is_trivially_copyable<Result>::value, "Results must be queue-copyable");

struct UiState {
  AnalyzerSettings effective;
  uint32_t generation = 1;
  uint32_t pendingId = 0;
  bool ready = false;
  uint32_t nextId = 1;

  bool busy() const { return !ready || pendingId != 0; }
  bool submit(Command command, QueueHandle_t queue) {
    if (busy()) return false;
    command.id = nextId;
    if (xQueueSend(queue, &command, 0) != pdPASS) return false;
    pendingId = command.id;
    if (++nextId == 0) nextId = 1;
    return true;
  }
  bool accept(const Result& result) {
    if (result.type == CommandType::Startup) {
      if (ready) return false;
      ready = true;
    } else {
      if (!ready || pendingId == 0 || result.id != pendingId) return false;
      pendingId = 0;
    }
    effective = result.effective;
    generation = result.generation;
    return true;
  }
};

class Analyzer {
public:
  Analyzer(Config& config, SensorManager& sensors) : _config(config), _sensors(sensors) {}
  Result begin();
  Result execute(const Command& command);
  bool service(QueueHandle_t commands, QueueHandle_t results);
  SensorError measure() { return _sensors.readSensors(); }
  const AnalyzerSettings& effective() const { return _effective; }

private:
  void apply();
  Config& _config;
  SensorManager& _sensors;
  AnalyzerSettings _effective;
  uint32_t _generation = 1;
  Result _pending;
  bool _hasPending = false;
};

}

#endif