#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_NOT_USED = 2,
    _SCREEN_ID_LAST = 2
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *not_used;
    lv_obj_t *background_container;
    lv_obj_t *header_container;
    lv_obj_t *main_viewport_container;
    lv_obj_t *lcd_1;
    lv_obj_t *sat_no_info;
    lv_obj_t *gps_icon;
    lv_obj_t *wifi_info;
    lv_obj_t *ip_address_info;
    lv_obj_t *sync_source;
    lv_obj_t *info_label_1;
    lv_obj_t *info_label_2;
    lv_obj_t *info_label_3;
    lv_obj_t *info_label_4;
    lv_obj_t *meteo_area_container;
    lv_obj_t *weather_icon;
    lv_obj_t *container_sageti_evolutie_meteo;
    lv_obj_t *interior_temp;
    lv_obj_t *temp_unit_indoor;
    lv_obj_t *pressure_value;
    lv_obj_t *pressure_unit;
    lv_obj_t *forecast_text;
    lv_obj_t *clock_loc_info_container;
    lv_obj_t *ext_sensors;
    lv_obj_t *temp_unit_outd_1;
    lv_obj_t *ext_temp_1;
    lv_obj_t *temp_label_1;
    lv_obj_t *time_hour_minute;
    lv_obj_t *time_seconds;
    lv_obj_t *utc_time_hour_minute;
    lv_obj_t *utc_lt;
    lv_obj_t *local_date_label;
    lv_obj_t *day_date_label;
    lv_obj_t *maidenhead;
    lv_obj_t *watch_alarm_active_indicator;
    lv_obj_t *chime_active_indicator;
    lv_obj_t *weather_alert_active_indicator;
    lv_obj_t *utc_lt_1;
    lv_obj_t *pos_source_label;
    lv_obj_t *sync_running;
    lv_obj_t *barograph_time_span;
    lv_obj_t *time_span_scale;
    lv_obj_t *baro_graph;
    lv_obj_t *lcd_2;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void create_screen_not_used();
void tick_screen_not_used();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/