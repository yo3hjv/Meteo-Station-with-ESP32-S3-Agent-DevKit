#ifndef EEZ_LVGL_UI_STYLES_H
#define EEZ_LVGL_UI_STYLES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Style: Function_Buttons
lv_style_t *get_style_function_buttons_MAIN_DEFAULT();
lv_style_t *get_style_function_buttons_MAIN_PRESSED();
void add_style_function_buttons(lv_obj_t *obj);
void remove_style_function_buttons(lv_obj_t *obj);

// Style: small_indicators
lv_style_t *get_style_small_indicators_MAIN_CHECKED();
lv_style_t *get_style_small_indicators_MAIN_DEFAULT();
void add_style_small_indicators(lv_obj_t *obj);
void remove_style_small_indicators(lv_obj_t *obj);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_STYLES_H*/