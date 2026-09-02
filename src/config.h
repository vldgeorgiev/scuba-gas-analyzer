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
  const char* O2_CALIBRATION_100 = "o2_calib_100";
  const char* PO2_MAX_BOTTOM = "po2_max_bottom";
  const float PO2_MAX_BOTTOM_DEFAULT = 1.4f;
  const char* PO2_MAX_DECO = "po2_max_deco";
  const float PO2_MAX_DECO_DEFAULT = 1.6f;
  const char* CO_ENABLED = "co_enabled";
  const char* HE_ENABLED = "he_enabled";
  const char* HE_CALIBRATION_100 = "he_calib_100";
  const char* CALIBRATE_ON_START = "calib_start";
  const char* BRIGHTNESS = "brightness";

public:
  const float O2_CALIBRATION_21_DEFAULT = 10;
  const float O2_CALIBRATION_100_DEFAULT = NAN;
  const float HE_CALIBRATION_100_DEFAULT = 620; // Looks like a suitable default, based on experiments. Might not always be accurate with different sensors.

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
    log_i("  Calibrate on start: %d", getCalibrateOnStart());
  }

  void setO2Enabled(bool value) {
    preferences.putBool(O2_ENABLED, value);
  }
  bool getO2Enabled() {
    return preferences.getBool(O2_ENABLED, true);
  }

  void setPO2Bottom(float value) {
    // Validate PO2 values (typical range for scuba diving)
    if (value >= 1.0f && value <= 1.6f) {
      preferences.putFloat(PO2_MAX_BOTTOM, value);
    } else {
      log_w("Invalid PO2 bottom value: %.2f (should be 1.0-1.6)", value);
    }
  }

  float getPO2Bottom() {
    return preferences.getFloat(PO2_MAX_BOTTOM, PO2_MAX_BOTTOM_DEFAULT);
  }

  void setPO2Deco(float value) {
    // Deco PO2 should be higher than bottom PO2
    if (value >= 1.0f && value <= 2.0f && value >= getPO2Bottom()) {
      preferences.putFloat(PO2_MAX_DECO, value);
    } else {
      log_w("Invalid PO2 deco value: %.2f (should be 1.0-2.0, >= bottom PO2)", value);
    }
  }

  float getPO2Deco() {
    return preferences.getFloat(PO2_MAX_DECO, PO2_MAX_DECO_DEFAULT);
  }

  void setO2Calibration21(float value) {
    // Validate O2 calibration values (reasonable range for O2 sensor in mV)
    if (value >= 5.0f && value <= 50.0f) {
      preferences.putFloat(O2_CALIBRATION_21, value);
    } else {
      log_w("Invalid O2 21%% calibration value: %.2f mV", value);
    }
  }

  float getO2Calibration21() {
    return preferences.getFloat(O2_CALIBRATION_21, O2_CALIBRATION_21_DEFAULT);
  }

  void setO2Calibration100(float value) {
    // 100% O2 should be higher than 21% calibration
    float cal21 = getO2Calibration21();
    if (value > cal21 && value <= 100.0f) {
      preferences.putFloat(O2_CALIBRATION_100, value);
    } else {
      log_w("Invalid O2 100%% calibration value: %.2f mV (21%% cal: %.2f mV)", value, cal21);
    }
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

  void setCalibrateOnStart(bool value) {
    preferences.putBool(CALIBRATE_ON_START, value);
  }

  bool getCalibrateOnStart() {
    return preferences.getBool(CALIBRATE_ON_START, true);
  }

  void setBrightness(uint8_t value) {
    preferences.putUChar(BRIGHTNESS, value);
  }

  uint8_t getBrightness() {
    return preferences.getUChar(BRIGHTNESS, 128);
  }
};

#endif // CONFIG_H