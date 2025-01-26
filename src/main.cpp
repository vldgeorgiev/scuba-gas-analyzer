#include <atomic>
#include "display/DisplayManager.h"
#include "ui.h"
#include "vars.h"
#include "structs.h"
#include "sensors/sensors.h"
#include "config.h"
#include "main.h"
#include "ui-log.h"
#include "pin_config.h"
#include "utils.h"

Config config;
DisplayManager displayManager;

QueueHandle_t sensorDataQueue;
SemaphoreHandle_t gui_mutex;

SensorManager sensors(sensorDataQueue);

std::atomic<bool> configOpen;

void Task_LVGL(void *pvParameters) {
  displayManager.init();
  displayManager.setBrightness(config.getBrightness());
  while (1) {
    if (xSemaphoreTake(gui_mutex, portMAX_DELAY) == pdTRUE) {
      displayManager.tick();
      xSemaphoreGive(gui_mutex);
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void Task_Screen_Update(void *pvParameters) {
  delay(1000); // TODO why the EEZ flow crashes without this delay? 300 for esp32dev, 1000 for lilygo
  if (xSemaphoreTake(gui_mutex, portMAX_DELAY) == pdTRUE) {
    flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_O2_ENABLED, config.getO2Enabled());
    flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_CO_ENABLED, config.getCOEnabled());
    flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_HE_ENABLED, config.getHeEnabled());
    flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_PO2_MAX_BOTTOM, FloatValue(config.getPO2Bottom()));
    flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_PO2_MAX_DECO, FloatValue(config.getPO2Deco()));
    flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_CALIBRATE_ON_START, config.getCalibrateOnStart());
    flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_BRIGHTNESS, IntegerValue(config.getBrightness()));
    xSemaphoreGive(gui_mutex);
  }

  while (true) {
    // log_d("[Task_Screen_Update] running on core: %d, Free stack space: %d", xPortGetCoreID(), uxTaskGetStackHighWaterMark(NULL));
    sensorsData data;
    if (xQueueReceive(sensorDataQueue, &data, portMAX_DELAY) == pdPASS) {
      if (xSemaphoreTake(gui_mutex, portMAX_DELAY) == pdTRUE) {
        int maxDepthBottomO2 = config.getPO2Bottom() / data.O2Level.percentage * 1000 - 10;
        int maxDepthDecoO2 = config.getPO2Deco() / data.O2Level.percentage * 1000 - 10;
        flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_O2_VALUE, FloatValue(data.O2Level.percentage));
        flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_O2_MILLIVOLTS, FloatValue(data.O2Level.millivolts));
        flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_MOD_PO2_BOTTOM, IntegerValue(maxDepthBottomO2));
        flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_MOD_PO2_DECO, IntegerValue(maxDepthDecoO2));
        flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_CO_VALUE, IntegerValue(data.CoLevel.ppm));
        flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_CO_MILLIVOLTS, FloatValue(data.CoLevel.millivolts));
        flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_HE_VALUE, FloatValue(data.HeLevel.percentage));
        flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_HE_MILLIVOLTS, FloatValue(data.HeLevel.millivolts));
        flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_HE_TEMPERATURE, FloatValue(data.HeTemperature));

        switch (UiLog::getInstance().getLevel()) {
          case UiLogLevel::None:
            flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_UI_LOG_LEVEL, logLevel_None);
            break;
          case UiLogLevel::Warning:
            flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_UI_LOG_LEVEL, logLevel_Warning);
            break;
          case UiLogLevel::Error:
            flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_UI_LOG_LEVEL, logLevel_Error);
            break;
        }

#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
        // TODO Should be battery reading be here? No need to check so often. Better place in its own task together with other utility checks
        float voltage = getBatteryVoltage();
        flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_BATT_VOLTAGE, FloatValue(voltage));
#endif

        xSemaphoreGive(gui_mutex);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void Task_Sensors(void *pvParameters) {
  if (config.getCalibrateOnStart()) {
    float o2CalibrationAir = sensors.calibrateO2_21();
    config.setO2Calibration21(o2CalibrationAir);
  }
  sensors.setSensorsConfig(config.getO2Enabled(), config.getCOEnabled(), config.getHeEnabled(), config.getO2Calibration21(), config.getO2Calibration100(), config.getHeCalibration100());

  TickType_t xLastWakeTime = xTaskGetTickCount();
  while (true)
  {
    if (!configOpen)
      sensors.readSensors();

    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
  }
}

void setup() {
  Serial.begin(115200);
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  Serial.setDebugOutput(true); // To catch logs over the USB https://thingpulse.com/usb-settings-for-logging-with-the-esp32-s3-in-platformio
#endif

  gui_mutex = xSemaphoreCreateMutex();
  if (gui_mutex == NULL) {
    log_e("GUI mutex creation failed");
    logUi("GUI mutex creation failed", UiLogLevel::Error);
  }

  sensorDataQueue = xQueueCreate(5, sizeof(sensorsData));
  if (sensorDataQueue == NULL) {
    log_e("Failed to create sensor data queue");
    logUi("Failed to create sensor data queue", UiLogLevel::Error);
  }

  config.begin();
  sensors.init();

  xTaskCreatePinnedToCore(Task_LVGL, "Task_LVGL", 1024 * 10, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(Task_Screen_Update, "Task_Screen_Update", 1024 * 3, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(Task_Sensors, "Task_Sensors", 1024 * 3, NULL, 1, NULL, 1);
}

void loop() {} // All work is done in the tasks
