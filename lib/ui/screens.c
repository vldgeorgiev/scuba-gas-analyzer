#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;
lv_obj_t *tick_value_change_obj;

static void event_handler_cb_main_main(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 1, 0, e);
    }
}

static void event_handler_cb_main_warning_indicator(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            assignBooleanProperty(flowState, 4, 4, value, "Failed to assign Checked state");
        }
    }
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 4, 0, e);
    }
}

static void event_handler_cb_main_btn_config(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 5, 0, e);
    }
}

static void event_handler_cb_main_co_pnl(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            assignBooleanProperty(flowState, 12, 4, value, "Failed to assign Checked state");
        }
    }
}

static void event_handler_cb_large_large(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 1, 0, e);
    }
}

static void event_handler_cb_config_btn_home(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 0, 0, e);
    }
}

static void event_handler_cb_config_o2_enabled(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            assignBooleanProperty(flowState, 5, 3, value, "Failed to assign Checked state");
        }
    }
}

static void event_handler_cb_config_calibrate_air(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_RELEASED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 7, 0, e);
    }
}

static void event_handler_cb_config_calibrate_100_(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_RELEASED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 9, 0, e);
    }
}

static void event_handler_cb_config_obj0(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_dropdown_get_selected(ta);
            if (tick_value_change_obj != ta) {
                assignIntegerProperty(flowState, 11, 4, value, "Failed to assign Selected in Dropdown widget");
            }
        }
    }
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 11, 0, e);
    }
}

static void event_handler_cb_config_obj1(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_dropdown_get_selected(ta);
            if (tick_value_change_obj != ta) {
                assignIntegerProperty(flowState, 12, 4, value, "Failed to assign Selected in Dropdown widget");
            }
        }
    }
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 12, 0, e);
    }
}

static void event_handler_cb_config_co_enabled(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            assignBooleanProperty(flowState, 17, 3, value, "Failed to assign Checked state");
        }
    }
}

static void event_handler_cb_config_he_enabled(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            assignBooleanProperty(flowState, 20, 3, value, "Failed to assign Checked state");
        }
    }
}

static void event_handler_cb_config_calibrate_100__1(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_RELEASED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 21, 0, e);
    }
}

static void event_handler_cb_config_obj2(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            assignBooleanProperty(flowState, 23, 3, value, "Failed to assign Checked state");
        }
    }
}

static void event_handler_cb_config_brightness(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_slider_get_value(ta);
            if (tick_value_change_obj != ta) {
                assignIntegerProperty(flowState, 25, 3, value, "Failed to assign Value in Slider widget");
            }
        }
    }
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 25, 0, e);
    }
}

static void event_handler_cb_log_obj3(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            const char *value = lv_textarea_get_text(ta);
            if (tick_value_change_obj != ta) {
                assignStringProperty(flowState, 0, 3, value, "Failed to assign Text in Textarea widget");
            }
        }
    }
}

static void event_handler_cb_log_btn_home_1(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 2, 0, e);
    }
}

void create_screen_main() {
    void *flowState = getFlowState(0, 0);
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 170);
    lv_obj_add_event_cb(obj, event_handler_cb_main_main, LV_EVENT_ALL, flowState);
    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN_WRAP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // Header
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.header = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 50, LV_PCT(100));
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            add_style_panel_style(obj);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_img_set_src(obj, &img_pishman_logo);
                    lv_img_set_pivot(obj, 0, 0);
                }
                {
                    // battIndicator
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.batt_indicator = obj;
                    lv_obj_set_pos(obj, 5, 55);
                    lv_obj_set_size(obj, LV_PCT(90), 30);
                    lv_label_set_text(obj, "");
                    add_style_label_style(obj);
                }
                {
                    // warningIndicator
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.warning_indicator = obj;
                    lv_obj_set_pos(obj, 7, 90);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "");
                    lv_obj_add_event_cb(obj, event_handler_cb_main_warning_indicator, LV_EVENT_ALL, flowState);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    add_style_label_style(obj);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffc000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffff6d6d), LV_PART_MAIN | LV_STATE_CHECKED);
                }
                {
                    // btnConfig
                    lv_obj_t *obj = lv_btn_create(parent_obj);
                    objects.btn_config = obj;
                    lv_obj_set_pos(obj, 5, 130);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 26);
                    lv_obj_add_event_cb(obj, event_handler_cb_main_btn_config, LV_EVENT_ALL, flowState);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_IGNORE_LAYOUT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj4 = obj;
                            lv_obj_set_pos(obj, 0, -2);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text(obj, "");
                            add_style_label_style(obj);
                        }
                    }
                }
            }
        }
        {
            // o2Pnl
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.o2_pnl = obj;
            lv_obj_set_pos(obj, 12, 55);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
            add_style_panel_style(obj);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffa4ffa4), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // O2
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.o2 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "");
                    add_style_label_style(obj);
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 20);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "MOD");
                    add_style_label_style(obj);
                    lv_obj_set_style_pad_top(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj5 = obj;
                    lv_obj_set_pos(obj, 10, 40);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "");
                    add_style_label_style(obj);
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj6 = obj;
                    lv_obj_set_pos(obj, 10, 60);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "");
                    add_style_label_style(obj);
                }
            }
        }
        {
            // coPnl
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.co_pnl = obj;
            lv_obj_set_pos(obj, 12, 160);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_add_event_cb(obj, event_handler_cb_main_co_pnl, LV_EVENT_ALL, flowState);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
            add_style_panel_style(obj);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffff6d6d), LV_PART_MAIN | LV_STATE_CHECKED);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // CO
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.co = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "");
                    add_style_label_style(obj);
                }
            }
        }
        {
            // hePnl
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.he_pnl = obj;
            lv_obj_set_pos(obj, 12, 160);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
            add_style_panel_style(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // HE
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.he = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "");
                    add_style_label_style(obj);
                }
            }
        }
    }
}

