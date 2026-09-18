/**
 * @file diagnostics_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "diagnostics_gen.h"
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

lv_obj_t * diagnostics_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");


    lv_obj_t * the_root = NULL;

    #if LVGL_UI_PROJECT_CHECK_COMPILE_TARGET(LVGL_UI_PROJECT_TARGET_ALL)
    if (lvgl_ui_project_check_target(LVGL_UI_PROJECT_TARGET_ALL)) {
        lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
        lv_obj_set_name_static(lv_obj_0, "diagnostics_#");
        lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_style_pad_all(lv_obj_0, 0, 0);

        lv_obj_t * row_0 = row_create(lv_obj_0, 0, 0, 0, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_width(row_0, lv_pct(100));
        lv_obj_set_height(row_0, lv_pct(100));
        lv_obj_t * diagnostics_back = button_create(row_0, "", icon_arrow_left, COLOR_ACCENT, COLOR_ACCENT_TEXT, RADIUS_DEFAULT);
        lv_obj_set_name(diagnostics_back, "diagnostics_back");
        lv_obj_add_screen_create_event(diagnostics_back, LV_EVENT_CLICKED, calibration_create, LV_SCREEN_LOAD_ANIM_NONE, 0, 0);

        lv_obj_t * diagnostics_list = column_create(row_0, SPACE_SM, SPACE_XS, 1, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_name(diagnostics_list, "diagnostics_list");
        lv_obj_set_height(diagnostics_list, lv_pct(100));
        lv_obj_set_flag(diagnostics_list, LV_OBJ_FLAG_SCROLLABLE, true);
        h5_create(diagnostics_list, "Diagnostics", "");

        lv_obj_t * diagnostics_log = lv_label_create(diagnostics_list);
        lv_obj_set_name(diagnostics_log, "diagnostics_log");
        lv_label_bind_text(diagnostics_log, &diagnostics_log_text, NULL);
        lv_obj_set_width(diagnostics_log, lv_pct(100));
        lv_label_set_long_mode(diagnostics_log, LV_LABEL_LONG_MODE_WRAP);

        the_root = lv_obj_0;
    }
    #endif

    LV_TRACE_OBJ_CREATE("finished");

    return the_root;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

