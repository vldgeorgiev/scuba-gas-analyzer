#ifndef EEZ_LVGL_UI_STYLES_H
#define EEZ_LVGL_UI_STYLES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Style: LabelStyle
lv_style_t *get_style_label_style_MAIN_DEFAULT();
void add_style_label_style(lv_obj_t *obj);
void remove_style_label_style(lv_obj_t *obj);

// Style: PanelStyle
lv_style_t *get_style_panel_style_MAIN_DEFAULT();
void add_style_panel_style(lv_obj_t *obj);
void remove_style_panel_style(lv_obj_t *obj);



#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_STYLES_H*/