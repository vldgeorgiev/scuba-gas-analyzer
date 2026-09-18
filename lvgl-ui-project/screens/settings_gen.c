/**
 * @file settings_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "settings_gen.h"
#include "../lvgl_ui_project.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *  STATIC VARIABLES
 **********************/

/***********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * settings_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");


    lv_obj_t * the_root = NULL;

    #if LVGL_UI_PROJECT_CHECK_COMPILE_TARGET(LVGL_UI_PROJECT_TARGET_ALL)
    if (lvgl_ui_project_check_target(LVGL_UI_PROJECT_TARGET_ALL)) {
        lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
        lv_obj_set_name_static(lv_obj_0, "settings_#");
        lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);

        lv_obj_t * row_0 = row_create(lv_obj_0, SPACE_SM, SPACE_SM, 0, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_width(row_0, lv_pct(100));
        lv_obj_set_height(row_0, lv_pct(100));
        lv_obj_t * settings_back = button_create(row_0, "", icon_arrow_left, COLOR_ACCENT, COLOR_ACCENT_TEXT, RADIUS_DEFAULT);
        lv_obj_set_name(settings_back, "settings_back");
        lv_obj_add_screen_load_event(settings_back, LV_EVENT_CLICKED, mainscr, LV_SCREEN_LOAD_ANIM_NONE, 0, 0);

        lv_obj_t * settings_list = column_create(row_0, 0, SPACE_MD, 1, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_name(settings_list, "settings_list");
        lv_obj_set_height(settings_list, lv_pct(100));
        lv_obj_set_flag(settings_list, LV_OBJ_FLAG_SCROLLABLE, true);
        lv_obj_set_style_pad_right(settings_list, SPACE_MD, 0);
        lv_obj_t * o2_row = row_create(settings_list, 0, 0, 0, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_name(o2_row, "o2_row");
        lv_obj_set_width(o2_row, lv_pct(100));
        h5_create(o2_row, "O2", "");

        lv_obj_t * o2_enabled = switch_create(o2_row, &settings_o2_enabled, COLOR_ACCENT);
        lv_obj_set_name(o2_enabled, "o2_enabled");
        lv_obj_set_ext_click_area(o2_enabled, SPACE_SM);

        lv_obj_t * he_row = row_create(settings_list, 0, 0, 0, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_name(he_row, "he_row");
        lv_obj_set_width(he_row, lv_pct(100));
        h5_create(he_row, "He", "");

        lv_obj_t * he_enabled = switch_create(he_row, &settings_he_enabled, COLOR_ACCENT);
        lv_obj_set_name(he_enabled, "he_enabled");
        lv_obj_set_ext_click_area(he_enabled, SPACE_SM);

        lv_obj_t * co_row = row_create(settings_list, 0, 0, 0, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_name(co_row, "co_row");
        lv_obj_set_width(co_row, lv_pct(100));
        h5_create(co_row, "CO", "");

        lv_obj_t * co_enabled = switch_create(co_row, &settings_co_enabled, COLOR_ACCENT);
        lv_obj_set_name(co_enabled, "co_enabled");
        lv_obj_set_ext_click_area(co_enabled, SPACE_SM);

        lv_obj_t * po2_bottom_row = row_create(settings_list, 0, 0, 0, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_name(po2_bottom_row, "po2_bottom_row");
        lv_obj_set_width(po2_bottom_row, lv_pct(100));
        h5_create(po2_bottom_row, "Bottom pO2", "");

        lv_obj_t * po2_bottom = dropdown_create(po2_bottom_row, "1.00\n1.10\n1.20\n1.30\n1.40\n1.50\n1.60", 4, &settings_po2_bottom_index);
        lv_obj_set_name(po2_bottom, "po2_bottom");
        lv_obj_set_width(po2_bottom, 104);

        lv_obj_t * po2_deco_row = row_create(settings_list, 0, 0, 0, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_name(po2_deco_row, "po2_deco_row");
        lv_obj_set_width(po2_deco_row, lv_pct(100));
        h5_create(po2_deco_row, "Deco pO2", "");

        lv_obj_t * po2_deco = dropdown_create(po2_deco_row, "1.00\n1.10\n1.20\n1.30\n1.40\n1.50\n1.60\n1.70\n1.80\n1.90\n2.00", 6, &settings_po2_deco_index);
        lv_obj_set_name(po2_deco, "po2_deco");
        lv_obj_set_width(po2_deco, 104);

        lv_obj_t * brightness_row = row_create(settings_list, 0, 0, 0, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_name(brightness_row, "brightness_row");
        lv_obj_set_width(brightness_row, lv_pct(100));
        h5_create(brightness_row, "Brightness", "");

        lv_obj_t * brightness = slider_create(brightness_row, &settings_brightness, 8, 255, COLOR_ACCENT);
        lv_obj_set_name(brightness, "brightness");
        lv_obj_set_width(brightness, 112);
        lv_obj_set_ext_click_area(brightness, SPACE_LG);

        lv_obj_t * sleep_row = row_create(settings_list, 0, 0, 0, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_name(sleep_row, "sleep_row");
        lv_obj_set_width(sleep_row, lv_pct(100));
        h5_create(sleep_row, "Sleep", "");

        lv_obj_t * sleep_timeout = dropdown_create(sleep_row, "Never\n1 min\n2 min\n5 min\n10 min\n30 min", 3, &settings_sleep_index);
        lv_obj_set_name(sleep_timeout, "sleep_timeout");
        lv_obj_set_width(sleep_timeout, 112);

        lv_obj_t * open_updates = button_create(settings_list, "Firmware update", icon_chevron_right, COLOR_ACCENT, COLOR_ACCENT_TEXT, RADIUS_DEFAULT);
        lv_obj_set_name(open_updates, "open_updates");
        lv_obj_set_width(open_updates, lv_pct(100));

        the_root = lv_obj_0;
    }
    #endif

    LV_TRACE_OBJ_CREATE("finished");

    return the_root;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

