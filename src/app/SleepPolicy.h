#ifndef APP_SLEEP_POLICY_H
#define APP_SLEEP_POLICY_H

#include <cstdint>
#include <cstdio>
#include <cstddef>
#include "settings/Settings.h"

namespace app {

constexpr uint32_t APPLICATION_SLEEP_MARKER = 0x534C4550;

inline bool consumeSleepMarker(uint32_t& marker, bool expectedWake) {
  const bool confirmed = expectedWake && marker == APPLICATION_SLEEP_MARKER;
  marker = 0;
  return confirmed;
}

inline uint8_t sleepMinutesForSelection(uint32_t index) {
  return index < sizeof(SLEEP_OPTIONS) / sizeof(SLEEP_OPTIONS[0]) ? SLEEP_OPTIONS[index] : DEFAULT_SLEEP_MINUTES;
}

inline uint32_t sleepSelectionForMinutes(uint8_t minutes) {
  if (!AnalyzerSettings::validSleepMinutes(minutes)) minutes = DEFAULT_SLEEP_MINUTES;
  for (uint32_t index = 0; index < sizeof(SLEEP_OPTIONS) / sizeof(SLEEP_OPTIONS[0]); ++index) {
    if (SLEEP_OPTIONS[index] == minutes) return index;
  }
  return 0;
}

inline bool formatSleepOptions(char* text, size_t capacity) {
  size_t used = 0;
  for (uint8_t minutes : SLEEP_OPTIONS) {
    const char* separator = used == 0 ? "" : "\n";
    const int written = minutes == 0 ? std::snprintf(text + used, capacity - used, "%sOff", separator) :
        std::snprintf(text + used, capacity - used, "%s%u min", separator, static_cast<unsigned>(minutes));
    if (written < 0 || static_cast<size_t>(written) >= capacity - used) return false;
    used += static_cast<size_t>(written);
  }
  return true;
}

inline bool sleepDue(uint8_t minutes, uint32_t inactiveMs, bool inhibited, bool buttonReleased) {
  return AnalyzerSettings::validSleepMinutes(minutes) && minutes != 0 && !inhibited && buttonReleased &&
         inactiveMs >= static_cast<uint32_t>(minutes) * 60000;
}

inline bool preparationInterrupted(uint32_t initialInactivity, uint32_t currentInactivity,
                                   bool buttonReleased, bool touchPressed) {
  return !buttonReleased || touchPressed || currentInactivity < initialInactivity;
}

class WakeButton {
public:
  bool update(bool pressed, uint32_t now) {
    const bool changed = pressed != _pressed;
    if (changed) {
      _pressed = pressed;
      _changedAt = now;
    }
    const bool released = !pressed && static_cast<uint32_t>(now - _changedAt) >= 50;
    const bool activity = pressed || changed || released != _released;
    _released = released;
    return activity;
  }
  bool released() const { return _released; }

private:
  bool _pressed = true;
  bool _released = false;
  uint32_t _changedAt = 0;
};

}

#endif