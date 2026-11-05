#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

//
// Screens
//

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    {
        lv_obj_t *parent_obj = obj;
        {
            // background_container
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.background_container = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 320);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // header_container
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.header_container = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, 480, 29);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    // main_viewport_container
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.main_viewport_container = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, 480, 320);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // lcd_1
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.lcd_1 = obj;
                            lv_obj_set_pos(obj, 9, 10);
                            lv_obj_set_size(obj, 463, 302);
                            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff4fd7d7), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff191919), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_outline_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_outline_color(obj, lv_color_hex(0xff2e9999), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_outline_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_outline_pad(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // sat_no_info
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.sat_no_info = obj;
                                    lv_obj_set_pos(obj, 350, 276);
                                    lv_obj_set_size(obj, 59, 14);
                                    lv_obj_set_style_text_font(obj, &ui_font_dseg14_modern_italic_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "16/12");
                                }
                                {
                                    // gps_icon
                                    lv_obj_t *obj = lv_image_create(parent_obj);
                                    objects.gps_icon = obj;
                                    lv_obj_set_pos(obj, 422, 258);
                                    lv_obj_set_size(obj, 32, 32);
                                    lv_image_set_src(obj, &img_gps_ic_on);
                                    lv_obj_set_style_image_opa(obj, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_image_recolor_opa(obj, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_CHECKED);
                                    lv_obj_set_style_image_recolor(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_CHECKED);
                                    lv_obj_set_style_image_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_CHECKED);
                                }
                                {
                                    // wifi_info
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.wifi_info = obj;
                                    lv_obj_set_pos(obj, 9, 276);
                                    lv_obj_set_size(obj, 108, 14);
                                    lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
                                    lv_obj_set_style_text_font(obj, &ui_font_dseg14_modern_italic_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "Nimic_Interesant");
                                }
                                {
                                    // ip_address_info
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.ip_address_info = obj;
                                    lv_obj_set_pos(obj, 136, 276);
                                    lv_obj_set_size(obj, 120, 14);
                                    lv_obj_set_style_text_font(obj, &ui_font_dseg14_modern_italic_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "255.255.255.255");
                                }
                                {
                                    // sync_source
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.sync_source = obj;
                                    lv_obj_set_pos(obj, 285, 276);
                                    lv_obj_set_size(obj, 36, 14);
                                    lv_obj_set_style_text_font(obj, &ui_font_dseg14_modern_italic_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_PRESSED);
                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_PRESSED);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_PRESSED);
                                    lv_label_set_text(obj, "NTP");
                                }
                                {
                                    // info_label_1
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.info_label_1 = obj;
                                    lv_obj_set_pos(obj, 29, 258);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff3b3b3b), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "WiFi Name");
                                }
                                {
                                    // info_label_2
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.info_label_2 = obj;
                                    lv_obj_set_pos(obj, 163, 258);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff3b3b3b), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "IP Address");
                                }
                                {
                                    // info_label_3
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.info_label_3 = obj;
                                    lv_obj_set_pos(obj, 266, 258);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff3b3b3b), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "Sync Source");
                                }
                                {
                                    // info_label_4
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.info_label_4 = obj;
                                    lv_obj_set_pos(obj, 365, 258);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff3b3b3b), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "SATS");
                                }
                                {
                                    // meteo_area_container
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.meteo_area_container = obj;
                                    lv_obj_set_pos(obj, 276, 6);
                                    lv_obj_set_size(obj, 178, 190);
                                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff303030), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            // weather_icon
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.weather_icon = obj;
                                            lv_obj_set_pos(obj, 3, 44);
                                            lv_obj_set_size(obj, 170, 119);
                                            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff898989), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff4fc1c5), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        }
                                        {
                                            // container_sageti_evolutie_meteo
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.container_sageti_evolutie_meteo = obj;
                                            lv_obj_set_pos(obj, 132, 3);
                                            lv_obj_set_size(obj, 41, 21);
                                            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        }
                                        {
                                            // interior_temp
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.interior_temp = obj;
                                            lv_obj_set_pos(obj, 15, 24);
                                            lv_obj_set_size(obj, 37, 19);
                                            lv_obj_set_style_text_font(obj, &ui_font_dseg7_classic_bold_italic_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "23.6");
                                        }
                                        {
                                            // temp_unit_indoor
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.temp_unit_indoor = obj;
                                            lv_obj_set_pos(obj, 52, 26);
                                            lv_obj_set_size(obj, 21, 15);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_label_set_text(obj, "C.");
                                        }
                                        {
                                            // pressure_value
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.pressure_value = obj;
                                            lv_obj_set_pos(obj, 3, 4);
                                            lv_obj_set_size(obj, 61, 19);
                                            lv_obj_set_style_text_font(obj, &ui_font_dseg7_classic_bold_italic_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "1020.4");
                                        }
                                        {
                                            // pressure_unit
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.pressure_unit = obj;
                                            lv_obj_set_pos(obj, 64, 6);
                                            lv_obj_set_size(obj, 36, 16);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_label_set_text(obj, "hPa");
                                        }
                                        {
                                            // forecast_text
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.forecast_text = obj;
                                            lv_obj_set_pos(obj, 3, 167);
                                            lv_obj_set_size(obj, 160, 21);
                                            lv_label_set_long_mode(obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
                                            lv_obj_set_style_text_font(obj, &ui_font_dseg14_modern_italic_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, " INITIALIZING");
                                        }
                                    }
                                }
                                {
                                    // clock_loc_info_container
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.clock_loc_info_container = obj;
                                    lv_obj_set_pos(obj, 3, 6);
                                    lv_obj_set_size(obj, 271, 190);
                                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff303030), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            // ext_sensors
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.ext_sensors = obj;
                                            lv_obj_set_pos(obj, 177, 105);
                                            lv_obj_set_size(obj, 92, 79);
                                            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff898989), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    // temp_unit_outd_1
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.temp_unit_outd_1 = obj;
                                                    lv_obj_set_pos(obj, 71, 31);
                                                    lv_obj_set_size(obj, 17, 15);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_PRESSED);
                                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_PRESSED);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_PRESSED);
                                                    lv_label_set_text(obj, "C.");
                                                }
                                                {
                                                    // ext_temp_1
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.ext_temp_1 = obj;
                                                    lv_obj_set_pos(obj, 8, 34);
                                                    lv_obj_set_size(obj, 59, 25);
                                                    lv_obj_set_style_text_font(obj, &ui_font_dseg7_classic_bold_italic_22, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "23.6");
                                                }
                                                {
                                                    // temp_label_1
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.temp_label_1 = obj;
                                                    lv_obj_set_pos(obj, 3, 4);
                                                    lv_obj_set_size(obj, 85, 16);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_PRESSED);
                                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_PRESSED);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_PRESSED);
                                                    lv_label_set_text(obj, "Outdoor ");
                                                }
                                            }
                                        }
                                        {
                                            // time_hour_minute
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.time_hour_minute = obj;
                                            lv_obj_set_pos(obj, 41, 21);
                                            lv_obj_set_size(obj, 132, 38);
                                            lv_obj_set_style_text_font(obj, &ui_font_dseg_14_modern_italic_38, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "23:59");
                                        }
                                        {
                                            // time_seconds
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.time_seconds = obj;
                                            lv_obj_set_pos(obj, 173, 21);
                                            lv_obj_set_size(obj, 45, 29);
                                            lv_obj_set_style_text_font(obj, &ui_font_dseg14_modern_italic_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, ":59");
                                        }
                                        {
                                            // utc_time_hour_minute
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.utc_time_hour_minute = obj;
                                            lv_obj_set_pos(obj, 47, 102);
                                            lv_obj_set_size(obj, 113, 38);
                                            lv_obj_set_style_text_font(obj, &ui_font_dseg14_modern_bold_italic_32, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "23:59");
                                        }
                                        {
                                            // utc_lt
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.utc_lt = obj;
                                            lv_obj_set_pos(obj, 5, 21);
                                            lv_obj_set_size(obj, 18, 19);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff3b3b3b), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_label_set_text(obj, "LT");
                                        }
                                        {
                                            // local_date_label
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.local_date_label = obj;
                                            lv_obj_set_pos(obj, 41, 72);
                                            lv_obj_set_size(obj, 85, 22);
                                            lv_obj_set_style_text_font(obj, &ui_font_dseg14_modern_italic_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "22 oct. ");
                                        }
                                        {
                                            // day_date_label
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.day_date_label = obj;
                                            lv_obj_set_pos(obj, 135, 72);
                                            lv_obj_set_size(obj, 51, 22);
                                            lv_obj_set_style_text_font(obj, &ui_font_dseg14_modern_italic_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "TUE");
                                        }
                                        {
                                            // maidenhead
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.maidenhead = obj;
                                            lv_obj_set_pos(obj, 47, 153);
                                            lv_obj_set_size(obj, 126, 29);
                                            lv_obj_set_style_text_font(obj, &ui_font_dseg14_modern_italic_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "kn24bk");
                                        }
                                        {
                                            // watch_alarm_active_indicator
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.watch_alarm_active_indicator = obj;
                                            lv_obj_set_pos(obj, 115, 1);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            add_style_small_indicators(obj);
                                            lv_label_set_text(obj, "ALM");
                                        }
                                        {
                                            // chime_active_indicator
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.chime_active_indicator = obj;
                                            lv_obj_set_pos(obj, 47, 1);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            add_style_small_indicators(obj);
                                            lv_label_set_text(obj, "CHM");
                                        }
                                        {
                                            // weather_alert_active_indicator
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.weather_alert_active_indicator = obj;
                                            lv_obj_set_pos(obj, 183, 1);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            add_style_small_indicators(obj);
                                            lv_label_set_text(obj, "ALRT");
                                        }
                                        {
                                            // utc_lt_1
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.utc_lt_1 = obj;
                                            lv_obj_set_pos(obj, 5, 102);
                                            lv_obj_set_size(obj, 31, 16);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff3b3b3b), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_label_set_text(obj, "UTC");
                                        }
                                        {
                                            // pos_source_label
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.pos_source_label = obj;
                                            lv_obj_set_pos(obj, 5, 158);
                                            lv_obj_set_size(obj, 36, 19);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff3b3b3b), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_label_set_text(obj, "USR");
                                        }
                                        {
                                            // sync_running
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.sync_running = obj;
                                            lv_obj_set_pos(obj, 5, 121);
                                            lv_obj_set_size(obj, 31, 16);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff3b3b3b), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_label_set_text(obj, "SYN");
                                        }
                                    }
                                }
                                {
                                    // barograph_time_span
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.barograph_time_span = obj;
                                    lv_obj_set_pos(obj, 3, 198);
                                    lv_obj_set_size(obj, 451, 20);
                                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            // time_span_scale
                                            lv_obj_t *obj = lv_scale_create(parent_obj);
                                            objects.time_span_scale = obj;
                                            lv_obj_set_pos(obj, 13, 5);
                                            lv_obj_set_size(obj, 434, 10);
                                            lv_scale_set_mode(obj, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
                                            lv_scale_set_range(obj, -10, 0);
                                            lv_scale_set_angle_range(obj, 270);
                                            lv_scale_set_rotation(obj, 135);
                                            lv_scale_set_total_tick_count(obj, 31);
                                            lv_scale_set_major_tick_every(obj, 5);
                                            lv_scale_set_label_show(obj, true);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        }
                                    }
                                }
                                {
                                    // baro_graph
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.baro_graph = obj;
                                    lv_obj_set_pos(obj, 3, 220);
                                    lv_obj_set_size(obj, 451, 34);
                                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff303030), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 150, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff4fc1c5), LV_PART_MAIN | LV_STATE_DEFAULT);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    tick_screen_main();
}

