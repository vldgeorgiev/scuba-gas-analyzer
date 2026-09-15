#pragma once

#include "app/Analyzer.h"
#include "display/DisplayManager.h"

extern DisplayManager displayManager;

bool submitAnalyzerCommand(app::Command command);
const AnalyzerSettings& uiSettings();
void syncUiSettings();
void setUiSettingsEditing(bool editing);
void showAnalyzerResult(const app::Result& result);
void setNetworkOperationActive(bool active);
void messageBox(const char* title, float value);