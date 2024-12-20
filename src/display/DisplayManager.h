#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

class DisplayManager {
public:
  DisplayManager() {};
  void init();
  void tick();
  void setBrightness(uint8_t brightness);
private:

};

#endif // DISPLAYMANAGER_H
