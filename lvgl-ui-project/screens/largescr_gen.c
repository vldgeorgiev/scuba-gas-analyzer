/**
 * @file largescr_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "largescr_gen.h"
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

lv_obj_t * largescr_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    static bool screen_created = false;
    if (screen_created) {
        LV_LOG_WARN("`largescr` is already initialized as a permanent screen. Returning with no change.");
        return largescr;
    }

    static lv_style_t style_large_percent;
    static lv_style_t style_large_ppm;

    static bool style_inited = false;

    if (!style_inited) {
        /*Init all styles*/
        lv_style_init(&style_large_percent);
        lv_style_init(&style_large_ppm);

        lv_style_set_text_font(&style_large_percent, font_h3);
        lv_style_set_text_font(&style_large_ppm, font_h5);

        style_inited = true;
    }


    lv_obj_t * the_root = NULL;

    #if LVGL_UI_PROJECT_CHECK_COMPILE_TARGET(LVGL_UI_PROJECT_TARGET_ALL)
    if (lvgl_ui_project_check_target(LVGL_UI_PROJECT_TARGET_ALL)) {
        largescr = lv_obj_create(NULL);
        lv_obj_t * lv_obj_0 = largescr;
        lv_obj_set_name_static(lv_obj_0, "largescr_#");
        lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_CLICKABLE, true);
        lv_obj_set_style_pad_all(lv_obj_0, 0, 0);

        lv_obj_add_screen_load_event(lv_obj_0, LV_EVENT_CLICKED, mainscr, LV_SCREEN_LOAD_ANIM_NONE, 0, 0);
        lv_obj_t * row_0 = row_create(lv_obj_0, SPACE_SM, 0, 0, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_width(row_0, lv_pct(100));
        lv_obj_set_height(row_0, lv_pct(100));
        lv_obj_set_flag(row_0, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_obj_t * large_gases = column_create(row_0, 0, 0, 1, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START);
        lv_obj_set_name(large_gases, "large_gases");
        lv_obj_set_height(large_gases, lv_pct(100));
        lv_obj_set_flag(large_gases, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_obj_t * large_o2_row = row_create(large_gases, 0, 0, 0, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_name(large_o2_row, "large_o2_row");
        lv_obj_set_width(large_o2_row, lv_pct(100));
        lv_obj_set_flag(large_o2_row, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_obj_t * h3_0 = h3_create(large_o2_row, "O2");
        lv_obj_set_width(h3_0, 58);

        lv_obj_t * large_o2_value = lv_spangroup_create(large_o2_row);
        lv_obj_set_name(large_o2_value, "large_o2_value");
        lv_obj_set_flex_grow(large_o2_value, 1);
        lv_obj_set_height(large_o2_value, LV_SIZE_CONTENT);
        lv_spangroup_set_overflow(large_o2_value, LV_SPAN_OVERFLOW_ELLIPSIS);
        lv_spangroup_set_max_lines(large_o2_value, 1);
        lv_obj_set_style_text_font(large_o2_value, font_h1, 0);
        lv_obj_set_flag(large_o2_value, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_span_t * lv_spangroup_span_0 = lv_spangroup_add_span(large_o2_value);
        lv_spangroup_bind_span_text(large_o2_value, lv_spangroup_span_0, &main_o2_text, NULL);
        lv_span_t * lv_spangroup_span_1 = lv_spangroup_add_span(large_o2_value);
        lv_spangroup_set_span_text(large_o2_value, lv_spangroup_span_1, "%");
        lv_spangroup_set_span_style(large_o2_value, lv_spangroup_span_1, &style_large_percent);

        lv_obj_t * large_he_row = row_create(large_gases, 0, 0, 0, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_name(large_he_row, "large_he_row");
        lv_obj_set_width(large_he_row, lv_pct(100));
        lv_obj_set_flag(large_he_row, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_obj_t * h3_1 = h3_create(large_he_row, "He");
        lv_obj_set_width(h3_1, 58);

        lv_obj_t * large_he_value = lv_spangroup_create(large_he_row);
        lv_obj_set_name(large_he_value, "large_he_value");
        lv_obj_set_flex_grow(large_he_value, 1);
        lv_obj_set_height(large_he_value, LV_SIZE_CONTENT);
        lv_spangroup_set_overflow(large_he_value, LV_SPAN_OVERFLOW_ELLIPSIS);
        lv_spangroup_set_max_lines(large_he_value, 1);
        lv_obj_set_style_text_font(large_he_value, font_h1, 0);
        lv_obj_set_flag(large_he_value, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_span_t * lv_spangroup_span_2 = lv_spangroup_add_span(large_he_value);
        lv_spangroup_bind_span_text(large_he_value, lv_spangroup_span_2, &main_he_text, NULL);
        lv_span_t * lv_spangroup_span_3 = lv_spangroup_add_span(large_he_value);
        lv_spangroup_set_span_text(large_he_value, lv_spangroup_span_3, "%");
        lv_spangroup_set_span_style(large_he_value, lv_spangroup_span_3, &style_large_percent);

        lv_obj_t * large_status = column_create(row_0, 0, SPACE_SM, 0, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_name(large_status, "large_status");
        lv_obj_set_width(large_status, 72);
        lv_obj_set_height(large_status, lv_pct(100));
        lv_obj_set_flag(large_status, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_obj_t * large_co_title = h4_create(large_status, "CO", "");
        lv_obj_set_name(large_co_title, "large_co_title");

        lv_obj_t * large_co_value = lv_spangroup_create(large_status);
        lv_obj_set_name(large_co_value, "large_co_value");
        lv_obj_set_width(large_co_value, lv_pct(100));
        lv_obj_set_height(large_co_value, LV_SIZE_CONTENT);
        lv_spangroup_set_overflow(large_co_value, LV_SPAN_OVERFLOW_ELLIPSIS);
        lv_spangroup_set_max_lines(large_co_value, 1);
        lv_obj_set_style_text_align(large_co_value, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(large_co_value, font_h4, 0);
        lv_obj_set_flag(large_co_value, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_span_t * lv_spangroup_span_4 = lv_spangroup_add_span(large_co_value);
        lv_spangroup_bind_span_text(large_co_value, lv_spangroup_span_4, &main_co_text, NULL);

        lv_obj_t * large_co_unit = h5_create(large_status, "ppm", "");
        lv_obj_set_name(large_co_unit, "large_co_unit");

        the_root = lv_obj_0;
    }
    #endif

    if (the_root) screen_created = true;

    LV_TRACE_OBJ_CREATE("finished");

    return the_root;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

