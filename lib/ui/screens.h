#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *config;
    lv_obj_t *log;
    lv_obj_t *batt_indicator;
    lv_obj_t *btn_config;
    lv_obj_t *btn_home;
    lv_obj_t *btn_home_1;
    lv_obj_t *calibrate_100_;
    lv_obj_t *calibrate_100__1;
    lv_obj_t *calibrate_air;
    lv_obj_t *co;
    lv_obj_t *co_conf_pnl;
    lv_obj_t *co_enabled;
    lv_obj_t *co_pnl;
    lv_obj_t *he;
    lv_obj_t *he_conf_pnl;
    lv_obj_t *he_enabled;
    lv_obj_t *he_pnl;
    lv_obj_t *header;
    lv_obj_t *o2;
    lv_obj_t *o2_conf_pnl;
    lv_obj_t *o2_enabled;
    lv_obj_t *o2_pnl;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *obj7;
    lv_obj_t *warning_indicator;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_CONFIG = 2,
    SCREEN_ID_LOG = 3,
};

void create_screen_main();
void tick_screen_main();

void create_screen_config();
void tick_screen_config();

void create_screen_log();
void tick_screen_log();

void create_screens();
void tick_screen(int screen_index);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/