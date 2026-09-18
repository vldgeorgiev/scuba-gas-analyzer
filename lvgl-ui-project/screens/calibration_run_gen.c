/**
 * @file calibration_run_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "calibration_run_gen.h"
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

lv_obj_t * calibration_run_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");


    lv_obj_t * the_root = NULL;

    #if LVGL_UI_PROJECT_CHECK_COMPILE_TARGET(LVGL_UI_PROJECT_TARGET_ALL)
    if (lvgl_ui_project_check_target(LVGL_UI_PROJECT_TARGET_ALL)) {
        lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
        lv_obj_set_name_static(lv_obj_0, "calibration_run_#");
        lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);

        lv_obj_t * column_0 = column_create(lv_obj_0, SPACE_SM, SPACE_SM, 0, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_width(column_0, lv_pct(100));
        lv_obj_set_height(column_0, lv_pct(100));
        lv_obj_t * row_0 = row_create(column_0, 0, 0, 0, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_width(row_0, lv_pct(100));
        lv_obj_t * calibration_run_title = text_create(row_0, "Body text");
        lv_obj_set_name(calibration_run_title, "calibration_run_title");
        lv_label_bind_text(calibration_run_title, &calibration_run_title_text, NULL);
        lv_label_set_long_mode(calibration_run_title, LV_LABEL_LONG_MODE_DOTS);

        lv_obj_t * calibration_current = text_create(row_0, "Body text");
        lv_obj_set_name(calibration_current, "calibration_current");
        lv_label_bind_text(calibration_current, &calibration_current_text, NULL);
        lv_label_set_long_mode(calibration_current, LV_LABEL_LONG_MODE_DOTS);

        lv_obj_t * calibration_graph = lv_chart_create(column_0);
        lv_obj_set_name(calibration_graph, "calibration_graph");
        lv_obj_set_width(calibration_graph, lv_pct(100));
        lv_obj_set_height(calibration_graph, 90);
        lv_chart_set_type(calibration_graph, LV_CHART_TYPE_LINE);
        lv_chart_set_point_count(calibration_graph, 41);
        lv_chart_set_update_mode(calibration_graph, LV_CHART_UPDATE_MODE_SHIFT);
        lv_chart_set_hor_div_line_count(calibration_graph, 3);
        lv_chart_set_ver_div_line_count(calibration_graph, 5);
        lv_chart_series_t * lv_chart_series_0 = lv_chart_add_series(calibration_graph, COLOR_ACCENT, LV_CHART_AXIS_PRIMARY_Y);
        lv_chart_set_axis_min_value(calibration_graph, LV_CHART_AXIS_PRIMARY_Y, 0);
        lv_chart_set_axis_max_value(calibration_graph, LV_CHART_AXIS_PRIMARY_Y, 100);

        lv_obj_t * row_1 = row_create(column_0, 0, 0, 0, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_width(row_1, lv_pct(100));
        lv_obj_t * column_1 = column_create(row_1, 0, 0, 0, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_flex_grow(column_1, 1);
        lv_obj_t * calibration_stability = text_create(column_1, "Body text");
        lv_obj_set_name(calibration_stability, "calibration_stability");
        lv_label_bind_text(calibration_stability, &calibration_stability_text, NULL);
        lv_label_set_long_mode(calibration_stability, LV_LABEL_LONG_MODE_WRAP);
        lv_obj_set_width(calibration_stability, lv_pct(100));

        lv_obj_t * calibration_elapsed = text_create(column_1, "Body text");
        lv_obj_set_name(calibration_elapsed, "calibration_elapsed");
        lv_label_bind_text(calibration_elapsed, &calibration_elapsed_text, NULL);
        lv_label_set_long_mode(calibration_elapsed, LV_LABEL_LONG_MODE_DOTS);

        lv_obj_t * calibration_cancel = button_create(row_1, "Cancel", icon_close, COLOR_ACCENT, COLOR_ACCENT_TEXT, RADIUS_DEFAULT);
        lv_obj_set_name(calibration_cancel, "calibration_cancel");
        lv_obj_set_width(calibration_cancel, 96);

        lv_obj_t * calibration_done = button_create(row_1, "Done", icon_check, COLOR_ACCENT, COLOR_ACCENT_TEXT, RADIUS_DEFAULT);
        lv_obj_set_name(calibration_done, "calibration_done");
        lv_obj_set_width(calibration_done, 96);
        lv_obj_set_flag(calibration_done, LV_OBJ_FLAG_HIDDEN, true);

        the_root = lv_obj_0;
    }
    #endif

    LV_TRACE_OBJ_CREATE("finished");

    return the_root;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

