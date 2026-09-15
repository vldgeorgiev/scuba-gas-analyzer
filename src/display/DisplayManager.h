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
  void setSleepMinutes(uint8_t minutes);
  uint8_t getSleepMinutes() const;
  void resetInactivity();
  uint32_t inactiveTime() const;
  bool prepareSleep();
  bool restoreAfterSleepAbort(uint8_t brightness);
  bool touchActive();
private:
  lv_obj_t* _sleepDropdown = nullptr;
  bool _panelSleeping = false;
  bool _touchSleepAttempted = false;
};

#endif // DISPLAYMANAGER_H
