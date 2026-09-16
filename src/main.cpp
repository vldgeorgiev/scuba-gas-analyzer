#include "main.h"
#include "display/UiAdapter.h"
#include "ui-log.h"
#include "utils.h"
#include "app/SleepPolicy.h"
#include "app/DeviceSleep.h"

DisplayManager displayManager;

static QueueHandle_t measurementQueue;
static QueueHandle_t commandQueue;
static QueueHandle_t resultQueue;
static app::UiState uiState;
static bool networkOperationActive = false;
static bool applicationWake = false;

void setNetworkOperationActive(bool active) {
  networkOperationActive = active;
  displayManager.resetInactivity();
}

const AnalyzerSettings& uiSettings() { return uiState.effective; }

void openUiSettings() {
  if (uiState.settingsEditing) return;
  uiState.settingsEditing = true;
  syncUiSettings();
}

bool closeUiSettings(const AnalyzerSettings& draft) {
  if (!uiState.settingsEditing) return true;
  const bool submitted = uiState.closeSettings(draft, commandQueue);
  syncUiSettings();
  displayManager.resetInactivity();
  return submitted;
}

bool submitAnalyzerCommand(app::Command command) {
  return uiState.submit(command, commandQueue);
}

void syncUiSettings() {
  const AnalyzerSettings& settings = uiSettings();
  ui::syncSettings(settings);
  displayManager.setBrightness(settings.brightness);
}

static void presentReadings(const sensorsData& data) {
  ui::presentReadings(data, uiSettings());
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
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  pinMode(PIN_BUTTON_2, INPUT_PULLUP);
  app::WakeButton wakeButton;
  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  uint32_t idleAtPreparation = 0;
#endif

  for (;;) {
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    if (wakeButton.update(digitalRead(PIN_BUTTON_2) == LOW, ::millis())) displayManager.resetInactivity();
    if (digitalRead(PIN_BUTTON_1) == LOW) displayManager.resetInactivity();
    if ((uiState.sleepPhase == app::SleepPhase::Preparing || uiState.sleepPhase == app::SleepPhase::Prepared) &&
        (uiState.settingsEditing ||
         app::preparationInterrupted(idleAtPreparation, displayManager.inactiveTime(),
                                    wakeButton.released(), displayManager.touchActive()))) {
      uiState.requestResume(commandQueue);
    }
#endif
    if (uiState.preparationExpired(::millis())) {
      uiState.requestResume(commandQueue);
      logUi("Sleep preparation timed out; resuming", UiLogLevel::Error);
      messageBox("Sleep preparation timed out - staying awake", NAN);
    }
    if (uiState.sleepPhase == app::SleepPhase::Resuming && !uiState.resumeQueued) {
      uiState.requestResume(commandQueue);
    }
    app::Result result;
    if (xQueueReceive(resultQueue, &result, 0) == pdPASS && uiState.accept(result)) {
      if (!uiState.busy()) displayManager.resetInactivity();
      if (uiState.shouldSyncSettings(result)) syncUiSettings();
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
          ui::presentBattery(getBatteryVoltage());
        }
#endif
        pendingReading = false;
        presentedFresh = fresh;
      }
      ui::presentStatus(displayed, uiState.busy(), uiState.ready);
      lastPresentation = now;
    }
    displayManager.tick();
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    if (uiState.sleepPhase == app::SleepPhase::Prepared) {
      if (uiState.settingsEditing || app::preparationInterrupted(idleAtPreparation, displayManager.inactiveTime(),
                       wakeButton.released() && digitalRead(PIN_BUTTON_2) != LOW,
                       displayManager.touchActive())) {
        uiState.requestResume(commandQueue);
      } else {
        const char* sleepFailure = app::enterDeviceSleep(displayManager);
        log_e("Sleep aborted: %s", sleepFailure);
        const bool restored = displayManager.restoreAfterSleepAbort(uiSettings().brightness);
        uiState.requestResume(commandQueue);
        logUi(sleepFailure, UiLogLevel::Error);
        if (!restored) logUi("Touch restoration failed", UiLogLevel::Error);
        messageBox(restored ? sleepFailure : "Sleep aborted - touch unavailable", NAN);
      }
    } else if (uiState.sleepPhase == app::SleepPhase::Awake &&
               app::sleepDue(uiSettings().sleepMinutes, displayManager.inactiveTime(),
                             uiState.busy() || networkOperationActive || uiState.settingsEditing,
                             wakeButton.released())) {
      app::Command prepare;
      prepare.type = app::CommandType::PrepareSleep;
      idleAtPreparation = displayManager.inactiveTime();
      if (!uiState.submit(prepare, commandQueue)) displayManager.resetInactivity();
    }
#endif
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

static void Task_Analyzer(void*) {
  SettingsStore settingsStore;
  SensorManager sensors(measurementQueue);
  app::Analyzer analyzer(settingsStore, sensors);
  analyzer.begin(applicationWake);
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
  applicationWake = app::consumeDeviceWake();
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