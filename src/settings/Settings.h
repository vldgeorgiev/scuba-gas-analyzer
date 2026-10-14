#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include "app/AnalyzerPolicy.h"
#include <cmath>
#include <cstdint>

namespace app {
constexpr uint8_t DEFAULT_SLEEP_MINUTES = 5;
constexpr uint8_t SLEEP_OPTIONS[] = {0, 1, 2, 5, 10, 30};
constexpr int8_t CO_CORRECTION_MIN_PPM = -5;
constexpr int8_t CO_CORRECTION_MAX_PPM = 2;
}

struct AnalyzerSettings {
  bool o2Enabled = true;
  bool coEnabled = true;
  bool heEnabled = true;
  bool calibrateOnStart = true;
  uint8_t brightness = 128;
  uint8_t sleepMinutes = app::DEFAULT_SLEEP_MINUTES;
  int8_t coCorrectionPpm = 0;
  float po2Bottom = 1.4f;
  float po2Deco = 1.6f;
  float o2Air = app::policy::O2_AIR_CALIBRATION_DEFAULT_MV;
  float o2Pure = NAN;
  float externalO2Air = app::policy::O2_AIR_CALIBRATION_DEFAULT_MV;
  float externalO2Pure = NAN;
  float heCalibration = app::policy::HE_CALIBRATION_DEFAULT_MV;

  static bool validSleepMinutes(uint8_t minutes) {
    for (uint8_t option : app::SLEEP_OPTIONS) {
      if (minutes == option) return true;
    }
    return false;
  }

  static bool validCoCorrection(int value) {
    return value >= app::CO_CORRECTION_MIN_PPM && value <= app::CO_CORRECTION_MAX_PPM;
  }

  static bool validPo2Bottom(float value) {
    return std::isfinite(value) && value >= 1 && value <= 1.6f;
  }

  static bool validPo2Deco(float value, float bottom) {
    return std::isfinite(value) && value >= bottom && value <= 2;
  }

  static bool validO2Air(float value) {
    return std::isfinite(value) && value >= app::policy::O2_AIR_CALIBRATION_MIN_MV &&
           value <= app::policy::O2_AIR_CALIBRATION_MAX_MV;
  }

  static bool validO2Pure(float value, float air) {
    return std::isnan(value) ||
           (std::isfinite(value) && value >= app::policy::O2_PURE_CALIBRATION_MIN_MV &&
            value <= app::policy::O2_PURE_CALIBRATION_MAX_MV && value > air);
  }

  static bool validHeCalibration(float value) {
    return std::isfinite(value) && value >= app::policy::HE_CALIBRATION_MIN_MV;
  }

  bool valid() const {
    return validSleepMinutes(sleepMinutes) && brightness >= 8 && validCoCorrection(coCorrectionPpm) &&
           validPo2Bottom(po2Bottom) &&
           validPo2Deco(po2Deco, po2Bottom) && validO2Air(o2Air) && validO2Pure(o2Pure, o2Air) &&
           validO2Air(externalO2Air) && validO2Pure(externalO2Pure, externalO2Air) &&
           validHeCalibration(heCalibration);
  }

  bool repairInvalidFields() {
    const bool repaired = !valid();
    const AnalyzerSettings defaults;
    if (!validSleepMinutes(sleepMinutes)) sleepMinutes = defaults.sleepMinutes;
    if (brightness < 8) brightness = defaults.brightness;
    if (!validCoCorrection(coCorrectionPpm)) coCorrectionPpm = defaults.coCorrectionPpm;
    if (!validPo2Bottom(po2Bottom)) {
      po2Bottom = validPo2Deco(po2Deco, 1) && po2Deco < defaults.po2Bottom ? po2Deco : defaults.po2Bottom;
    }
    if (!validPo2Deco(po2Deco, po2Bottom)) {
      po2Deco = validPo2Deco(po2Deco, 1) ? po2Bottom : defaults.po2Deco;
    }
    if (!validO2Air(o2Air)) {
      o2Air = defaults.o2Air;
      o2Pure = defaults.o2Pure;
    }
    if (!validO2Pure(o2Pure, o2Air)) o2Pure = defaults.o2Pure;
    if (!validO2Air(externalO2Air)) {
      externalO2Air = defaults.externalO2Air;
      externalO2Pure = NAN;
    }
    if (!validO2Pure(externalO2Pure, externalO2Air)) externalO2Pure = NAN;
    if (!validHeCalibration(heCalibration)) heCalibration = defaults.heCalibration;
    return repaired;
  }

  bool sameMeasurementSettings(const AnalyzerSettings& other) const {
    return o2Enabled == other.o2Enabled && coEnabled == other.coEnabled && heEnabled == other.heEnabled &&
           coCorrectionPpm == other.coCorrectionPpm && o2Air == other.o2Air && heCalibration == other.heCalibration &&
           externalO2Air == other.externalO2Air &&
           (externalO2Pure == other.externalO2Pure || (std::isnan(externalO2Pure) && std::isnan(other.externalO2Pure))) &&
           (o2Pure == other.o2Pure || (std::isnan(o2Pure) && std::isnan(other.o2Pure)));
  }
};

#endif