/**
 * @file mainscr_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "mainscr_gen.h"
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

lv_obj_t * mainscr_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    static bool screen_created = false;
    if (screen_created) {
        LV_LOG_WARN("`mainscr` is already initialized as a permanent screen. Returning with no change.");
        return mainscr;
    }


    lv_obj_t * the_root = NULL;

    #if LVGL_UI_PROJECT_CHECK_COMPILE_TARGET(LVGL_UI_PROJECT_TARGET_ALL)
    if (lvgl_ui_project_check_target(LVGL_UI_PROJECT_TARGET_ALL)) {
        mainscr = lv_obj_create(NULL);
        lv_obj_t * lv_obj_0 = mainscr;
        lv_obj_set_name_static(lv_obj_0, "mainscr_#");
        lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_CLICKABLE, true);

        lv_obj_add_screen_create_event(lv_obj_0, LV_EVENT_CLICKED, largescr_create, LV_SCREEN_LOAD_ANIM_NONE, 0, 0);
        lv_obj_t * column_0 = column_create(lv_obj_0, SPACE_XS, SPACE_XS, 0, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_width(column_0, lv_pct(100));
        lv_obj_set_height(column_0, lv_pct(100));
        lv_obj_set_flag(column_0, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_obj_t * row_0 = row_create(column_0, 0, 0, 0, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_width(row_0, lv_pct(100));
        lv_obj_set_flag(row_0, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_obj_t * logo_image = image_create(row_0, phony_divers_logo);
        lv_obj_set_name(logo_image, "logo_image");

        lv_obj_t * row_1 = row_create(row_0, 0, SPACE_SM, 0, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_flag(row_1, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_obj_t * row_2 = row_create(row_1, 0, SPACE_XS, 0, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_flag(row_2, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        monoicon_create(row_2, icon_battery);

        lv_obj_t * lv_label_0 = lv_label_create(row_2);
        lv_label_bind_text(lv_label_0, &main_battery_text, NULL);

        lv_obj_t * open_diagnostics = button_create(row_1, "", icon_bell, COLOR_DANGER, COLOR_ACCENT_TEXT, RADIUS_DEFAULT);
        lv_obj_set_name(open_diagnostics, "open_diagnostics");
        lv_obj_set_flag(open_diagnostics, LV_OBJ_FLAG_HIDDEN, true);
        lv_obj_set_flag(open_diagnostics, LV_OBJ_FLAG_EVENT_BUBBLE, false);

        lv_obj_t * open_calibration = button_create(row_1, "", icon_refresh, COLOR_ACCENT, COLOR_ACCENT_TEXT, RADIUS_DEFAULT);
        lv_obj_set_name(open_calibration, "open_calibration");
        lv_obj_set_flag(open_calibration, LV_OBJ_FLAG_EVENT_BUBBLE, false);

        lv_obj_t * open_settings = button_create(row_1, "", icon_settings, COLOR_ACCENT, COLOR_ACCENT_TEXT, RADIUS_DEFAULT);
        lv_obj_set_name(open_settings, "open_settings");
        lv_obj_set_flag(open_settings, LV_OBJ_FLAG_EVENT_BUBBLE, false);
        lv_obj_add_screen_create_event(open_settings, LV_EVENT_CLICKED, settings_create, LV_SCREEN_LOAD_ANIM_NONE, 0, 0);

        lv_obj_t * readings = row_create(column_0, 0, 0, 1, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_name(readings, "readings");
        lv_obj_set_width(readings, lv_pct(100));
        lv_obj_set_flag(readings, LV_OBJ_FLAG_CLICKABLE, true);
        lv_obj_set_flag(readings, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_obj_t * main_o2_reading = measurement_reading_create(readings, "O2", &main_o2_text, "%", &main_o2_mv_text, 12);
        lv_obj_set_name(main_o2_reading, "main_o2_reading");
        lv_obj_set_flag(main_o2_reading, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_obj_t * row_3 = row_create(main_o2_reading, 0, 0, 0, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_scrollbar_mode(row_3, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_width(row_3, lv_pct(100));
        lv_obj_t * text_0 = text_create(row_3, "Body text");
        lv_label_bind_text(text_0, &main_mod_bottom_text, NULL);
        lv_label_set_long_mode(text_0, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_style_text_align(text_0, LV_TEXT_ALIGN_CENTER, 0);

        text_create(row_3, ", ");

        lv_obj_t * text_2 = text_create(row_3, "Body text");
        lv_label_bind_text(text_2, &main_mod_deco_text, NULL);
        lv_label_set_long_mode(text_2, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_style_text_align(text_2, LV_TEXT_ALIGN_CENTER, 0);

        lv_obj_t * main_he_reading = measurement_reading_create(readings, "He", &main_he_text, "%", &main_he_mv_text, 10);
        lv_obj_set_name(main_he_reading, "main_he_reading");
        lv_obj_set_flag(main_he_reading, LV_OBJ_FLAG_EVENT_BUBBLE, true);
        lv_obj_t * he_temperature = lv_label_create(main_he_reading);
        lv_obj_set_name(he_temperature, "he_temperature");
        lv_obj_set_width(he_temperature, lv_pct(100));
        lv_label_set_long_mode(he_temperature, LV_LABEL_LONG_MODE_DOTS);
        lv_label_bind_text(he_temperature, &main_he_temperature_text, NULL);
        lv_obj_set_style_text_align(he_temperature, LV_TEXT_ALIGN_CENTER, 0);

        lv_obj_t * main_co_reading = measurement_reading_create(readings, "CO", &main_co_text, "ppm", &main_co_mv_text, 11);
        lv_obj_set_name(main_co_reading, "main_co_reading");
        lv_obj_set_flag(main_co_reading, LV_OBJ_FLAG_EVENT_BUBBLE, true);

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

