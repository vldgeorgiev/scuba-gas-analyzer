#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <cstdint>
#include <lvgl.h>

class DisplayManager {
public:
  DisplayManager() {};
  void init();
  void tick();
  void setBrightness(uint8_t brightness);
  void resetInactivity();
  uint32_t inactiveTime() const;
  bool prepareSleep();
  const char* sleepFailure() const { return _sleepFailure; }
  bool restoreAfterSleepAbort(uint8_t brightness);
  bool touchActive();
private:
  bool resetTouchController();
  bool _panelSleeping = false;
  bool _touchSleepAttempted = false;
  const char* _sleepFailure = "Display sleep failed";
};

#endif // DISPLAYMANAGER_H