void tick_screen_main() {
    void *flowState = getFlowState(0, 0);
    {
        const char *new_val = evalTextProperty(flowState, 3, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.batt_indicator);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.batt_indicator;
            lv_label_set_text(objects.batt_indicator, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 4, 4, "Failed to evaluate Checked state");
        bool cur_val = lv_obj_has_state(objects.warning_indicator, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.warning_indicator;
            if (new_val) lv_obj_add_state(objects.warning_indicator, LV_STATE_CHECKED);
            else lv_obj_clear_state(objects.warning_indicator, LV_STATE_CHECKED);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 4, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.warning_indicator, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.warning_indicator;
            if (new_val) lv_obj_add_flag(objects.warning_indicator, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.warning_indicator, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = evalTextProperty(flowState, 4, 5, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.warning_indicator);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.warning_indicator;
            lv_label_set_text(objects.warning_indicator, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = evalTextProperty(flowState, 6, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.obj4);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj4;
            lv_label_set_text(objects.obj4, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 7, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.o2_pnl, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.o2_pnl;
            if (new_val) lv_obj_add_flag(objects.o2_pnl, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.o2_pnl, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = evalTextProperty(flowState, 8, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.o2);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.o2;
            lv_label_set_text(objects.o2, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = evalTextProperty(flowState, 10, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.obj5);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj5;
            lv_label_set_text(objects.obj5, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = evalTextProperty(flowState, 11, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.obj6);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj6;
            lv_label_set_text(objects.obj6, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 12, 4, "Failed to evaluate Checked state");
        bool cur_val = lv_obj_has_state(objects.co_pnl, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.co_pnl;
            if (new_val) lv_obj_add_state(objects.co_pnl, LV_STATE_CHECKED);
            else lv_obj_clear_state(objects.co_pnl, LV_STATE_CHECKED);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 12, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.co_pnl, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.co_pnl;
            if (new_val) lv_obj_add_flag(objects.co_pnl, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.co_pnl, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = evalTextProperty(flowState, 13, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.co);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.co;
            lv_label_set_text(objects.co, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 14, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.he_pnl, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.he_pnl;
            if (new_val) lv_obj_add_flag(objects.he_pnl, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.he_pnl, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = evalTextProperty(flowState, 15, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.he);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.he;
            lv_label_set_text(objects.he, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_large() {
    void *flowState = getFlowState(0, 1);
    lv_obj_t *obj = lv_obj_create(0);
    objects.large = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 170);
    lv_obj_add_event_cb(obj, event_handler_cb_large_large, LV_EVENT_ALL, flowState);
    {
        lv_obj_t *parent_obj = obj;
        {
            // largePanel
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.large_panel = obj;
            lv_obj_set_pos(obj, 16, 9);
            lv_obj_set_size(obj, LV_PCT(90), LV_PCT(90));
            lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            add_style_panel_style(obj);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffa4ffa4), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // O2_1
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.o2_1 = obj;
                    lv_obj_set_pos(obj, 0, 10);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "");
                    add_style_label_style(obj);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj7 = obj;
                    lv_obj_set_pos(obj, 0, 70);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "");
                    add_style_label_style(obj);
                    lv_obj_set_style_pad_top(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
    }
}

void tick_screen_large() {
    void *flowState = getFlowState(0, 1);
    {
        bool new_val = evalBooleanProperty(flowState, 0, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.large_panel, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.large_panel;
            if (new_val) lv_obj_add_flag(objects.large_panel, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.large_panel, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = evalTextProperty(flowState, 2, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.o2_1);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.o2_1;
            lv_label_set_text(objects.o2_1, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = evalTextProperty(flowState, 3, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.obj7);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj7;
            lv_label_set_text(objects.obj7, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_config() {
    void *flowState = getFlowState(0, 2);
    lv_obj_t *obj = lv_obj_create(0);
    objects.config = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 640, 170);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN_WRAP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        static lv_coord_t dsc[] = {LV_GRID_TEMPLATE_LAST};
        lv_obj_set_style_grid_row_dsc_array(obj, dsc, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        static lv_coord_t dsc[] = {LV_GRID_TEMPLATE_LAST};
        lv_obj_set_style_grid_column_dsc_array(obj, dsc, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        lv_obj_t *parent_obj = obj;
        {
            // btnHome
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.btn_home = obj;
            lv_obj_set_pos(obj, 12, 282);
            lv_obj_set_size(obj, 40, 30);
            lv_obj_add_event_cb(obj, event_handler_cb_config_btn_home, LV_EVENT_ALL, flowState);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj8 = obj;
                    lv_obj_set_pos(obj, -1, -1);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "");
                    add_style_label_style(obj);
                }
            }
        }
        {
            // O2ConfPnl
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.o2_conf_pnl = obj;
            lv_obj_set_pos(obj, 12, 55);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            add_style_panel_style(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "O2");
                    add_style_label_style(obj);
                }
                {
                    // o2 enabled
                    lv_obj_t *obj = lv_switch_create(parent_obj);
                    objects.o2_enabled = obj;
                    lv_obj_set_pos(obj, 0, 22);
                    lv_obj_set_size(obj, 50, 25);
                    lv_obj_add_event_cb(obj, event_handler_cb_config_o2_enabled, LV_EVENT_ALL, flowState);
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 65, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "Calibrate");
                    add_style_label_style(obj);
                }
                {
                    // Calibrate Air
                    lv_obj_t *obj = lv_btn_create(parent_obj);
                    objects.calibrate_air = obj;
                    lv_obj_set_pos(obj, 65, 22);
                    lv_obj_set_size(obj, 52, 24);
                    lv_obj_add_event_cb(obj, event_handler_cb_config_calibrate_air, LV_EVENT_ALL, flowState);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 3, -4);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text(obj, "Air");
                            add_style_label_style(obj);
                        }
                    }
                }
                {
                    // Calibrate 100%
                    lv_obj_t *obj = lv_btn_create(parent_obj);
                    objects.calibrate_100_ = obj;
                    lv_obj_set_pos(obj, 120, 22);
                    lv_obj_set_size(obj, 52, 24);
                    lv_obj_add_event_cb(obj, event_handler_cb_config_calibrate_100_, LV_EVENT_ALL, flowState);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, -4, -4);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text(obj, "100%");
                            add_style_label_style(obj);
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_dropdown_create(parent_obj);
                    objects.obj0 = obj;
                    lv_obj_set_pos(obj, 100, 60);
                    lv_obj_set_size(obj, 67, 30);
                    lv_dropdown_set_options(obj, "");
                    lv_dropdown_set_dir(obj, LV_DIR_LEFT);
                    lv_obj_add_event_cb(obj, event_handler_cb_config_obj0, LV_EVENT_ALL, flowState);
                }
                {
                    lv_obj_t *obj = lv_dropdown_create(parent_obj);
                    objects.obj1 = obj;
                    lv_obj_set_pos(obj, 100, 90);
                    lv_obj_set_size(obj, 67, 30);
                    lv_dropdown_set_options(obj, "");
                    lv_dropdown_set_dir(obj, LV_DIR_LEFT);
                    lv_obj_add_event_cb(obj, event_handler_cb_config_obj1, LV_EVENT_ALL, flowState);
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 95);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "Deco pO2");
                    add_style_label_style(obj);
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 65);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "Bottom pO2");
                    add_style_label_style(obj);
                }
            }
        }
        {
            // CoConfPnl
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.co_conf_pnl = obj;
            lv_obj_set_pos(obj, 12, 134);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_panel_style(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "CO");
                    add_style_label_style(obj);
                }
                {
                    // CO enabled
                    lv_obj_t *obj = lv_switch_create(parent_obj);
                    objects.co_enabled = obj;
                    lv_obj_set_pos(obj, 0, 22);
                    lv_obj_set_size(obj, 50, 25);
                    lv_obj_add_event_cb(obj, event_handler_cb_config_co_enabled, LV_EVENT_ALL, flowState);
                }
            }
        }
        {
            // HeConfPnl
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.he_conf_pnl = obj;
            lv_obj_set_pos(obj, 12, 134);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_panel_style(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "He");
                    add_style_label_style(obj);
                }
                {
                    // He enabled
                    lv_obj_t *obj = lv_switch_create(parent_obj);
                    objects.he_enabled = obj;
                    lv_obj_set_pos(obj, 0, 22);
                    lv_obj_set_size(obj, 50, 25);
                    lv_obj_add_event_cb(obj, event_handler_cb_config_he_enabled, LV_EVENT_ALL, flowState);
                }
                {
                    // Calibrate 100%_1
                    lv_obj_t *obj = lv_btn_create(parent_obj);
                    objects.calibrate_100__1 = obj;
                    lv_obj_set_pos(obj, 0, 55);
                    lv_obj_set_size(obj, 52, 24);
                    lv_obj_add_event_cb(obj, event_handler_cb_config_calibrate_100__1, LV_EVENT_ALL, flowState);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, -4, -4);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text(obj, "100%");
                            add_style_label_style(obj);
                        }
                    }
                }
            }
        }
        {
            lv_obj_t *obj = lv_checkbox_create(parent_obj);
            objects.obj2 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_checkbox_set_text(obj, "Calibrate on start");
            lv_obj_add_event_cb(obj, event_handler_cb_config_obj2, LV_EVENT_ALL, flowState);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 0, -20);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "Brightness");
            add_style_label_style(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // Brightness
                    lv_obj_t *obj = lv_slider_create(parent_obj);
                    objects.brightness = obj;
                    lv_obj_set_pos(obj, 0, 22);
                    lv_obj_set_size(obj, 150, 10);
                    lv_slider_set_range(obj, 0, 254);
                    lv_obj_add_event_cb(obj, event_handler_cb_config_brightness, LV_EVENT_ALL, flowState);
                    lv_obj_set_style_margin_bottom(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
        }
    }
}

