#pragma once

#include "app/Analyzer.h"
#include "display/DisplayManager.h"

extern DisplayManager displayManager;

bool submitAnalyzerCommand(app::Command command);
bool cancelAnalyzerCalibration();
const AnalyzerSettings& uiSettings();
void syncUiSettings();
void openUiSettings();
bool closeUiSettings(const AnalyzerSettings& draft);
void showAnalyzerResult(const app::Result& result);
void setNetworkOperationActive(bool active);
void messageBox(const char* title, float value);