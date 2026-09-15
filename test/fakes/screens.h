#ifndef TEST_SCREENS_H
#define TEST_SCREENS_H

#include <string>

struct lv_font_t { unsigned height; };
struct lv_obj_t {
  std::string text;
  const lv_font_t* font;
};
constexpr unsigned LV_PART_MAIN = 0;
constexpr unsigned LV_STATE_DEFAULT = 0;
inline const lv_font_t* lv_obj_get_style_text_font(lv_obj_t* object, unsigned) { return object->font; }
inline void lv_obj_set_style_text_font(lv_obj_t* object, const lv_font_t* font, unsigned) { object->font = font; }
inline const char* lv_label_get_text(lv_obj_t* object) { return object->text.c_str(); }
inline void lv_label_set_text(lv_obj_t* object, const char* text) { object->text = text; }

struct TestObjects {
  lv_obj_t* o2 = nullptr;
  lv_obj_t* co = nullptr;
  lv_obj_t* he = nullptr;
  lv_obj_t* o2_1 = nullptr;
};
inline TestObjects objects;

#endif