void tick_screen_config() {
    void *flowState = getFlowState(0, 2);
    {
        const char *new_val = evalTextProperty(flowState, 2, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.obj8);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj8;
            lv_label_set_text(objects.obj8, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 5, 3, "Failed to evaluate Checked state");
        bool cur_val = lv_obj_has_state(objects.o2_enabled, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.o2_enabled;
            if (new_val) lv_obj_add_state(objects.o2_enabled, LV_STATE_CHECKED);
            else lv_obj_clear_state(objects.o2_enabled, LV_STATE_CHECKED);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = evalStringArrayPropertyAndJoin(flowState, 11, 3, "Failed to evaluate Options in Dropdown widget", "\n");
        const char *cur_val = lv_dropdown_get_options(objects.obj0);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj0;
            lv_dropdown_set_options(objects.obj0, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        if (!(lv_obj_get_state(objects.obj0) & LV_STATE_EDITED)) {
            int32_t new_val = evalIntegerProperty(flowState, 11, 4, "Failed to evaluate Selected in Dropdown widget");
            int32_t cur_val = lv_dropdown_get_selected(objects.obj0);
            if (new_val != cur_val) {
                tick_value_change_obj = objects.obj0;
                lv_dropdown_set_selected(objects.obj0, new_val);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        const char *new_val = evalStringArrayPropertyAndJoin(flowState, 12, 3, "Failed to evaluate Options in Dropdown widget", "\n");
        const char *cur_val = lv_dropdown_get_options(objects.obj1);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj1;
            lv_dropdown_set_options(objects.obj1, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        if (!(lv_obj_get_state(objects.obj1) & LV_STATE_EDITED)) {
            int32_t new_val = evalIntegerProperty(flowState, 12, 4, "Failed to evaluate Selected in Dropdown widget");
            int32_t cur_val = lv_dropdown_get_selected(objects.obj1);
            if (new_val != cur_val) {
                tick_value_change_obj = objects.obj1;
                lv_dropdown_set_selected(objects.obj1, new_val);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 17, 3, "Failed to evaluate Checked state");
        bool cur_val = lv_obj_has_state(objects.co_enabled, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.co_enabled;
            if (new_val) lv_obj_add_state(objects.co_enabled, LV_STATE_CHECKED);
            else lv_obj_clear_state(objects.co_enabled, LV_STATE_CHECKED);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 20, 3, "Failed to evaluate Checked state");
        bool cur_val = lv_obj_has_state(objects.he_enabled, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.he_enabled;
            if (new_val) lv_obj_add_state(objects.he_enabled, LV_STATE_CHECKED);
            else lv_obj_clear_state(objects.he_enabled, LV_STATE_CHECKED);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 23, 3, "Failed to evaluate Checked state");
        bool cur_val = lv_obj_has_state(objects.obj2, LV_STATE_CHECKED);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj2;
            if (new_val) lv_obj_add_state(objects.obj2, LV_STATE_CHECKED);
            else lv_obj_clear_state(objects.obj2, LV_STATE_CHECKED);
            tick_value_change_obj = NULL;
        }
    }
    {
        int32_t new_val = evalIntegerProperty(flowState, 25, 3, "Failed to evaluate Value in Slider widget");
        int32_t cur_val = lv_slider_get_value(objects.brightness);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.brightness;
            lv_slider_set_value(objects.brightness, new_val, LV_ANIM_OFF);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_log() {
    void *flowState = getFlowState(0, 3);
    lv_obj_t *obj = lv_obj_create(0);
    objects.log = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 170);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_textarea_create(parent_obj);
            objects.obj3 = obj;
            lv_obj_set_pos(obj, 40, 0);
            lv_obj_set_size(obj, 280, LV_PCT(100));
            lv_textarea_set_max_length(obj, 1000);
            lv_textarea_set_one_line(obj, false);
            lv_textarea_set_password_mode(obj, false);
            lv_obj_add_event_cb(obj, event_handler_cb_log_obj3, LV_EVENT_ALL, flowState);
        }
        {
            // btnHome_1
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.btn_home_1 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 40, 30);
            lv_obj_add_event_cb(obj, event_handler_cb_log_btn_home_1, LV_EVENT_ALL, flowState);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj9 = obj;
                    lv_obj_set_pos(obj, -1, -1);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, "");
                    add_style_label_style(obj);
                }
            }
        }
    }
}

