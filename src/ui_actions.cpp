#include <lvgl.h>
#include "structs.h"
#include "actions.h"
#include "vars.h"
#include "main.h"
#include "ui-log.h"
#include "pin_config.h"

void messageBox(const char * title, float value) {
  char text[32];
  snprintf(text, sizeof(text), "%.2f mv", value);

  lv_obj_t * mbox = lv_msgbox_create(NULL);
  lv_msgbox_add_close_button(mbox);
  lv_msgbox_add_title(mbox, title);
  lv_msgbox_add_text(mbox, text);
  lv_obj_set_size(mbox, LV_PCT(70), LV_SIZE_CONTENT);
}

void action_calibrate_o2_21(lv_event_t * e) {
  float value = sensors.calibrateO2_21();
  config.setO2Calibration21(value);
  messageBox("O2 Air", value);
}

void action_calibrate_o2_100(lv_event_t * e) {
  float value = sensors.calibrateO2_100();
  config.setO2Calibration100(value);
  messageBox("O2 100%", value);
}

void action_calibrate_he(lv_event_t * e) {
  float value = sensors.calibrateHe_100();
  config.setHeCalibration100(value);
  messageBox("He 100%", value);
}

void action_reset_o2_21(lv_event_t * e) {
  log_i("Resetting O2 21 calibration");
  config.setO2Calibration21(config.O2_CALIBRATION_21_DEFAULT);
}

void action_reset_o2_100(lv_event_t * e) {
  log_i("Resetting O2 100 calibration");
  config.setO2Calibration100(config.O2_CALIBRATION_100_DEFAULT);
}

void action_reset_he(lv_event_t * e) {
  log_i("Resetting He calibration");
  config.setHeCalibration100(config.HE_CALIBRATION_100_DEFAULT);
}

void action_open_config(lv_event_t * e) {
  log_i("Opening config");
  configOpen = true;
}

void action_close_config(lv_event_t * e) {
  log_i("Closing config");
  config.setO2Enabled(flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_O2_ENABLED).getBoolean());
  config.setCOEnabled(flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_CO_ENABLED).getBoolean());
  config.setHeEnabled(flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_HE_ENABLED).getBoolean());

  config.setPO2Bottom(flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_PO2_MAX_BOTTOM).getFloat());
  config.setPO2Deco(flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_PO2_MAX_DECO).getFloat());

  config.setCalibrateOnStart(flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_CALIBRATE_ON_START).getBoolean());
  config.setBrightness(flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_BRIGHTNESS).getUInt8());

  digitalWrite(PIN_HE_ENABLE, config.getHeEnabled());
  digitalWrite(PIN_CO_ENABLE, config.getCOEnabled());

  configOpen = false;
}

const char *get_var_ui_log() {
  return UiLog::getInstance().getLogAsCString();
}

void set_var_ui_log(const char *value) {
  // ui_log is read only
}

void action_brightness_change(lv_event_t * e) {
  log_i("Brightness change");
  displayManager.setBrightness(flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_BRIGHTNESS).getUInt8());
}
