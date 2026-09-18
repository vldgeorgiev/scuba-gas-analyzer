#include "UiFeedback.h"
#include <lvgl.h>
#include "main.h"
#include "ui-log.h"

void messageBox(const char* title, float value) {
  char text[32] = "";
  if (std::isfinite(value)) snprintf(text, sizeof(text), "%.2f mv", value);

  lv_obj_t* mbox = lv_msgbox_create(NULL);
  lv_msgbox_add_close_button(mbox);
  lv_msgbox_add_title(mbox, title);
  lv_msgbox_add_text(mbox, text);
  lv_obj_set_size(mbox, LV_PCT(70), LV_SIZE_CONTENT);
}

void showAnalyzerResult(const app::Result& result) {
  if (result.sensorError != SensorError::None) {
    logUi(SensorManager::getErrorString(result.sensorError), UiLogLevel::Warning);
  }
  if (result.calibrationPhase != app::CalibrationPhase::None && !result.complete) return;
  const char* title = nullptr;
  const char* required = result.calibrationRequiredMessage();
  if (required && result.failure != app::Failure::CalibrationRequired &&
      result.type != app::CommandType::PrepareSleep && result.type != app::CommandType::Resume) {
    logUi(required, UiLogLevel::Warning);
  }
  if (result.failure != app::Failure::None) {
    switch (result.failure) {
      case app::Failure::Invalid: title = "Invalid settings or calibration"; break;
      case app::Failure::Storage: title = "Settings could not be saved"; break;
      case app::Failure::Sampling: title = "Calibration read failed"; break;
      case app::Failure::LoadedDefaults: title = "Invalid saved settings - defaults loaded"; break;
      case app::Failure::Busy: title = "Analyzer is preparing for sleep"; break;
      case app::Failure::CalibrationRequired:
        title = result.calibrationRequiredMessage();
        if (!title) title = "O2 calibration required";
        break;
      case app::Failure::Cancelled: break;
      default: title = "Operation failed"; break;
    }
    logUi(title, UiLogLevel::Error);
  } else {
    switch (result.type) {
      case app::CommandType::CalibrateAir: title = "O2 Air Calibrated"; break;
      case app::CommandType::CalibratePure: title = "O2 100% Calibrated"; break;
      case app::CommandType::CalibrateHe: title = "He 100% Calibrated"; break;
      case app::CommandType::ClearPure: title = "Pure O2 calibration cleared"; break;
      default: break;
    }
  }
  if (!title && required && result.type == app::CommandType::ApplySettings) title = required;
  if (title && result.calibrationPhase == app::CalibrationPhase::None) messageBox(title, result.calibration);
}