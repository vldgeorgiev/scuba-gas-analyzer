#pragma once

#include <cstdio>
#include "sensors/sensors.h"
#include "settings/Settings.h"

namespace ui {

inline void formatValue(char* text, size_t size, float value, ChannelState state,
                        const char* format) {
  if (state != ChannelState::Valid) {
    std::snprintf(text, size, "%s", channelStateText(state));
  } else if (!std::isfinite(value)) {
    std::snprintf(text, size, "Invalid");
  } else {
    std::snprintf(text, size, format, static_cast<double>(value));
  }
}

inline void formatMod(char* text, size_t size, const char* prefix, float po2,
                      float oxygen, ChannelState state) {
  int depth;
  if (state != ChannelState::Valid ||
      !conversions::toInt(conversions::maximumOperatingDepth(po2, oxygen), depth) || depth < 0) {
    std::snprintf(text, size, "%s -- m", prefix);
  } else {
    std::snprintf(text, size, "%s %d m", prefix, depth);
  }
}

inline int po2Selection(float value) {
  return std::isfinite(value) ? static_cast<int>(std::lround((value - 1.0f) * 10.0f)) : -1;
}

inline bool applySelections(AnalyzerSettings& candidate, int bottom, int deco,
                            int brightness, int sleep) {
  const int sleepCount = sizeof(app::SLEEP_OPTIONS) / sizeof(app::SLEEP_OPTIONS[0]);
  if (bottom < 0 || bottom > 6 || deco < 0 || deco > 10 ||
      brightness < 8 || brightness > 255 || sleep < 0 || sleep >= sleepCount) return false;
  AnalyzerSettings updated = candidate;
  if (bottom != po2Selection(updated.po2Bottom)) updated.po2Bottom = 1.0f + bottom / 10.0f;
  if (deco != po2Selection(updated.po2Deco)) updated.po2Deco = 1.0f + deco / 10.0f;
  updated.brightness = static_cast<uint8_t>(brightness);
  updated.sleepMinutes = app::SLEEP_OPTIONS[sleep];
  if (!updated.valid()) return false;
  candidate = updated;
  return true;
}

}