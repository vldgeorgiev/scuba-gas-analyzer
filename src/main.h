#pragma once

#include <atomic>
#include "config.h"
#include "display/DisplayManager.h"
#include "sensors/sensors.h"
#include "sensors/SensorAccess.h"

extern Config config;
extern DisplayManager displayManager;

extern QueueHandle_t sensorDataQueue;
extern SemaphoreHandle_t gui_mutex;

extern SensorManager sensors;
extern SensorAccess sensorAccess;

extern std::atomic<bool> configOpen;