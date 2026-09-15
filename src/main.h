#pragma once

#include "app/Analyzer.h"
#include "display/DisplayManager.h"

extern DisplayManager displayManager;

bool submitAnalyzerCommand(app::Command command);
const AnalyzerSettings& uiSettings();
void syncUiSettings();
void openUiSettings();
bool closeUiSettings(const AnalyzerSettings& draft);
bool discardUiSettingsDraft();
void showAnalyzerResult(const app::Result& result);
void setNetworkOperationActive(bool active);
void messageBox(const char* title, float value);