#ifndef CONFIG_H
#define CONFIG_H

#include <Preferences.h>
#include "ui-log.h"

class Config {
private:
  Preferences preferences;
  // Max key length is 15 characters
  const char* O2_ENABLED = "o2_enabled";
  const char* O2_CALIBRATION_21 = "o2_calib_21";
  const float O2_CALIBRATION_21_DEFAULT = 10;
  const char* O2_CALIBRATION_100 = "o2_calib_100";
  const float O2_CALIBRATION_100_DEFAULT = NAN;
  const char* PO2_MAX_BOTTOM = "po2_max_bottom";
  const float PO2_MAX_BOTTOM_DEFAULT = 1.4f;
  const char* PO2_MAX_DECO = "po2_max_deco";
  const float PO2_MAX_DECO_DEFAULT = 1.6f;
  const char* CO_ENABLED = "co_enabled";
  const char* HE_ENABLED = "he_enabled";
  const char* HE_CALIBRATION_100 = "he_calib_100";
  const float HE_CALIBRATION_100_DEFAULT = 500;

public:
  Config() {}

  ~Config() {
    preferences.end();
  }

  void begin() {
    if (!preferences.begin("config", false)) {
      log_e("Failed to open preferences");
      logUi("Failed to open preferences", UiLogLevel::Error);
    }

    // Log existing preference values
    log_i("Config values loaded:");
    log_i("  O2 Enabled: %d", getO2Enabled());
    log_i("  O2 Calibration 21: %.2f", getO2Calibration21());
    log_i("  O2 Calibration 100: %.2f", getO2Calibration100());
    log_i("  PO2 Max Bottom: %.2f", getPO2Bottom());
    log_i("  PO2 Max Deco: %.2f", getPO2Deco());
    log_i("  CO Enabled: %d", getCOEnabled());
    log_i("  HE Enabled: %d", getHeEnabled());
    log_i("  HE Calibration 100: %.2f", getHeCalibration100());
  }

  void setO2Enabled(bool value) {
    preferences.putBool(O2_ENABLED, value);
  }
  bool getO2Enabled() {
    return preferences.getBool(O2_ENABLED, true);
  }

  void setPO2Bottom(float value) {
    preferences.putFloat(PO2_MAX_BOTTOM, value);
  }

  float getPO2Bottom() {
    return preferences.getFloat(PO2_MAX_BOTTOM, PO2_MAX_BOTTOM_DEFAULT);
  }

  void setPO2Deco(float value) {
    preferences.putFloat(PO2_MAX_DECO, value);
  }

  float getPO2Deco() {
    return preferences.getFloat(PO2_MAX_DECO, PO2_MAX_DECO_DEFAULT);
  }

  void setO2Calibration21(float value) {
    preferences.putFloat(O2_CALIBRATION_21, value);
  }

  float getO2Calibration21() {
    return preferences.getFloat(O2_CALIBRATION_21, O2_CALIBRATION_21_DEFAULT);
  }

  void setO2Calibration100(float value) {
    preferences.putFloat(O2_CALIBRATION_100, value);
  }

  float getO2Calibration100() {
    return preferences.getFloat(O2_CALIBRATION_100, O2_CALIBRATION_100_DEFAULT);
  }

  void setCOEnabled(bool value) {
    preferences.putBool(CO_ENABLED, value);
  }
  bool getCOEnabled() {
    return preferences.getBool(CO_ENABLED, true);
  }

  void setHeEnabled(bool value) {
    preferences.putBool(HE_ENABLED, value);
  }

  bool getHeEnabled() {
    return preferences.getBool(HE_ENABLED, true);
  }

  void setHeCalibration100(float value) {
    preferences.putFloat(HE_CALIBRATION_100, value);
  }

  float getHeCalibration100() {
    return preferences.getFloat(HE_CALIBRATION_100, HE_CALIBRATION_100_DEFAULT);
  }
};

#endif // CONFIG_H