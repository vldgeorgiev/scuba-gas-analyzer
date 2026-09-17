#pragma once

#include <lvgl.h>
#include "app/Analyzer.h"
#include "settings/Settings.h"
#include "sensors/sensors.h"

namespace ui {
void init();
void syncSettings(const AnalyzerSettings& settings);
void syncActionSettings(const AnalyzerSettings& settings);
bool readSettings(AnalyzerSettings& settings);
void presentReadings(const sensorsData& data, const AnalyzerSettings& settings);
void presentBattery(float voltage);
void presentStatus(const sensorsData& data, bool busy, bool ready);
void presentCalibration(const app::Result& result);
void openCalibration(lv_event_t* event);
void openUpdates(lv_event_t* event);
void openLogs(lv_event_t* event);
}