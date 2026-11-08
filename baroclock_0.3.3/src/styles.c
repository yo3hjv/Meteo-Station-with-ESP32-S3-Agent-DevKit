#include "styles.h"
#include "images.h"
#include "fonts.h"

#include "ui.h"
#include "screens.h"

//
// Style: Function_Buttons
//

void init_style_function_buttons_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_border_color(style, lv_color_hex(0xff9a9a9a));
    lv_style_set_bg_color(style, lv_color_hex(0xff656565));
    lv_style_set_bg_opa(style, 255);
    lv_style_set_bg_grad_dir(style, LV_GRAD_DIR_VER);
    lv_style_set_bg_grad_color(style, lv_color_hex(0xff0d0d0d));
    lv_style_set_bg_grad_stop(style, 60);
    lv_style_set_bg_main_stop(style, 1);
    lv_style_set_border_opa(style, 255);
    lv_style_set_border_width(style, 1);
};

lv_style_t *get_style_function_buttons_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_function_buttons_MAIN_DEFAULT(style);
    }
    return style;
};

void init_style_function_buttons_MAIN_PRESSED(lv_style_t *style) {
    lv_style_set_border_color(style, lv_color_hex(0xff9a9a9a));
    lv_style_set_border_opa(style, 255);
    lv_style_set_border_width(style, 2);
};

lv_style_t *get_style_function_buttons_MAIN_PRESSED() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_function_buttons_MAIN_PRESSED(style);
    }
    return style;
};

void add_style_function_buttons(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_function_buttons_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_function_buttons_MAIN_PRESSED(), LV_PART_MAIN | LV_STATE_PRESSED);
};

void remove_style_function_buttons(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_function_buttons_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(obj, get_style_function_buttons_MAIN_PRESSED(), LV_PART_MAIN | LV_STATE_PRESSED);
};

//
// Style: small_indicators
//

void init_style_small_indicators_MAIN_CHECKED(lv_style_t *style) {
    lv_style_set_text_color(style, lv_color_hex(0xff2b2b2b));
    lv_style_set_text_opa(style, 255);
    lv_style_set_text_font(style, &lv_font_montserrat_10);
};

lv_style_t *get_style_small_indicators_MAIN_CHECKED() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_small_indicators_MAIN_CHECKED(style);
    }
    return style;
};

void init_style_small_indicators_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_text_font(style, &lv_font_montserrat_10);
    lv_style_set_text_color(style, lv_color_hex(0xffcccbcb));
};

lv_style_t *get_style_small_indicators_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_small_indicators_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_small_indicators(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_small_indicators_MAIN_CHECKED(), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_add_style(obj, get_style_small_indicators_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_small_indicators(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_small_indicators_MAIN_CHECKED(), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_remove_style(obj, get_style_small_indicators_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
//
//

void add_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*AddStyleFunc)(lv_obj_t *obj);
    static const AddStyleFunc add_style_funcs[] = {
        add_style_function_buttons,
        add_style_small_indicators,
    };
    add_style_funcs[styleIndex](obj);
}

void remove_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*RemoveStyleFunc)(lv_obj_t *obj);
    static const RemoveStyleFunc remove_style_funcs[] = {
        remove_style_function_buttons,
        remove_style_small_indicators,
    };
    remove_style_funcs[styleIndex](obj);
}