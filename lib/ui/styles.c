#include "styles.h"
#include "images.h"
#include "fonts.h"

//
// Style: LabelStyle
//

void init_style_label_style_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_text_font(style, &lv_font_montserrat_14);
};

lv_style_t *get_style_label_style_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_label_style_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_label_style(lv_obj_t *obj) {
    lv_obj_add_style(obj, get_style_label_style_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_label_style(lv_obj_t *obj) {
    lv_obj_remove_style(obj, get_style_label_style_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: PanelStyle
//

void init_style_panel_style_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_pad_top(style, 5);
    lv_style_set_pad_bottom(style, 5);
    lv_style_set_pad_left(style, 5);
    lv_style_set_pad_right(style, 5);
    lv_style_set_pad_row(style, 5);
    lv_style_set_pad_column(style, 5);
};

lv_style_t *get_style_panel_style_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_panel_style_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_panel_style(lv_obj_t *obj) {
    lv_obj_add_style(obj, get_style_panel_style_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_panel_style(lv_obj_t *obj) {
    lv_obj_remove_style(obj, get_style_panel_style_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
//
//

void add_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*AddStyleFunc)(lv_obj_t *obj);
    static const AddStyleFunc add_style_funcs[] = {
        add_style_label_style,
        add_style_panel_style,
    };
    add_style_funcs[styleIndex](obj);
}

void remove_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*RemoveStyleFunc)(lv_obj_t *obj);
    static const RemoveStyleFunc remove_style_funcs[] = {
        remove_style_label_style,
        remove_style_panel_style,
    };
    remove_style_funcs[styleIndex](obj);
}

