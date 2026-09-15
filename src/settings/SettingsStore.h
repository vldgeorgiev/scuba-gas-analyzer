#ifndef SETTINGS_STORE_H
#define SETTINGS_STORE_H

#include <Preferences.h>
#include <Arduino.h>
#include "Settings.h"

class SettingsStore {
public:
  ~SettingsStore() { _preferences.end(); }

  void begin() {
    _ready = _preferences.begin("config", false);
    if (!_ready) log_e("Failed to open preferences");
  }

  bool ready() const { return _ready; }

  AnalyzerSettings load() {
    AnalyzerSettings value;
    value.o2Enabled = _preferences.getBool("o2_enabled", value.o2Enabled);
    value.coEnabled = _preferences.getBool("co_enabled", value.coEnabled);
    value.heEnabled = _preferences.getBool("he_enabled", value.heEnabled);
    value.calibrateOnStart = _preferences.getBool("calib_start", value.calibrateOnStart);
    value.brightness = _preferences.getUChar("brightness", value.brightness);
    value.sleepMinutes = _preferences.getUChar("sleep_minutes", value.sleepMinutes);
    const bool invalidSleep = !AnalyzerSettings::validSleepMinutes(value.sleepMinutes);
    if (invalidSleep) {
      value.sleepMinutes = app::DEFAULT_SLEEP_MINUTES;
      log_w("Invalid sleep timeout; using five minutes");
    }
    value.po2Bottom = _preferences.getFloat("po2_max_bottom", value.po2Bottom);
    value.po2Deco = _preferences.getFloat("po2_max_deco", value.po2Deco);
    value.o2Air = _preferences.getFloat("o2_calib_21", value.o2Air);
    value.o2Pure = _preferences.getFloat("o2_calib_100", value.o2Pure);
    value.heCalibration = _preferences.getFloat("he_calib_100", value.heCalibration);
    _rewrite = invalidSleep || !value.valid();
    return value;
  }

  bool save(const AnalyzerSettings& value, const AnalyzerSettings& previous) {
    if (!_ready || !value.valid()) return false;
    const bool force = _rewrite;
    const bool samePure = value.o2Pure == previous.o2Pure ||
                         (std::isnan(value.o2Pure) && std::isnan(previous.o2Pure));
    const bool saved =
      ((!force && value.o2Enabled == previous.o2Enabled) || _preferences.putBool("o2_enabled", value.o2Enabled) == 1) &&
      ((!force && value.coEnabled == previous.coEnabled) || _preferences.putBool("co_enabled", value.coEnabled) == 1) &&
      ((!force && value.heEnabled == previous.heEnabled) || _preferences.putBool("he_enabled", value.heEnabled) == 1) &&
      ((!force && value.calibrateOnStart == previous.calibrateOnStart) || _preferences.putBool("calib_start", value.calibrateOnStart) == 1) &&
      ((!force && value.brightness == previous.brightness) || _preferences.putUChar("brightness", value.brightness) == 1) &&
      ((!force && value.sleepMinutes == previous.sleepMinutes) || _preferences.putUChar("sleep_minutes", value.sleepMinutes) == 1) &&
      ((!force && value.po2Bottom == previous.po2Bottom) || _preferences.putFloat("po2_max_bottom", value.po2Bottom) == sizeof(float)) &&
      ((!force && value.po2Deco == previous.po2Deco) || _preferences.putFloat("po2_max_deco", value.po2Deco) == sizeof(float)) &&
      ((!force && value.o2Air == previous.o2Air) || _preferences.putFloat("o2_calib_21", value.o2Air) == sizeof(float)) &&
      ((!force && samePure) || _preferences.putFloat("o2_calib_100", value.o2Pure) == sizeof(float)) &&
      ((!force && value.heCalibration == previous.heCalibration) || _preferences.putFloat("he_calib_100", value.heCalibration) == sizeof(float));
    _rewrite = !saved;
    return saved;
  }

private:
  Preferences _preferences;
  bool _ready = false;
  bool _rewrite = false;
};

#endif