void tick_screen_log() {
    void *flowState = getFlowState(0, 3);
    {
        const char *new_val = evalTextProperty(flowState, 0, 3, "Failed to evaluate Text in Textarea widget");
        const char *cur_val = lv_textarea_get_text(objects.obj3);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj3;
            lv_textarea_set_text(objects.obj3, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = evalTextProperty(flowState, 3, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.obj9);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj9;
            lv_label_set_text(objects.obj9, new_val);
            tick_value_change_obj = NULL;
        }
    }
}


extern void add_style(lv_obj_t *obj, int32_t styleIndex);
extern void remove_style(lv_obj_t *obj, int32_t styleIndex);

static const char *screen_names[] = { "Main", "Large", "Config", "Log" };
static const char *object_names[] = { "main", "large", "config", "log", "warning_indicator", "btn_config", "co_pnl", "btn_home", "o2_enabled", "calibrate_air", "calibrate_100_", "obj0", "obj1", "co_enabled", "he_enabled", "calibrate_100__1", "obj2", "brightness", "obj3", "btn_home_1", "header", "batt_indicator", "o2_pnl", "o2", "co", "he_pnl", "he", "obj4", "obj5", "obj6", "large_panel", "o2_1", "obj7", "o2_conf_pnl", "co_conf_pnl", "he_conf_pnl", "obj8", "obj9" };
static const char *style_names[] = { "LabelStyle", "PanelStyle" };

void create_screens() {
    eez_flow_init_styles(add_style, remove_style);
    
    eez_flow_init_screen_names(screen_names, sizeof(screen_names) / sizeof(const char *));
    eez_flow_init_object_names(object_names, sizeof(object_names) / sizeof(const char *));
    eez_flow_init_style_names(style_names, sizeof(style_names) / sizeof(const char *));
    
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_main();
    create_screen_large();
    create_screen_config();
    create_screen_log();
}

typedef void (*tick_screen_func_t)();

tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
    tick_screen_large,
    tick_screen_config,
    tick_screen_log,
};

void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
