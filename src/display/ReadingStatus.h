#ifndef READING_STATUS_H
#define READING_STATUS_H

#include <cstdio>
#include <cstring>
#include "screens.h"
#include "fonts.h"
#include "sensors/sensors.h"

inline void applyReadingStatus(const sensorsData& data) {
  struct Label {
    lv_obj_t* object;
    const char* name;
    ChannelState state;
  };
  const Label labels[] = {
    {objects.o2, "O2", data.o2State},
    {objects.co, "CO", data.coState},
    {objects.he, "He", data.heState},
    {objects.o2_1, "O2", data.o2State}
  };
  static const lv_font_t* normalFonts[4] = {};
  for (unsigned index = 0; index < 4; ++index) {
    const Label& label = labels[index];
    if (!label.object) continue;
    if (!normalFonts[index]) normalFonts[index] = lv_obj_get_style_text_font(label.object, LV_PART_MAIN);
    const lv_font_t* font = label.state == ChannelState::Valid ? normalFonts[index] : &ui_font_geneva16;
    if (lv_obj_get_style_text_font(label.object, LV_PART_MAIN) != font) {
      lv_obj_set_style_text_font(label.object, font, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (label.state != ChannelState::Valid) {
      char text[32];
      std::snprintf(text, sizeof(text), "%s: %s", label.name, channelStateText(label.state));
      if (std::strcmp(lv_label_get_text(label.object), text) != 0) lv_label_set_text(label.object, text);
    } else if (index == 2 && data.temperatureState != ChannelState::Valid) {
      char text[64];
      std::snprintf(text, sizeof(text), "He %.1f%%, T: %s", data.HeLevel.percentage,
                    channelStateText(data.temperatureState));
      if (std::strcmp(lv_label_get_text(label.object), text) != 0) lv_label_set_text(label.object, text);
    }
  }
}

#endif