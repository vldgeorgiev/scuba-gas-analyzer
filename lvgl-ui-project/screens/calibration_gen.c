/**
 * @file calibration_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "calibration_gen.h"
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

lv_obj_t * calibration_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");


    lv_obj_t * the_root = NULL;

    #if LVGL_UI_PROJECT_CHECK_COMPILE_TARGET(LVGL_UI_PROJECT_TARGET_ALL)
    if (lvgl_ui_project_check_target(LVGL_UI_PROJECT_TARGET_ALL)) {
        lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
        lv_obj_set_name_static(lv_obj_0, "calibration_#");
        lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);

        lv_obj_t * row_0 = row_create(lv_obj_0, SPACE_SM, SPACE_SM, 0, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_width(row_0, lv_pct(100));
        lv_obj_set_height(row_0, lv_pct(100));
        lv_obj_t * column_0 = column_create(row_0, 0, SPACE_SM, 0, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_t * calibration_back = button_create(column_0, "", icon_arrow_left, COLOR_ACCENT, COLOR_ACCENT_TEXT, RADIUS_DEFAULT);
        lv_obj_set_name(calibration_back, "calibration_back");
        lv_obj_add_screen_load_event(calibration_back, LV_EVENT_CLICKED, mainscr, LV_SCREEN_LOAD_ANIM_NONE, 0, 0);

        lv_obj_t * open_diagnostics = button_create(column_0, "", icon_info, COLOR_ACCENT, COLOR_ACCENT_TEXT, RADIUS_DEFAULT);
        lv_obj_set_name(open_diagnostics, "open_diagnostics");
        lv_obj_add_screen_create_event(open_diagnostics, LV_EVENT_CLICKED, diagnostics_create, LV_SCREEN_LOAD_ANIM_NONE, 0, 0);

        lv_obj_t * calibration_list = column_create(row_0, 0, SPACE_SM, 1, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_name(calibration_list, "calibration_list");
        lv_obj_set_flag(calibration_list, LV_OBJ_FLAG_SCROLLABLE, true);
        h5_create(calibration_list, "Calibration", "");

        lv_obj_t * row_1 = row_create(calibration_list, 0, 0, 0, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_width(row_1, lv_pct(100));
        text_create(row_1, "Calibrate air on start");

        lv_obj_t * startup_calibration = switch_create(row_1, &calibration_on_start, COLOR_ACCENT);
        lv_obj_set_name(startup_calibration, "startup_calibration");

        lv_obj_t * calibrate_air = button_create(calibration_list, "Air (20.9%)", icon_refresh, COLOR_ACCENT, COLOR_ACCENT_TEXT, RADIUS_DEFAULT);
        lv_obj_set_name(calibrate_air, "calibrate_air");
        lv_obj_set_width(calibrate_air, lv_pct(100));

        lv_obj_t * row_2 = row_create(calibration_list, 0, 0, 0, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_width(row_2, lv_pct(100));
        lv_obj_t * calibrate_o2 = button_create(row_2, "O2 (100%)", icon_refresh, COLOR_ACCENT, COLOR_ACCENT_TEXT, RADIUS_DEFAULT);
        lv_obj_set_name(calibrate_o2, "calibrate_o2");
        lv_obj_set_width(calibrate_o2, lv_pct(49));

        lv_obj_t * clear_o2 = button_create(row_2, "Clear O2", icon_trash, COLOR_ACCENT, COLOR_ACCENT_TEXT, RADIUS_DEFAULT);
        lv_obj_set_name(clear_o2, "clear_o2");
        lv_obj_set_width(clear_o2, lv_pct(49));

        lv_obj_t * calibrate_he = button_create(calibration_list, "He (100%)", icon_refresh, COLOR_ACCENT, COLOR_ACCENT_TEXT, RADIUS_DEFAULT);
        lv_obj_set_name(calibrate_he, "calibrate_he");
        lv_obj_set_width(calibrate_he, lv_pct(100));

        the_root = lv_obj_0;
    }
    #endif

    LV_TRACE_OBJ_CREATE("finished");

    return the_root;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

