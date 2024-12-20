#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_open_config(lv_event_t * e);
extern void action_close_config(lv_event_t * e);
extern void action_calibrate_o2_21(lv_event_t * e);
extern void action_calibrate_o2_100(lv_event_t * e);
extern void action_calibrate_he(lv_event_t * e);
extern void action_brightness_change(lv_event_t * e);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/