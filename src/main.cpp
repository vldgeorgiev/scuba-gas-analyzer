#include "main.h"
#include "display/ReadingStatus.h"
#include "ui.h"
#include "vars.h"
#include "structs.h"
#include "ui-log.h"
#include "utils.h"

DisplayManager displayManager;

static QueueHandle_t measurementQueue;
static QueueHandle_t commandQueue;
static QueueHandle_t resultQueue;
static app::UiState uiState;
static bool settingsEditing = false;

const AnalyzerSettings& uiSettings() { return uiState.effective; }
void setUiSettingsEditing(bool editing) { settingsEditing = editing; }

bool submitAnalyzerCommand(app::Command command) {
  return uiState.submit(command, commandQueue);
}

void syncUiSettings() {
  const AnalyzerSettings& settings = uiSettings();
  flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_O2_ENABLED, settings.o2Enabled);
  flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_CO_ENABLED, settings.coEnabled);
  flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_HE_ENABLED, settings.heEnabled);
  flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_PO2_MAX_BOTTOM, FloatValue(settings.po2Bottom));
  flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_PO2_MAX_DECO, FloatValue(settings.po2Deco));
  flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_CALIBRATE_ON_START, settings.calibrateOnStart);
  flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_BRIGHTNESS, IntegerValue(settings.brightness));
  displayManager.setBrightness(settings.brightness);
}

static eez::Value integerOrUnavailable(float value) {
  int integer;
  return conversions::toInt(value, integer) ? IntegerValue(integer) : FloatValue(NAN);
}

static void presentReadings(const sensorsData& data) {
  const AnalyzerSettings& settings = uiSettings();
  const float bottom = conversions::maximumOperatingDepth(settings.po2Bottom, data.O2Level.percentage);
  const float deco = conversions::maximumOperatingDepth(settings.po2Deco, data.O2Level.percentage);
  flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_O2_VALUE, FloatValue(data.O2Level.percentage));
  flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_O2_MILLIVOLTS, FloatValue(data.O2Level.millivolts));
  flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_MOD_PO2_BOTTOM, integerOrUnavailable(bottom));
  flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_MOD_PO2_DECO, integerOrUnavailable(deco));
  flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_CO_VALUE, integerOrUnavailable(data.CoLevel.ppm));
  flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_CO_MILLIVOLTS, FloatValue(data.CoLevel.millivolts));
  flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_HE_VALUE, FloatValue(data.HeLevel.percentage));
  flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_HE_MILLIVOLTS, FloatValue(data.HeLevel.millivolts));
  flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_HE_TEMPERATURE, FloatValue(data.HeTemperature));
}

static void Task_UI(void*) {
  displayManager.init();
  syncUiSettings();
  sensorsData latest;
  sensorsData displayed;
  presentReadings(displayed);
  bool pendingReading = true;
  bool presentedFresh = false;
  uint32_t lastPresentation = 0;
  SensorError previousError = SensorError::None;

  for (;;) {
    app::Result result;
    if (xQueueReceive(resultQueue, &result, 0) == pdPASS && uiState.accept(result)) {
      if (!settingsEditing || result.type == app::CommandType::Startup) syncUiSettings();
      displayed = latest.forDisplay(::millis(), uiState.generation);
      presentReadings(displayed);
      pendingReading = true;
      showAnalyzerResult(result);
    }
    sensorsData incoming;
    if (xQueueReceive(measurementQueue, &incoming, 0) == pdPASS) {
      latest = incoming;
      pendingReading = true;
      if (incoming.lastError != SensorError::None && incoming.lastError != previousError) {
        logUi(SensorManager::getErrorString(incoming.lastError), UiLogLevel::Warning);
      }
      previousError = incoming.lastError;
    }
    const uint32_t now = ::millis();
    if (static_cast<uint32_t>(now - lastPresentation) >= 100) {
      const bool fresh = latest.isFresh(now);
      if (pendingReading || fresh != presentedFresh) {
        displayed = uiState.ready ? latest.forDisplay(now, uiState.generation) : sensorsData{};
        presentReadings(displayed);
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
        if (pendingReading) {
          flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_BATT_VOLTAGE, FloatValue(getBatteryVoltage()));
        }
#endif
        pendingReading = false;
        presentedFresh = fresh;
      }
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
      lastPresentation = now;
    }
    displayManager.tick();
    applyReadingStatus(displayed);
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

static void Task_Analyzer(void*) {
  Config config;
  SensorManager sensors(measurementQueue);
  app::Analyzer analyzer(config, sensors);
  analyzer.begin();
  uint32_t lastMeasurement = ::millis() - 500;

  for (;;) {
    if (analyzer.service(commandQueue, resultQueue)) {
      lastMeasurement = ::millis() - 500;
    }
    if (static_cast<uint32_t>(::millis() - lastMeasurement) >= 500) {
      analyzer.measure();
      lastMeasurement = ::millis();
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup() {
  Serial.begin(115200);
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  Serial.setDebugOutput(true);
#endif
  measurementQueue = xQueueCreate(1, sizeof(sensorsData));
  commandQueue = xQueueCreate(1, sizeof(app::Command));
  resultQueue = xQueueCreate(1, sizeof(app::Result));
  if (!measurementQueue || !commandQueue || !resultQueue) {
    log_e("Application queue allocation failed");
    return;
  }
  if (xTaskCreatePinnedToCore(Task_Analyzer, "Analyzer", 1024 * 4, nullptr, 1, nullptr, 1) != pdPASS) {
    log_e("Analyzer task creation failed");
    logUi("Analyzer unavailable", UiLogLevel::Error);
  }
  if (xTaskCreatePinnedToCore(Task_UI, "UI", 1024 * 10, nullptr, 3, nullptr, 0) != pdPASS) {
    log_e("UI task creation failed");
  }
}

void loop() { vTaskDelay(portMAX_DELAY); }