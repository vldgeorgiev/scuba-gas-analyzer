#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <cmath>
#include <cstdint>

namespace app {
constexpr uint8_t DEFAULT_SLEEP_MINUTES = 5;
constexpr uint8_t SLEEP_OPTIONS[] = {0, 1, 2, 5, 10, 30};
}

struct AnalyzerSettings {
  bool o2Enabled = true;
  bool coEnabled = true;
  bool heEnabled = true;
  bool calibrateOnStart = true;
  uint8_t brightness = 128;
  uint8_t sleepMinutes = app::DEFAULT_SLEEP_MINUTES;
  float po2Bottom = 1.4f;
  float po2Deco = 1.6f;
  float o2Air = 10;
  float o2Pure = NAN;
  float heCalibration = 620;

  static bool validSleepMinutes(uint8_t minutes) {
    for (uint8_t option : app::SLEEP_OPTIONS) {
      if (minutes == option) return true;
    }
    return false;
  }

  bool valid() const {
    return validSleepMinutes(sleepMinutes) && brightness >= 8 && std::isfinite(po2Bottom) && po2Bottom >= 1 && po2Bottom <= 1.6f &&
           std::isfinite(po2Deco) && po2Deco >= po2Bottom && po2Deco <= 2 &&
           std::isfinite(o2Air) && o2Air >= 5 && o2Air <= 50 &&
           (std::isnan(o2Pure) || (std::isfinite(o2Pure) && o2Pure > o2Air && o2Pure <= 100)) &&
           std::isfinite(heCalibration) && heCalibration > 0;
  }

  bool sameMeasurementSettings(const AnalyzerSettings& other) const {
    return o2Enabled == other.o2Enabled && coEnabled == other.coEnabled && heEnabled == other.heEnabled &&
           o2Air == other.o2Air && heCalibration == other.heCalibration &&
           (o2Pure == other.o2Pure || (std::isnan(o2Pure) && std::isnan(other.o2Pure)));
  }
};

#endif