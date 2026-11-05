#ifndef EEZ_LVGL_UI_FONTS_H
#define EEZ_LVGL_UI_FONTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_font_t ui_font_dseg14_modern_italic_12;
extern const lv_font_t ui_font_dseg14_modern_italic_14;
extern const lv_font_t ui_font_dseg14_modern_italic_18;
extern const lv_font_t ui_font_dseg14_modern_italic_24;
extern const lv_font_t ui_font_dseg14_modern_italic_30;
extern const lv_font_t ui_font_dseg14_modern_italic_32;
extern const lv_font_t ui_font_dseg_14_modern_italic_38;
extern const lv_font_t ui_font_dseg14_modern_bold_italic_32;
extern const lv_font_t ui_font_dseg7_italic_14;
extern const lv_font_t ui_font_dseg7_classic_bold_italic_14;
extern const lv_font_t ui_font_dseg7_classic_bold_italic_18;
extern const lv_font_t ui_font_dseg7_classic_bold_italic_20;
extern const lv_font_t ui_font_dseg7_classic_bold_italic_22;

#ifndef EXT_FONT_DESC_T
#define EXT_FONT_DESC_T
typedef struct _ext_font_desc_t {
    const char *name;
    const void *font_ptr;
} ext_font_desc_t;
#endif

extern ext_font_desc_t fonts[];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_FONTS_H*/