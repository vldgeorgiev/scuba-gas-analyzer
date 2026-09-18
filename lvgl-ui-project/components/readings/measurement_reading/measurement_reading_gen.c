/**
 * @file measurement_reading_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "measurement_reading_gen.h"
#include "../../../lvgl_ui_project.h"

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

lv_obj_t * measurement_reading_create(lv_obj_t * parent, const char * label, lv_subject_t * subject, const char * unit, lv_subject_t * secondary_subject, int32_t grow)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_primary_unit;

    static bool style_inited = false;

    if (!style_inited) {
        /*Init all styles*/
        lv_style_init(&style_primary_unit);

        lv_style_set_text_font(&style_primary_unit, font_h5);

        style_inited = true;
    }


    lv_obj_t * the_root = NULL;

    #if LVGL_UI_PROJECT_CHECK_COMPILE_TARGET(LVGL_UI_PROJECT_TARGET_ALL)
    if (lvgl_ui_project_check_target(LVGL_UI_PROJECT_TARGET_ALL)) {
        lv_obj_t * column_0 = column_create(parent, 0, SPACE_SM, grow, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_name_static(column_0, "measurement_reading_#");
        lv_obj_set_width(column_0, 0);
        lv_obj_set_height(column_0, lv_pct(100));

        h5_create(column_0, label, "");

        lv_obj_t * primary_value = lv_spangroup_create(column_0);
        lv_obj_set_flag(primary_value, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_obj_set_name(primary_value, "primary_value");
        lv_obj_set_width(primary_value, lv_pct(100));
        lv_obj_set_height(primary_value, LV_SIZE_CONTENT);
        lv_spangroup_set_overflow(primary_value, LV_SPAN_OVERFLOW_ELLIPSIS);
        lv_spangroup_set_max_lines(primary_value, 1);
        lv_obj_set_style_text_align(primary_value, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(primary_value, font_h2, 0);
        lv_span_t * lv_spangroup_span_0 = lv_spangroup_add_span(primary_value);
        lv_spangroup_bind_span_text(primary_value, lv_spangroup_span_0, subject, NULL);
        lv_span_t * lv_spangroup_span_1 = lv_spangroup_add_span(primary_value);
        lv_spangroup_set_span_text(primary_value, lv_spangroup_span_1, unit);
        lv_spangroup_set_span_style(primary_value, lv_spangroup_span_1, &style_primary_unit);

        lv_obj_t * lv_label_0 = lv_label_create(column_0);
        lv_obj_set_width(lv_label_0, lv_pct(100));
        lv_label_set_long_mode(lv_label_0, LV_LABEL_LONG_MODE_DOTS);
        lv_label_bind_text(lv_label_0, secondary_subject, NULL);
        lv_obj_set_style_text_align(lv_label_0, LV_TEXT_ALIGN_CENTER, 0);

        the_root = column_0;
    }
    #endif

    LV_TRACE_OBJ_CREATE("finished");

    return the_root;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

