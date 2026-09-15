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
private:
  lv_obj_t* _sleepDropdown = nullptr;
};

#endif // DISPLAYMANAGER_H