void tick_screen_main() {
}

void create_screen_not_used() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.not_used = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    {
        lv_obj_t *parent_obj = obj;
        {
            // lcd_2
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.lcd_2 = obj;
            lv_obj_set_pos(obj, 9, 129);
            lv_obj_set_size(obj, 463, 104);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff4fd1d7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff1e927d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_grad_stop(obj, 1255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_main_stop(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff33beb6), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_outline_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_outline_color(obj, lv_color_hex(0xff2e9999), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_outline_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_outline_pad(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_not_used();
}

void tick_screen_not_used() {
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
    tick_screen_not_used,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
    { "DSEG14_modern_italic_12", &ui_font_dseg14_modern_italic_12 },
    { "DSEG14_modern_italic_14", &ui_font_dseg14_modern_italic_14 },
    { "DSEG14_modern_italic_18", &ui_font_dseg14_modern_italic_18 },
    { "DSEG14_modern_italic_24", &ui_font_dseg14_modern_italic_24 },
    { "DSEG14_modern_italic_30", &ui_font_dseg14_modern_italic_30 },
    { "DSEG14_modern_italic_32", &ui_font_dseg14_modern_italic_32 },
    { "DSEG_14_modern_italic_38", &ui_font_dseg_14_modern_italic_38 },
    { "DSEG14_modern_bold_italic_32", &ui_font_dseg14_modern_bold_italic_32 },
    { "DSEG7_italic_14", &ui_font_dseg7_italic_14 },
    { "DSEG7_classic_bold_italic_14", &ui_font_dseg7_classic_bold_italic_14 },
    { "DSEG7_classic_bold_italic_18", &ui_font_dseg7_classic_bold_italic_18 },
    { "DSEG7_classic_bold_italic_20", &ui_font_dseg7_classic_bold_italic_20 },
    { "DSEG7_classic_bold_italic_22", &ui_font_dseg7_classic_bold_italic_22 },
#if LV_FONT_MONTSERRAT_8
    { "MONTSERRAT_8", &lv_font_montserrat_8 },
#endif
#if LV_FONT_MONTSERRAT_10
    { "MONTSERRAT_10", &lv_font_montserrat_10 },
#endif
#if LV_FONT_MONTSERRAT_12
    { "MONTSERRAT_12", &lv_font_montserrat_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
#if LV_FONT_MONTSERRAT_16
    { "MONTSERRAT_16", &lv_font_montserrat_16 },
#endif
#if LV_FONT_MONTSERRAT_18
    { "MONTSERRAT_18", &lv_font_montserrat_18 },
#endif
#if LV_FONT_MONTSERRAT_20
    { "MONTSERRAT_20", &lv_font_montserrat_20 },
#endif
#if LV_FONT_MONTSERRAT_22
    { "MONTSERRAT_22", &lv_font_montserrat_22 },
#endif
#if LV_FONT_MONTSERRAT_24
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
#endif
#if LV_FONT_MONTSERRAT_26
    { "MONTSERRAT_26", &lv_font_montserrat_26 },
#endif
#if LV_FONT_MONTSERRAT_28
    { "MONTSERRAT_28", &lv_font_montserrat_28 },
#endif
#if LV_FONT_MONTSERRAT_30
    { "MONTSERRAT_30", &lv_font_montserrat_30 },
#endif
#if LV_FONT_MONTSERRAT_32
    { "MONTSERRAT_32", &lv_font_montserrat_32 },
#endif
#if LV_FONT_MONTSERRAT_34
    { "MONTSERRAT_34", &lv_font_montserrat_34 },
#endif
#if LV_FONT_MONTSERRAT_36
    { "MONTSERRAT_36", &lv_font_montserrat_36 },
#endif
#if LV_FONT_MONTSERRAT_38
    { "MONTSERRAT_38", &lv_font_montserrat_38 },
#endif
#if LV_FONT_MONTSERRAT_40
    { "MONTSERRAT_40", &lv_font_montserrat_40 },
#endif
#if LV_FONT_MONTSERRAT_42
    { "MONTSERRAT_42", &lv_font_montserrat_42 },
#endif
#if LV_FONT_MONTSERRAT_44
    { "MONTSERRAT_44", &lv_font_montserrat_44 },
#endif
#if LV_FONT_MONTSERRAT_46
    { "MONTSERRAT_46", &lv_font_montserrat_46 },
#endif
#if LV_FONT_MONTSERRAT_48
    { "MONTSERRAT_48", &lv_font_montserrat_48 },
#endif
};

//
// Color themes
//

uint32_t active_theme_index = 0;

//
//
//

void create_screens() {

// Set default LVGL theme
    lv_display_t *dispp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_display_set_theme(dispp, theme);
    
    // Initialize screens
    // Create screens
    create_screen_main();
    create_screen_not_used();
}