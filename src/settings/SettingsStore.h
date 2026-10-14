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
  bool loadedDefaults() const { return _loadedDefaults; }
  bool hasOxygenCalibration(bool external = false) const {
    return _ready && (external ? _externalOxygenAccepted : _oxygenAccepted);
  }
  bool hasHeliumCalibration() const { return _ready && _heliumAccepted; }

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
    value.coCorrectionPpm = _preferences.getChar("co_correction", value.coCorrectionPpm);
    value.po2Bottom = _preferences.getFloat("po2_max_bottom", value.po2Bottom);
    value.po2Deco = _preferences.getFloat("po2_max_deco", value.po2Deco);
    const float storedAir = _preferences.getFloat("o2_calib_21", NAN);
    _oxygenAccepted = AnalyzerSettings::validO2Air(storedAir);
    value.o2Air = std::isnan(storedAir) ? value.o2Air : storedAir;
    value.o2Pure = _preferences.getFloat("o2_calib_100", value.o2Pure);
    const float storedExternalAir = _preferences.getFloat("ext_o2_air", NAN);
    _externalOxygenAccepted = AnalyzerSettings::validO2Air(storedExternalAir);
    value.externalO2Air = std::isnan(storedExternalAir) ? value.externalO2Air : storedExternalAir;
    value.externalO2Pure = _preferences.getFloat("ext_o2_pure", value.externalO2Pure);
    if (!_externalOxygenAccepted) value.externalO2Pure = NAN;
    const float storedHelium = _preferences.getFloat("he_calib_100", NAN);
    _heliumAccepted = AnalyzerSettings::validHeCalibration(storedHelium);
    value.heCalibration = std::isnan(storedHelium) ? value.heCalibration : storedHelium;
    if (!_oxygenAccepted) value.o2Pure = NAN;
    _loadedDefaults = value.repairInvalidFields();
    _rewrite = invalidSleep || _loadedDefaults;
    return value;
  }

  bool save(const AnalyzerSettings& value, const AnalyzerSettings& previous,
            bool acceptOxygen = false, bool acceptHelium = false, bool acceptExternalOxygen = false) {
    if (!_ready || !value.valid()) return false;
    const bool force = _rewrite;
    const bool oxygenAccepted = _oxygenAccepted || acceptOxygen;
    const bool heliumAccepted = _heliumAccepted || acceptHelium;
    const bool externalOxygenAccepted = _externalOxygenAccepted || acceptExternalOxygen;
    const float externalAir = externalOxygenAccepted ? value.externalO2Air : NAN;
    const float previousExternalAir = _externalOxygenAccepted ? previous.externalO2Air : NAN;
    const float externalPure = externalOxygenAccepted ? value.externalO2Pure : NAN;
    const float previousExternalPure = _externalOxygenAccepted ? previous.externalO2Pure : NAN;
    const float air = oxygenAccepted ? value.o2Air : NAN;
    const float previousAir = _oxygenAccepted ? previous.o2Air : NAN;
    const float pure = oxygenAccepted ? value.o2Pure : NAN;
    const float previousPure = _oxygenAccepted ? previous.o2Pure : NAN;
    const float helium = heliumAccepted ? value.heCalibration : NAN;
    const float previousHelium = _heliumAccepted ? previous.heCalibration : NAN;
    const bool saved =
      ((!force && value.o2Enabled == previous.o2Enabled) || _preferences.putBool("o2_enabled", value.o2Enabled) == 1) &&
      ((!force && value.coEnabled == previous.coEnabled) || _preferences.putBool("co_enabled", value.coEnabled) == 1) &&
      ((!force && value.heEnabled == previous.heEnabled) || _preferences.putBool("he_enabled", value.heEnabled) == 1) &&
      ((!force && value.calibrateOnStart == previous.calibrateOnStart) || _preferences.putBool("calib_start", value.calibrateOnStart) == 1) &&
      ((!force && value.brightness == previous.brightness) || _preferences.putUChar("brightness", value.brightness) == 1) &&
      ((!force && value.sleepMinutes == previous.sleepMinutes) || _preferences.putUChar("sleep_minutes", value.sleepMinutes) == 1) &&
      ((!force && value.po2Bottom == previous.po2Bottom) || _preferences.putFloat("po2_max_bottom", value.po2Bottom) == sizeof(float)) &&
      ((!force && value.po2Deco == previous.po2Deco) || _preferences.putFloat("po2_max_deco", value.po2Deco) == sizeof(float)) &&
      ((!force && sameFloat(air, previousAir)) || _preferences.putFloat("o2_calib_21", air) == sizeof(float)) &&
      ((!force && !(!_oxygenAccepted && acceptOxygen) && sameFloat(pure, previousPure)) || _preferences.putFloat("o2_calib_100", pure) == sizeof(float)) &&
      ((!force && sameFloat(helium, previousHelium)) || _preferences.putFloat("he_calib_100", helium) == sizeof(float)) &&
      ((!force && sameFloat(externalAir, previousExternalAir)) || _preferences.putFloat("ext_o2_air", externalAir) == sizeof(float)) &&
      ((!force && !(!_externalOxygenAccepted && acceptExternalOxygen) && sameFloat(externalPure, previousExternalPure)) || _preferences.putFloat("ext_o2_pure", externalPure) == sizeof(float)) &&
      ((!force && value.coCorrectionPpm == previous.coCorrectionPpm) || _preferences.putChar("co_correction", value.coCorrectionPpm) == 1);
    _rewrite = !saved;
    if (saved) {
      _oxygenAccepted = oxygenAccepted;
      _heliumAccepted = heliumAccepted;
      _externalOxygenAccepted = externalOxygenAccepted;
    }
    return saved;
  }

private:
  static bool sameFloat(float value, float previous) {
    return value == previous || (std::isnan(value) && std::isnan(previous));
  }
  Preferences _preferences;
  bool _ready = false;
  bool _rewrite = false;
  bool _loadedDefaults = false;
  bool _oxygenAccepted = false;
  bool _externalOxygenAccepted = false;
  bool _heliumAccepted = false;
};

#endif