/**
 * @file firmware_update_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "firmware_update_gen.h"
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

lv_obj_t * firmware_update_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");


    lv_obj_t * the_root = NULL;

    #if LVGL_UI_PROJECT_CHECK_COMPILE_TARGET(LVGL_UI_PROJECT_TARGET_ALL)
    if (lvgl_ui_project_check_target(LVGL_UI_PROJECT_TARGET_ALL)) {
        lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
        lv_obj_set_name_static(lv_obj_0, "firmware_update_#");
        lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);

        lv_obj_add_subject_set_int_event(lv_obj_0, &update_keyboard_visible, LV_EVENT_SCREEN_LOADED, 0);
        lv_obj_t * column_0 = column_create(lv_obj_0, SPACE_SM, SPACE_SM, 0, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_width(column_0, lv_pct(100));
        lv_obj_set_height(column_0, lv_pct(100));
        lv_obj_t * row_0 = row_create(column_0, 0, 0, 1, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_width(row_0, lv_pct(100));
        lv_obj_t * update_back = button_create(row_0, "", icon_arrow_left, COLOR_ACCENT, COLOR_ACCENT_TEXT, RADIUS_DEFAULT);
        lv_obj_set_name(update_back, "update_back");

        lv_obj_t * update_form = column_create(row_0, SPACE_MD, SPACE_MD, 1, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_name(update_form, "update_form");
        lv_obj_set_height(update_form, lv_pct(100));
        lv_obj_set_flag(update_form, LV_OBJ_FLAG_SCROLLABLE, true);
        lv_obj_t * row_1 = row_create(update_form, 0, SPACE_SM, 0, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_width(row_1, lv_pct(100));
        lv_obj_bind_flag_if_eq(row_1, &update_keyboard_visible, LV_OBJ_FLAG_HIDDEN, 1);
        lv_obj_t * wifi_names = dropdown_create(row_1, "No networks", 0, &update_network_index);
        lv_obj_set_name(wifi_names, "wifi_names");
        lv_obj_set_width(wifi_names, 0);
        lv_obj_set_flex_grow(wifi_names, 1);

        lv_obj_t * scan_wifi = button_create(row_1, "", icon_refresh, COLOR_ACCENT, COLOR_ACCENT_TEXT, RADIUS_DEFAULT);
        lv_obj_set_name(scan_wifi, "scan_wifi");

        lv_obj_t * wifi_password = text_input_create(update_form, "", "Wi-Fi password", false);
        lv_obj_set_name(wifi_password, "wifi_password");
        lv_obj_set_width(wifi_password, lv_pct(100));
        lv_obj_add_subject_set_int_event(wifi_password, &update_keyboard_visible, LV_EVENT_CLICKED, 1);

        lv_obj_t * update_status = text_create(update_form, "Body text");
        lv_obj_set_name(update_status, "update_status");
        lv_label_bind_text(update_status, &update_status_text, NULL);
        lv_obj_set_width(update_status, lv_pct(100));
        lv_label_set_long_mode(update_status, LV_LABEL_LONG_MODE_WRAP);
        lv_obj_bind_flag_if_eq(update_status, &update_keyboard_visible, LV_OBJ_FLAG_HIDDEN, 1);

        lv_obj_t * install_firmware = button_create(update_form, "Install firmware", icon_download, COLOR_ACCENT, COLOR_ACCENT_TEXT, RADIUS_DEFAULT);
        lv_obj_set_name(install_firmware, "install_firmware");
        lv_obj_set_width(install_firmware, lv_pct(100));
        lv_obj_bind_flag_if_eq(install_firmware, &update_keyboard_visible, LV_OBJ_FLAG_HIDDEN, 1);
        lv_obj_bind_state_if_eq(install_firmware, &update_can_install, LV_STATE_DISABLED, 0);

        lv_obj_t * firmware_version = text_create(update_form, "Body text");
        lv_obj_set_name(firmware_version, "firmware_version");
        lv_label_bind_text(firmware_version, &firmware_version_text, NULL);
        lv_obj_set_width(firmware_version, lv_pct(100));
        lv_obj_bind_flag_if_eq(firmware_version, &update_keyboard_visible, LV_OBJ_FLAG_HIDDEN, 1);

        lv_obj_t * update_keyboard = keyboard_create(column_0, wifi_password, LV_KEYBOARD_MODE_TEXT_LOWER);
        lv_obj_set_name(update_keyboard, "update_keyboard");
        lv_obj_set_width(update_keyboard, lv_pct(100));
        lv_obj_set_height(update_keyboard, 120);
        lv_obj_bind_flag_if_eq(update_keyboard, &update_keyboard_visible, LV_OBJ_FLAG_HIDDEN, 0);
        lv_obj_add_subject_set_int_event(update_keyboard, &update_keyboard_visible, LV_EVENT_READY, 0);
        lv_obj_add_subject_set_int_event(update_keyboard, &update_keyboard_visible, LV_EVENT_CANCEL, 0);

        the_root = lv_obj_0;
    }
    #endif

    LV_TRACE_OBJ_CREATE("finished");

    return the_root;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

