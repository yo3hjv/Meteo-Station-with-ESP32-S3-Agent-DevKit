#include "gui.h"
#include "setup.h"
#include "lcd_driver.h"
#include "touch.h"
#include "sound.h"
#include "wifi_mgr.h"
#include "gps_handler.h"
#include "bme_sensor.h"
#include "weather_logger.h"
#include "clock.h"
#include "ext_sensors.h"

#include <lvgl.h>
#include "src/ui.h"
#include "src/screens.h"

#define LVGL_DRAW_BUF_LINES 30
static lv_color_t *draw_buf = nullptr;

// Canvas pentru Barograf in interiorul containerului baro_graph (451 x 34 px)
#define BARO_CANVAS_W 451
#define BARO_CANVAS_H 34
static uint8_t *baro_canvas_buf = nullptr;
static lv_obj_t *canvas_baro = nullptr;

// Canvas pentru Pictograma Meteo in interiorul containerului weather_icon (170 x 119 px, ARGB8888)
#define WTR_CANVAS_W 170
#define WTR_CANVAS_H 119
static uint8_t *wtr_canvas_buf = nullptr;
static lv_obj_t *canvas_wtr = nullptr;

// Canvas pentru Simboluri Dinamica Barica in interiorul container_sageti_evolutie_meteo (41 x 21 px, ARGB8888)
#define TREND_CANVAS_W 41
#define TREND_CANVAS_H 21
static uint8_t *trend_canvas_buf = nullptr;
static lv_obj_t *canvas_trend = nullptr;

static unsigned long lastChimeHour = 99;

// ----------------------------------------------------------------------------
// Palete Profiluri Cromatice (Teme) LVGL pentru Ecranul Principal
// ----------------------------------------------------------------------------
const UiThemePalette UI_THEMES[UI_THEME_COUNT] = {
    { THEME_0_NAME, THEME_0_BG, THEME_0_PANEL, THEME_0_BORDER, THEME_0_TEXT_PRI, THEME_0_TEXT_SEC, THEME_0_TEXT_DATE, THEME_0_TEXT_DAY, THEME_0_ACCENT, false },
    { THEME_1_NAME, THEME_1_BG, THEME_1_PANEL, THEME_1_BORDER, THEME_1_TEXT_PRI, THEME_1_TEXT_SEC, THEME_1_TEXT_DATE, THEME_1_TEXT_DAY, THEME_1_ACCENT, false },
    { THEME_2_NAME, THEME_2_BG, THEME_2_PANEL, THEME_2_BORDER, THEME_2_TEXT_PRI, THEME_2_TEXT_SEC, THEME_2_TEXT_DATE, THEME_2_TEXT_DAY, THEME_2_ACCENT, false },
    { THEME_3_NAME, THEME_3_BG, THEME_3_PANEL, THEME_3_BORDER, THEME_3_TEXT_PRI, THEME_3_TEXT_SEC, THEME_3_TEXT_DATE, THEME_3_TEXT_DAY, THEME_3_ACCENT, false },
    { THEME_4_NAME, THEME_4_BG, THEME_4_PANEL, THEME_4_BORDER, THEME_4_TEXT_PRI, THEME_4_TEXT_SEC, THEME_4_TEXT_DATE, THEME_4_TEXT_DAY, THEME_4_ACCENT, false },
    { THEME_5_NAME, THEME_5_BG, THEME_5_PANEL, THEME_5_BORDER, THEME_5_TEXT_PRI, THEME_5_TEXT_SEC, THEME_5_TEXT_DATE, THEME_5_TEXT_DAY, THEME_5_ACCENT, true }
};

static uint8_t currentUiTheme = DEFAULT_UI_THEME;

uint8_t guiGetCurrentTheme() {
    return currentUiTheme;
}

static void applyThemeLabelsRecursive(lv_obj_t *obj, uint32_t textPri, uint32_t textSec, uint32_t textDate, uint32_t textDay) {
    if (!obj) return;

    if (lv_obj_check_type(obj, &lv_label_class)) {
        if (obj != objects.watch_alarm_active_indicator &&
            obj != objects.chime_active_indicator &&
            obj != objects.weather_alert_active_indicator) {
            if (obj == objects.time_hour_minute ||
                obj == objects.time_seconds ||
                obj == objects.utc_time_hour_minute ||
                obj == objects.pressure_value ||
                obj == objects.interior_temp ||
                obj == objects.ext_temp_1) {
                lv_obj_set_style_text_color(obj, lv_color_hex(textPri), LV_PART_MAIN | LV_STATE_DEFAULT);
            } else if (obj == objects.local_date_label) {
                lv_obj_set_style_text_color(obj, lv_color_hex(textDate), LV_PART_MAIN | LV_STATE_DEFAULT);
            } else if (obj == objects.day_date_label) {
                lv_obj_set_style_text_color(obj, lv_color_hex(textDay), LV_PART_MAIN | LV_STATE_DEFAULT);
            } else {
                lv_obj_set_style_text_color(obj, lv_color_hex(textSec), LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        }
    }

    uint32_t childCnt = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < childCnt; i++) {
        applyThemeLabelsRecursive(lv_obj_get_child(obj, i), textPri, textSec, textDate, textDay);
    }
}

void guiApplyTheme(uint8_t themeIndex) {
    if (themeIndex >= UI_THEME_COUNT) themeIndex = DEFAULT_UI_THEME;
    currentUiTheme = themeIndex;
    const UiThemePalette &pal = UI_THEMES[themeIndex];

    if (!objects.main) return;

    if (pal.isEezOriginal) {
        if (objects.main) {
            lv_obj_set_style_bg_color(objects.main, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_grad_dir(objects.main, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (objects.background_container) {
            lv_obj_set_style_bg_color(objects.background_container, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_grad_dir(objects.background_container, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (objects.lcd_1) {
            lv_obj_set_style_bg_color(objects.lcd_1, lv_color_hex(0x4fd7d7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_grad_dir(objects.lcd_1, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(objects.lcd_1, lv_color_hex(0x191919), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_outline_color(objects.lcd_1, lv_color_hex(0x2e9999), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (objects.weather_icon) {
            lv_obj_set_style_bg_color(objects.weather_icon, lv_color_hex(0x4fc1c5), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(objects.weather_icon, lv_color_hex(0x898989), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (objects.clock_loc_info_container) {
            lv_obj_set_style_border_color(objects.clock_loc_info_container, lv_color_hex(0x303030), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (objects.ext_sensors) {
            lv_obj_set_style_border_color(objects.ext_sensors, lv_color_hex(0x898989), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (objects.baro_graph) {
            lv_obj_set_style_bg_color(objects.baro_graph, lv_color_hex(0x4fc1c5), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(objects.baro_graph, lv_color_hex(0x303030), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (objects.time_span_scale) {
            lv_obj_set_style_line_color(objects.time_span_scale, lv_color_hex(0x303030), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_line_color(objects.time_span_scale, lv_color_hex(0x303030), LV_PART_ITEMS | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(objects.time_span_scale, lv_color_hex(0x212121), LV_PART_ITEMS | LV_STATE_DEFAULT);
        }
        applyThemeLabelsRecursive(objects.main, 0x212121, 0x101010, 0x212121, 0x212121);
    } else {
        if (objects.main) {
            lv_obj_set_style_bg_color(objects.main, lv_color_hex(pal.bgColor), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_grad_dir(objects.main, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (objects.background_container) {
            lv_obj_set_style_bg_color(objects.background_container, lv_color_hex(pal.bgColor), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_grad_dir(objects.background_container, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (objects.lcd_1) {
            lv_obj_set_style_bg_color(objects.lcd_1, lv_color_hex(pal.bgColor), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_grad_dir(objects.lcd_1, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(objects.lcd_1, lv_color_hex(pal.borderColor), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_outline_color(objects.lcd_1, lv_color_hex(pal.borderColor), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (objects.weather_icon) {
            lv_obj_set_style_bg_color(objects.weather_icon, lv_color_hex(pal.panelColor), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(objects.weather_icon, lv_color_hex(pal.borderColor), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (objects.clock_loc_info_container) {
            lv_obj_set_style_border_color(objects.clock_loc_info_container, lv_color_hex(pal.borderColor), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (objects.ext_sensors) {
            lv_obj_set_style_border_color(objects.ext_sensors, lv_color_hex(pal.borderColor), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (objects.baro_graph) {
            lv_obj_set_style_bg_color(objects.baro_graph, lv_color_hex(pal.panelColor), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(objects.baro_graph, lv_color_hex(pal.borderColor), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (objects.time_span_scale) {
            lv_obj_set_style_line_color(objects.time_span_scale, lv_color_hex(pal.borderColor), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_line_color(objects.time_span_scale, lv_color_hex(pal.borderColor), LV_PART_ITEMS | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(objects.time_span_scale, lv_color_hex(pal.textSecondary), LV_PART_ITEMS | LV_STATE_DEFAULT);
        }
        applyThemeLabelsRecursive(objects.main, pal.textPrimary, pal.textSecondary, pal.textDate, pal.textDay);
    }

    if (objects.watch_alarm_active_indicator) {
        lv_obj_set_style_text_color(objects.watch_alarm_active_indicator, lv_color_hex(pal.accentColor), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(objects.watch_alarm_active_indicator, lv_color_hex(pal.isEezOriginal ? 0xcccbcb : pal.borderColor), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (objects.chime_active_indicator) {
        lv_obj_set_style_text_color(objects.chime_active_indicator, lv_color_hex(pal.accentColor), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(objects.chime_active_indicator, lv_color_hex(pal.isEezOriginal ? 0xcccbcb : pal.borderColor), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (objects.weather_alert_active_indicator) {
        lv_obj_set_style_text_color(objects.weather_alert_active_indicator, lv_color_hex(pal.accentColor), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(objects.weather_alert_active_indicator, lv_color_hex(pal.isEezOriginal ? 0xcccbcb : pal.borderColor), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (objects.gps_icon) {
        lv_obj_set_style_image_recolor(objects.gps_icon, lv_color_hex(pal.accentColor), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_image_recolor_opa(objects.gps_icon, 255, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_image_recolor(objects.gps_icon, lv_color_hex(pal.isEezOriginal ? 0x000000 : pal.borderColor), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_image_recolor_opa(objects.gps_icon, pal.isEezOriginal ? 40 : 160, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (canvas_baro) lv_obj_invalidate(canvas_baro);
    if (canvas_wtr) lv_obj_invalidate(canvas_wtr);
    if (canvas_trend) lv_obj_invalidate(canvas_trend);
    lv_obj_invalidate(objects.main);
}

// Callback baza de timp LVGL 9 (millis)
static uint32_t my_tick_get_cb(void) {
    return millis();
}

// Callback Flush LVGL 9 catre LCD ST7796 prin magistrala paralela rapida pe 8 biti
static void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    static bool firstFlush = false;
    if (!firstFlush) {
        firstFlush = true;
#if DEBUG_PONSTEST
        Serial.printf("[GUI] Primul flush LVGL: x1=%d, y1=%d, x2=%d, y2=%d\n", area->x1, area->y1, area->x2, area->y2);
#endif
    }
    uint16_t w = (area->x2 - area->x1 + 1);
    uint16_t h = (area->y2 - area->y1 + 1);

    lcdDrawBitmap(area->x1, area->y1, w, h, (const uint16_t*)px_map);

    lv_display_flush_ready(disp);
}

// Callback citire Touchscreen FT6336 integrat ca LVGL indev pointer
static int16_t lastValidX = 0;
static int16_t lastValidY = 0;
static unsigned long satPressStart = 0;
static bool satPageTriggered = false;

static void my_touch_read(lv_indev_t *indev, lv_indev_data_t *data) {
    uint16_t rawX = 0, rawY = 0;
    if (touchReadRaw(rawX, rawY)) {
        data->state = LV_INDEV_STATE_PRESSED;
        // Mapare FT6336 (320x480 portret -> 480x320 landscape)
        bool rot180 = settingsGetRotate180();
        int16_t lx = rot180 ? (int16_t)(479 - rawY) : (int16_t)rawY;
        int16_t ly = rot180 ? (int16_t)rawX         : (int16_t)(319 - rawX);
        if (lx < 0) lx = 0;
        if (lx >= SCREEN_WIDTH) lx = SCREEN_WIDTH - 1;
        if (ly < 0) ly = 0;
        if (ly >= SCREEN_HEIGHT) ly = SCREEN_HEIGHT - 1;

        lastValidX = lx;
        lastValidY = ly;
        data->point.x = lx;
        data->point.y = ly;

        // Verificare apasare lunga (1 secunda) in coltul dreapta-jos pe iconita de satelit (X >= 390, Y >= 240)
        if (lx >= 390 && ly >= 240) {
            if (satPressStart == 0) {
                satPressStart = millis();
                satPageTriggered = false;
            } else if (!satPageTriggered && (millis() - satPressStart >= 1000)) {
                satPageTriggered = true;
                soundPlayFeedbackClick();
                settingsSetDisplayPage(1);
#if DEBUG_TOUCH
                Serial.println("[TOUCH] Touch 1s pe iconita satelit -> Comutare ecran GNSS Info");
#endif
            }
        } else {
            satPressStart = 0;
            satPageTriggered = false;
        }

        // Daca suna ceasul desteptator sau alerta de furtuna, orice atingere pe ecran o opreste imediat
        if (soundIsAlarmClockRinging() || soundIsStormAlertRinging()) {
            soundStopAllAlarms();
            Serial.println("[TOUCH] Alarma oprita prin atingerea ecranului!");
        }

#if DEBUG_TOUCH
        static uint32_t lastTouchPrint = 0;
        if (millis() - lastTouchPrint > 250) {
            lastTouchPrint = millis();
            Serial.printf("[TOUCH DEBUG] Raw: X=%u, Y=%u -> Ecran: X=%d, Y=%d (rot180=%d)\n",
                          rawX, rawY, lx, ly, rot180 ? 1 : 0);
        }
#endif
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        data->point.x = lastValidX;
        data->point.y = lastValidY;
        satPressStart = 0;
        satPageTriggered = false;
    }
}

// ----------------------------------------------------------------------------
// Evenimente Tactile pe Obiectele UI (Touch Callbacks)
// ----------------------------------------------------------------------------
static void on_main_screen_clicked(lv_event_t *e) {
    if (soundIsAlarmClockRinging()) {
        soundStopAlarmClock();
#if DEBUG_TOUCH
        Serial.println("[TOUCH DEBUG] Alarma ceas oprita prin atingerea ecranului!");
#endif
    }
    if (soundIsStormAlertRinging()) {
        soundStopStormAlert();
#if DEBUG_TOUCH
        Serial.println("[TOUCH DEBUG] Alerta furtuna oprita prin atingerea ecranului!");
#endif
    }
}

static void on_watch_alarm_clicked(lv_event_t *e) {
    if (soundIsAlarmClockRinging()) {
        soundStopAlarmClock();
#if DEBUG_TOUCH
        Serial.println("[TOUCH DEBUG] Alarma ceas oprita prin atingere [ALM]!");
#endif
        return;
    }
    bool cur = settingsGetAlarmEnabled();
    settingsSetAlarmEnabled(!cur);
    soundPlayFeedbackClick();
    if (objects.watch_alarm_active_indicator) {
        if (!cur) lv_obj_add_state(objects.watch_alarm_active_indicator, LV_STATE_CHECKED);
        else lv_obj_remove_state(objects.watch_alarm_active_indicator, LV_STATE_CHECKED);
    }
#if DEBUG_TOUCH
    Serial.printf("[TOUCH DEBUG] -> Click pe [ALM] (Ceas desteptator) detectat! Stare dupa touch: %s\n",
                  !cur ? "ACTIVAT (ON)" : "DEZACTIVAT (OFF)");
#endif
}

static void on_chime_clicked(lv_event_t *e) {
    bool cur = settingsGetChimeEnabled();
    settingsSetChimeEnabled(!cur);
    soundPlayFeedbackClick();
    if (objects.chime_active_indicator) {
        if (!cur) lv_obj_add_state(objects.chime_active_indicator, LV_STATE_CHECKED);
        else lv_obj_remove_state(objects.chime_active_indicator, LV_STATE_CHECKED);
    }
#if DEBUG_TOUCH
    Serial.printf("[TOUCH DEBUG] -> Click pe [CHM] (Chime orar) detectat! Stare dupa touch: %s\n",
                  !cur ? "ACTIVAT (ON)" : "DEZACTIVAT (OFF)");
#endif
}

static void on_weather_alert_clicked(lv_event_t *e) {
    if (soundIsStormAlertRinging()) {
        soundStopStormAlert();
#if DEBUG_TOUCH
        Serial.println("[TOUCH DEBUG] Alerta furtuna oprita prin atingere [ALRT]!");
#endif
        return;
    }
    bool cur = settingsGetStormAlertEnabled();
    settingsSetStormAlertEnabled(!cur);
    settingsSetWtrDetAlertEnabled(!cur);
    soundPlayFeedbackClick();
    if (objects.weather_alert_active_indicator) {
        if (!cur) lv_obj_add_state(objects.weather_alert_active_indicator, LV_STATE_CHECKED);
        else lv_obj_remove_state(objects.weather_alert_active_indicator, LV_STATE_CHECKED);
    }
#if DEBUG_TOUCH
    Serial.printf("[TOUCH DEBUG] -> Click pe [ALRT] (Alerte meteo) detectat! Stare dupa touch: %s\n",
                  !cur ? "ACTIVAT (ON)" : "DEZACTIVAT (OFF)");
#endif
}

static void on_weather_icon_clicked(lv_event_t *e) {
    if (soundIsStormAlertRinging()) {
        soundStopStormAlert();
#if DEBUG_TOUCH
        Serial.println("[TOUCH DEBUG] Alerta furtuna anulata prin atingere [Weather Icon]!");
#endif
        return;
    }
    soundPlayFeedbackClick();
#if DEBUG_TOUCH
    Serial.println("[TOUCH DEBUG] -> Click pe zona [Weather Icon] detectat!");
#endif
}

// Declaratii anticipate functii randare barograf
static void renderBaroCanvas();
static void updateBaroTimeSpanScale();

// Callback gest glisare orizontala (Slide cu un deget) pe barograf pentru Zoom In / Zoom Out
static void on_baro_graph_event(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    static int16_t startX = 0;
    static int16_t startY = 0;
    static bool isPressing = false;
    static bool gestureTriggered = false;

    if (code == LV_EVENT_PRESSED) {
        lv_indev_t *indev = lv_indev_active();
        if (indev) {
            lv_point_t p;
            lv_indev_get_point(indev, &p);
            startX = p.x;
            startY = p.y;
            isPressing = true;
            gestureTriggered = false;
        }
    } else if (code == LV_EVENT_PRESSING && isPressing && !gestureTriggered) {
        lv_indev_t *indev = lv_indev_active();
        if (indev) {
            lv_point_t p;
            lv_indev_get_point(indev, &p);
            int16_t dx = p.x - startX;
            int16_t dy = p.y - startY;
            // Prag de 35 pixeli pe orizontala pentru detectie ferma de slide (fara declansari false)
            if (abs(dx) >= 35 && abs(dx) > abs(dy)) {
                gestureTriggered = true;
                if (dx > 0) {
                    // Glisare stanga -> dreapta: Zoom Out (fereastra mai lunga, spre zile)
                    baroGraphZoomOut();
#if DEBUG_BARO
                    Serial.printf("[BARO] Slide dreapta -> Zoom Out: %s (dx=%d)\n",
                                  baroGraphGetSpanName(baroGraphGetTimeSpan()), dx);
#endif
                } else {
                    // Glisare dreapta -> stanga: Zoom In (fereastra mai scurta, spre minute)
                    baroGraphZoomIn();
#if DEBUG_BARO
                    Serial.printf("[BARO] Slide stanga -> Zoom In: %s (dx=%d)\n",
                                  baroGraphGetSpanName(baroGraphGetTimeSpan()), dx);
#endif
                }
                soundPlayFeedbackClick(); // Feedback 200 ms la 45% volum hardware
                updateBaroTimeSpanScale();
                renderBaroCanvas();
                guiInvalidate();
            }
        }
    } else if (code == LV_EVENT_RELEASED) {
        isPressing = false;
        gestureTriggered = false;
    }
}

// ----------------------------------------------------------------------------
// Desenare Grafic Barografic pe Canvas (Transparent, Origine dreapta, punct curent la mijloc Y)
// ----------------------------------------------------------------------------
static void renderBaroCanvas() {
    if (!canvas_baro || !baro_canvas_buf) return;

    const float *history = baroGetPressHistory();
    if (!history) return;

    // Punctul cel mai recent din dreapta este referinta
    float pRef = history[SCREEN_WIDTH - 1];
    if (pRef < 0.0f) {
        for (int x = SCREEN_WIDTH - 1; x >= 0; x--) {
            if (history[x] >= 0.0f) {
                pRef = history[x];
                break;
            }
        }
    }
    if (pRef < 0.0f) return;

    // Culoarea punctelor 32-bit (ARGB8888, Alpha = 255) identica cu cea a ceasului
    uint32_t dotColor32 = 0xFF212121; // Implicit ton inchis pentru afisaj tip LCD
    if (objects.time_hour_minute) {
        lv_color_t tc = lv_obj_get_style_text_color(objects.time_hour_minute, LV_PART_MAIN);
        dotColor32 = 0xFF000000 | lv_color_to_u32(tc);
    }

    bool isHighSens = settingsGetHighSens();
    float scale = isHighSens ? 6.0f : 1.5f;

    // Smart-refresh dirty check: daca datele, scala sau culoarea nu s-au schimbat, nu redesenam
    static float lastPRef = -9999.0f;
    static float lastFirstPt = -9999.0f;
    static bool lastSens = false;
    static uint32_t lastDotColor = 0;
    static uint8_t lastRenderedSpan = 255;
    uint8_t curSpan = baroGraphGetTimeSpan();
    float firstPt = history[SCREEN_WIDTH - BARO_CANVAS_W];
    if (pRef == lastPRef && firstPt == lastFirstPt && isHighSens == lastSens && dotColor32 == lastDotColor && curSpan == lastRenderedSpan) {
        return;
    }
    lastPRef = pRef;
    lastFirstPt = firstPt;
    lastSens = isHighSens;
    lastDotColor = dotColor32;
    lastRenderedSpan = curSpan;

    // Curatare rapida buffer ARGB8888 (Alpha = 0, 100% transparent)
    size_t baro_buf_sz = BARO_CANVAS_W * BARO_CANVAS_H * 4;
    memset(baro_canvas_buf, 0, baro_buf_sz);

    // Limite strict in interiorul containerului de 451x34 px (fara a atinge bordura de 1px)
    const int minX = 1;
    const int maxX = BARO_CANVAS_W - 2; // 449
    const int minY = 1;
    const int maxY = BARO_CANVAS_H - 2; // 32
    const int midY = BARO_CANVAS_H / 2;  // 17 (jumatatea axei verticale)

    // Trasare directa in bufferul PSRAM: aliniere la marginea dreapta a canvas-ului (la maxX = 449)
    uint32_t *px = (uint32_t*)baro_canvas_buf;
    int histOffset = (SCREEN_WIDTH - 1) - maxX;
    for (int x = minX; x <= maxX; x++) {
        int histIdx = x + histOffset;
        if (histIdx >= 0 && histIdx < SCREEN_WIDTH && history[histIdx] >= 0.0f) {
            float delta = history[histIdx] - pRef;
            int yPos = midY - (int)roundf(delta * scale);

            if (yPos >= minY && yPos <= maxY) {
                // Puncte vizibile (2 pixeli pe verticala pentru claritate optima)
                px[yPos * BARO_CANVAS_W + x] = dotColor32;
                if (yPos + 1 <= maxY) {
                    px[(yPos + 1) * BARO_CANVAS_W + x] = dotColor32;
                }
            }
        }
    }
    lv_obj_invalidate(canvas_baro);
}

// ----------------------------------------------------------------------------
// Scala Dinamica de Timp pentru Barograf (lv_scale: 7 Major Ticks pe 451px)
// ----------------------------------------------------------------------------
static char time_span_str[7][12];
static const char *time_span_ptrs[8] = {
    time_span_str[0], time_span_str[1], time_span_str[2],
    time_span_str[3], time_span_str[4], time_span_str[5],
    time_span_str[6],
    NULL
};
static uint8_t lastBaroSpanIdx = 255;

static void updateBaroTimeSpanScale() {
    if (!objects.time_span_scale) return;

    uint8_t spanIdx = baroGraphGetTimeSpan();
    if (spanIdx == lastBaroSpanIdx) return;

    uint32_t totalSpanSec = baroGraphGetSpanSeconds(spanIdx);

    for (int i = 0; i < 7; i++) {
        // i = 0 este stanga (-totalSpanSec), i = 6 este dreapta (0 / acum)
        float absSec = (float)totalSpanSec * (float)(6 - i) / 6.0f;

        if (i == 6 || absSec < 1.0f) {
            snprintf(time_span_str[i], sizeof(time_span_str[i]), "0");
        } else if (totalSpanSec <= 7200) {
            // Pana la 2 ore -> afisare in minute (-Xm)
            int minVal = (int)roundf(absSec / 60.0f);
            snprintf(time_span_str[i], sizeof(time_span_str[i]), "-%dm", minVal);
        } else if (totalSpanSec <= 172800) {
            // Intre 2 ore si 48 ore -> afisare in ore (-Xh sau -X.Xh)
            float hVal = absSec / 3600.0f;
            if (fmodf(hVal, 1.0f) < 0.08f || fmodf(hVal, 1.0f) > 0.92f) {
                snprintf(time_span_str[i], sizeof(time_span_str[i]), "-%dh", (int)roundf(hVal));
            } else {
                snprintf(time_span_str[i], sizeof(time_span_str[i]), "-%.1fh", hVal);
            }
        } else {
            // Peste 48 ore -> afisare in zile (-Xd sau -X.Xd)
            float dVal = absSec / 86400.0f;
            if (fmodf(dVal, 1.0f) < 0.08f || fmodf(dVal, 1.0f) > 0.92f) {
                snprintf(time_span_str[i], sizeof(time_span_str[i]), "-%dd", (int)roundf(dVal));
            } else {
                snprintf(time_span_str[i], sizeof(time_span_str[i]), "-%.1fd", dVal);
            }
        }
    }

    lv_scale_set_label_show(objects.time_span_scale, true);
    lv_scale_set_text_src(objects.time_span_scale, time_span_ptrs);
    lv_obj_invalidate(objects.time_span_scale);
    lastBaroSpanIdx = spanIdx;
}

// ----------------------------------------------------------------------------
// Rasterizator Grafic Monocrom Vectorial pe Buffere ARGB8888
// ----------------------------------------------------------------------------
static inline void setPixel32(uint32_t *buf, int w, int h, int x, int y, uint32_t color) {
    if (x >= 0 && x < w && y >= 0 && y < h) {
        buf[y * w + x] = color;
    }
}

static void drawLine32(uint32_t *buf, int w, int h, int x0, int y0, int x1, int y1, uint32_t color, int thick = 1) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (true) {
        for (int tx = 0; tx < thick; tx++) {
            for (int ty = 0; ty < thick; ty++) {
                setPixel32(buf, w, h, x0 + tx, y0 + ty, color);
            }
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void drawCircleOutline32(uint32_t *buf, int w, int h, int xm, int ym, int r, uint32_t color, int thick = 1) {
    for (int t = 0; t < thick; t++) {
        int curR = r - t;
        if (curR <= 0) break;
        int x = -curR, y = 0, err = 2 - 2 * curR;
        do {
            setPixel32(buf, w, h, xm - x, ym + y, color);
            setPixel32(buf, w, h, xm - y, ym - x, color);
            setPixel32(buf, w, h, xm + x, ym - y, color);
            setPixel32(buf, w, h, xm + y, ym + x, color);
            curR = err;
            if (curR <= y) err += ++y * 2 + 1;
            if (curR > x || err > y) err += ++x * 2 + 1;
        } while (x < 0);
    }
}

static void drawArcOutline32(uint32_t *buf, int w, int h, int xm, int ym, int r, float startDeg, float endDeg, uint32_t color, int thick = 1) {
    float startRad = startDeg * (3.14159265f / 180.0f);
    float endRad   = endDeg * (3.14159265f / 180.0f);
    float step = 1.0f / (float)r;
    for (float a = startRad; a <= endRad; a += step) {
        int px = xm + (int)roundf(cosf(a) * r);
        int py = ym + (int)roundf(sinf(a) * r);
        for (int tx = 0; tx < thick; tx++) {
            for (int ty = 0; ty < thick; ty++) {
                setPixel32(buf, w, h, px + tx, py + ty, color);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Desenare Pictograma Meteo Vectoriala Contur Monocrom pe Canvas
// ----------------------------------------------------------------------------
static int8_t testWeatherIcon = -1;
static unsigned long testWeatherIconTimeout = 0;
static int8_t testTrendArrow = -1;
static unsigned long testTrendArrowTimeout = 0;

void guiSetTestWeatherIcon(int8_t icon) {
    testWeatherIcon = icon;
    testWeatherIconTimeout = (icon >= 0) ? (millis() + 20000) : 0; // 20s test
    if (canvas_wtr) lv_obj_invalidate(canvas_wtr);
}

void guiSetTestTrendArrow(int8_t trend) {
    testTrendArrow = trend;
    testTrendArrowTimeout = (trend >= 0) ? (millis() + 20000) : 0; // 20s test
    if (canvas_trend) lv_obj_invalidate(canvas_trend);
}

static void renderWeatherCanvas() {
    if (!canvas_wtr || !wtr_canvas_buf) return;

    const WeatherForecastInfo& fc = weatherGetForecast();
    WeatherIconType activeIcon = fc.icon;
    if (testWeatherIcon >= 0) {
        if (millis() < testWeatherIconTimeout) {
            activeIcon = (WeatherIconType)testWeatherIcon;
        } else {
            testWeatherIcon = -1;
        }
    }

    uint32_t color = 0xFF212121; // Implicit ton LCD
    if (objects.time_hour_minute) {
        lv_color_t tc = lv_obj_get_style_text_color(objects.time_hour_minute, LV_PART_MAIN);
        color = 0xFF000000 | lv_color_to_u32(tc);
    }

    // Smart-refresh dirty check: daca iconul si culoarea nu s-au schimbat, nu redesenam
    static uint8_t lastIcon = 255;
    static uint32_t lastColor = 0;
    if (activeIcon == lastIcon && color == lastColor) {
        return;
    }
    lastIcon = activeIcon;
    lastColor = color;

    size_t wtr_buf_sz = WTR_CANVAS_W * WTR_CANVAS_H * 4;
    memset(wtr_canvas_buf, 0, wtr_buf_sz);

    uint32_t *px = (uint32_t*)wtr_canvas_buf;
    const int W = WTR_CANVAS_W;
    const int H = WTR_CANVAS_H;
    const int cx = W / 2; // 85
    const int cy = H / 2; // 70

    switch (activeIcon) {
        case WEATHER_ICON_SUN: {
            // Soare cu raze: cerc central si 8 raze
            drawCircleOutline32(px, W, H, cx, cy, 26, color, 2);
            for (int i = 0; i < 8; i++) {
                float ang = i * (3.14159265f / 4.0f);
                int x0 = cx + (int)roundf(cosf(ang) * 33.0f);
                int y0 = cy + (int)roundf(sinf(ang) * 33.0f);
                int x1 = cx + (int)roundf(cosf(ang) * 47.0f);
                int y1 = cy + (int)roundf(sinf(ang) * 47.0f);
                drawLine32(px, W, H, x0, y0, x1, y1, color, 2);
            }
            break;
        }
        case WEATHER_ICON_SUN_CLOUD: {
            // Soare stanga-sus cu raze superioare
            drawCircleOutline32(px, W, H, 62, 48, 18, color, 2);
            for (int i = 3; i <= 7; i++) {
                float ang = i * (3.14159265f / 4.0f);
                int x0 = 62 + (int)roundf(cosf(ang) * 23.0f);
                int y0 = 48 + (int)roundf(sinf(ang) * 23.0f);
                int x1 = 62 + (int)roundf(cosf(ang) * 33.0f);
                int y1 = 48 + (int)roundf(sinf(ang) * 33.0f);
                drawLine32(px, W, H, x0, y0, x1, y1, color, 2);
            }
            // Nor in prim-plan dreapta-jos
            drawLine32(px, W, H, 50, 96, 126, 96, color, 2);
            drawArcOutline32(px, W, H, 64, 82, 14, 180.0f, 330.0f, color, 2);
            drawArcOutline32(px, W, H, 90, 74, 22, 210.0f, 360.0f, color, 2);
            drawArcOutline32(px, W, H, 114, 84, 12, 270.0f, 450.0f, color, 2);
            break;
        }
        case WEATHER_ICON_CLOUD: {
            // Nor contur mare centrat
            drawLine32(px, W, H, 44, 88, 126, 88, color, 2);
            drawArcOutline32(px, W, H, 60, 72, 16, 170.0f, 330.0f, color, 2);
            drawArcOutline32(px, W, H, 86, 62, 26, 210.0f, 360.0f, color, 2);
            drawArcOutline32(px, W, H, 112, 72, 16, 270.0f, 450.0f, color, 2);
            break;
        }
        case WEATHER_ICON_RAIN: {
            // Nor contur pozitionat mai sus
            drawLine32(px, W, H, 44, 74, 126, 74, color, 2);
            drawArcOutline32(px, W, H, 60, 58, 16, 170.0f, 330.0f, color, 2);
            drawArcOutline32(px, W, H, 86, 48, 26, 210.0f, 360.0f, color, 2);
            drawArcOutline32(px, W, H, 112, 58, 16, 270.0f, 450.0f, color, 2);
            // Picaturi oblice de ploaie sub nor
            int dropX[] = { 58, 74, 90, 106 };
            for (int i = 0; i < 4; i++) {
                drawLine32(px, W, H, dropX[i], 86, dropX[i] - 7, 104, color, 2);
                drawLine32(px, W, H, dropX[i] - 3, 112, dropX[i] - 7, 122, color, 2);
            }
            break;
        }
        case WEATHER_ICON_STORM: {
            // Alerta severa: Triunghi echilateral contur cu semnul '!'
            drawLine32(px, W, H, 85, 20, 32, 112, color, 3);
            drawLine32(px, W, H, 85, 20, 138, 112, color, 3);
            drawLine32(px, W, H, 32, 112, 138, 112, color, 3);
            // Semnul '!' in centru
            for (int y = 48; y <= 84; y++) {
                for (int x = 83; x <= 87; x++) {
                    setPixel32(px, W, H, x, y, color);
                }
            }
            for (int y = 96; y <= 100; y++) {
                for (int x = 83; x <= 87; x++) {
                    setPixel32(px, W, H, x, y, color);
                }
            }
            break;
        }
        case WEATHER_ICON_NEW_LOC:
        default: {
            // Achizitie / Calibrare / Tinta
            drawCircleOutline32(px, W, H, cx, cy, 38, color, 2);
            drawCircleOutline32(px, W, H, cx, cy, 22, color, 1);
            drawCircleOutline32(px, W, H, cx, cy, 6, color, 2);
            drawLine32(px, W, H, cx - 46, cy, cx + 46, cy, color, 1);
            drawLine32(px, W, H, cx, cy - 46, cx, cy + 46, color, 1);
            break;
        }
    }

    lv_obj_invalidate(canvas_wtr);
}

// ----------------------------------------------------------------------------
// Desenare Simboluri Dinamica Barica (Tendinta) pe Canvas
// ----------------------------------------------------------------------------
static void renderTrendCanvas() {
    if (!canvas_trend || !trend_canvas_buf) return;

    const WeatherForecastInfo& fc = weatherGetForecast();
    WeatherTrendDir activeTrend = fc.trend;
    if (testTrendArrow >= 0) {
        if (millis() < testTrendArrowTimeout) {
            activeTrend = (WeatherTrendDir)testTrendArrow;
        } else {
            testTrendArrow = -1;
        }
    }

    uint32_t color = 0xFF212121;
    if (objects.time_hour_minute) {
        lv_color_t tc = lv_obj_get_style_text_color(objects.time_hour_minute, LV_PART_MAIN);
        color = 0xFF000000 | lv_color_to_u32(tc);
    }

    // Smart-refresh dirty check: daca trendul si culoarea nu s-au schimbat, nu redesenam
    static int8_t lastTrend = 99;
    static uint32_t lastTrendColor = 0;
    if (activeTrend == lastTrend && color == lastTrendColor) {
        return;
    }
    lastTrend = activeTrend;
    lastTrendColor = color;

    size_t trend_buf_sz = TREND_CANVAS_W * TREND_CANVAS_H * 4;
    memset(trend_canvas_buf, 0, trend_buf_sz);

    uint32_t *px = (uint32_t*)trend_canvas_buf;
    const int W = TREND_CANVAS_W;
    const int H = TREND_CANVAS_H;
    const int cx = W / 2; // 20
    const int cy = H / 2; // 10

    if (activeTrend == TREND_RISING) {
        // Sageata orientata in sus (cresterea presiunii)
        drawLine32(px, W, H, cx, 16, cx, 5, color, 2);
        drawLine32(px, W, H, cx, 2, cx - 7, 9, color, 2);
        drawLine32(px, W, H, cx, 2, cx + 7, 9, color, 2);
        drawLine32(px, W, H, cx - 5, 18, cx + 5, 18, color, 2);
    } else if (activeTrend == TREND_FALLING) {
        // Sageata orientata in jos (scaderea presiunii)
        drawLine32(px, W, H, cx - 5, 2, cx + 5, 2, color, 2);
        drawLine32(px, W, H, cx, 4, cx, 15, color, 2);
        drawLine32(px, W, H, cx, 18, cx - 7, 11, color, 2);
        drawLine32(px, W, H, cx, 18, cx + 7, 11, color, 2);
    } else {
        // TREND_STEADY: Sageata orizontala bidirectionala (<--->)
        drawLine32(px, W, H, 6, cy, 34, cy, color, 2);
        drawLine32(px, W, H, 6, cy, 12, cy - 5, color, 2);
        drawLine32(px, W, H, 6, cy, 12, cy + 5, color, 2);
        drawLine32(px, W, H, 34, cy, 28, cy - 5, color, 2);
        drawLine32(px, W, H, 34, cy, 28, cy + 5, color, 2);
    }

    lv_obj_invalidate(canvas_trend);
}

// ----------------------------------------------------------------------------
// Initializare Modul GUI
// ----------------------------------------------------------------------------
void guiInit() {
#if DEBUG_PONSTEST
    Serial.println("[GUI] Initializare LVGL 9.4.0 & EEZ Studio Bridge...");
#endif

    // Alocare buffere de randare in PSRAM pentru a elibera SRAM intern
    size_t draw_buf_sz  = SCREEN_WIDTH * LVGL_DRAW_BUF_LINES * sizeof(lv_color_t);
    size_t baro_buf_sz  = BARO_CANVAS_W * BARO_CANVAS_H * 4; // Format ARGB8888 (4 bytes/px) pentru transparenta
    size_t wtr_buf_sz   = WTR_CANVAS_W * WTR_CANVAS_H * 4;   // Format ARGB8888
    size_t trend_buf_sz = TREND_CANVAS_W * TREND_CANVAS_H * 4; // Format ARGB8888

    if (psramFound()) {
#if DEBUG_PONSTEST
        Serial.println("[GUI] PSRAM detectat! Alocare buffere grafice in PSRAM.");
#endif
        draw_buf = (lv_color_t*)ps_malloc(draw_buf_sz);
        baro_canvas_buf = (uint8_t*)ps_malloc(baro_buf_sz);
        wtr_canvas_buf = (uint8_t*)ps_malloc(wtr_buf_sz);
        trend_canvas_buf = (uint8_t*)ps_malloc(trend_buf_sz);
    } else {
#if DEBUG_PONSTEST
        Serial.println("[GUI] PSRAM nedisponibil! Alocare in SRAM.");
#endif
        draw_buf = (lv_color_t*)malloc(draw_buf_sz);
        baro_canvas_buf = (uint8_t*)malloc(baro_buf_sz);
        wtr_canvas_buf = (uint8_t*)malloc(wtr_buf_sz);
        trend_canvas_buf = (uint8_t*)malloc(trend_buf_sz);
    }

    lv_init();
    lv_tick_set_cb(my_tick_get_cb);

    // 1. Initializare Touchscreen FT6336
    touchInit();

    // 2. Creare Display LVGL
    lv_display_t *disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, draw_buf, NULL, draw_buf_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, my_disp_flush);

    // 3. Creare Indev Touch LVGL si legare la display
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touch_read);
    lv_indev_set_display(indev, disp);

    // 4. Initializare ecran EEZ Studio
    ui_init();
    if (objects.main) {
        lv_obj_set_style_bg_color(objects.main, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(objects.main, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_flag(objects.main, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(objects.main, on_main_screen_clicked, LV_EVENT_CLICKED, NULL);
        lv_screen_load(objects.main);
        lv_obj_invalidate(objects.main);
    }

    // 5. Configurare Canvas Barograf in interiorul containerului baro_graph (Transparent ARGB8888)
    if (objects.baro_graph) {
        lv_obj_remove_flag(objects.baro_graph, LV_OBJ_FLAG_SCROLLABLE);
        canvas_baro = lv_canvas_create(objects.baro_graph);
        lv_canvas_set_buffer(canvas_baro, baro_canvas_buf, BARO_CANVAS_W, BARO_CANVAS_H, LV_COLOR_FORMAT_ARGB8888);
        lv_obj_set_pos(canvas_baro, 0, 0);
        lv_obj_remove_flag(canvas_baro, LV_OBJ_FLAG_SCROLLABLE);

        // Activare interactiune tactila pentru gesturi orizontale (Zoom In / Zoom Out)
        lv_obj_add_flag(objects.baro_graph, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(objects.baro_graph, 25);
        lv_obj_add_event_cb(objects.baro_graph, on_baro_graph_event, LV_EVENT_ALL, NULL);
        lv_obj_add_flag(canvas_baro, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(canvas_baro, on_baro_graph_event, LV_EVENT_ALL, NULL);
    }
    if (objects.barograph_time_span) {
        lv_obj_add_flag(objects.barograph_time_span, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(objects.barograph_time_span, 15);
        lv_obj_add_event_cb(objects.barograph_time_span, on_baro_graph_event, LV_EVENT_ALL, NULL);
    }
    if (objects.time_span_scale) {
        lv_obj_add_flag(objects.time_span_scale, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(objects.time_span_scale, on_baro_graph_event, LV_EVENT_ALL, NULL);
    }

    // 5b. Configurare etichete dinamice pe scala de timp a barografului
    lastBaroSpanIdx = 255;
    updateBaroTimeSpanScale();

    // 6. Configurare Canvas Meteo in interiorul containerului weather_icon (Transparent ARGB8888)
    if (objects.weather_icon) {
        lv_obj_remove_flag(objects.weather_icon, LV_OBJ_FLAG_SCROLLABLE);
        canvas_wtr = lv_canvas_create(objects.weather_icon);
        lv_canvas_set_buffer(canvas_wtr, wtr_canvas_buf, WTR_CANVAS_W, WTR_CANVAS_H, LV_COLOR_FORMAT_ARGB8888);
        lv_obj_set_pos(canvas_wtr, 0, 0);
        lv_obj_remove_flag(canvas_wtr, LV_OBJ_FLAG_SCROLLABLE);

        // Atasare eveniment atingere pe weather icon (pentru oprire alerte in caz de furtuna)
        lv_obj_add_flag(objects.weather_icon, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(objects.weather_icon, on_weather_icon_clicked, LV_EVENT_CLICKED, NULL);
    }

    // 6b. Configurare Canvas Dinamica Barica in interiorul container_sageti_evolutie_meteo (Transparent ARGB8888)
    if (objects.container_sageti_evolutie_meteo) {
        lv_obj_remove_flag(objects.container_sageti_evolutie_meteo, LV_OBJ_FLAG_SCROLLABLE);
        canvas_trend = lv_canvas_create(objects.container_sageti_evolutie_meteo);
        lv_canvas_set_buffer(canvas_trend, trend_canvas_buf, TREND_CANVAS_W, TREND_CANVAS_H, LV_COLOR_FORMAT_ARGB8888);
        lv_obj_set_pos(canvas_trend, 0, 0);
        lv_obj_remove_flag(canvas_trend, LV_OBJ_FLAG_SCROLLABLE);
    }

    // 7. Atasare evenimente tactile pe indicatorii ALM, CHM, ALRT
    if (objects.watch_alarm_active_indicator) {
        lv_obj_add_flag(objects.watch_alarm_active_indicator, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(objects.watch_alarm_active_indicator, 15);
        lv_obj_add_event_cb(objects.watch_alarm_active_indicator, on_watch_alarm_clicked, LV_EVENT_CLICKED, NULL);
    }
    if (objects.chime_active_indicator) {
        lv_obj_add_flag(objects.chime_active_indicator, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(objects.chime_active_indicator, 15);
        lv_obj_add_event_cb(objects.chime_active_indicator, on_chime_clicked, LV_EVENT_CLICKED, NULL);
    }
    if (objects.weather_alert_active_indicator) {
        lv_obj_add_flag(objects.weather_alert_active_indicator, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(objects.weather_alert_active_indicator, 15);
        lv_obj_add_event_cb(objects.weather_alert_active_indicator, on_weather_alert_clicked, LV_EVENT_CLICKED, NULL);
    }

    if (objects.sync_running) {
        lv_obj_add_flag(objects.sync_running, LV_OBJ_FLAG_HIDDEN);
    }

    // 7. Aplicare tema cromatica salvata in NVS
    guiApplyTheme(settingsGetUiTheme());

#if DEBUG_PONSTEST
    Serial.println("[GUI] LVGL 9 & UI initializate cu succes!");
#endif
}

// Obtinere struct tm in UTC si Ora Locala (LT) direct din Motorul de Ceas (Clock Engine fluid)
static bool getTimeSafe(struct tm &utcTm, struct tm &localTm) {
    return clockGetTime(utcTm, localTm);
}

static bool getLocalTimeSafe(struct tm &localTm) {
    struct tm dummyUtc;
    return clockGetTime(dummyUtc, localTm);
}

// ----------------------------------------------------------------------------
// Verificare Alarme Acustice (Chime & Ceas Desteptator) - apelabil din loop()
// ----------------------------------------------------------------------------
void guiCheckAlarms() {
    struct tm localTm;
    if (!getLocalTimeSafe(localTm)) return;

    // 1. Verificare Chime orar (la minutul 00, declansat o singura data pe ora)
    if (localTm.tm_min == 0) {
        if (lastChimeHour != (unsigned long)localTm.tm_hour) {
            lastChimeHour = (unsigned long)localTm.tm_hour;
            soundPlayChime();
        }
    } else {
        lastChimeHour = 99; // Re-armare pentru urmatoarea ora
    }

    // 2. Verificare Alarma ceas desteptator (pe toata durata minutului setat)
    static bool alarmTriggeredThisEvent = false;
    if (settingsGetAlarmEnabled()) {
        if (localTm.tm_hour == settingsGetAlarmHour() && localTm.tm_min == settingsGetAlarmMinute()) {
            if (!alarmTriggeredThisEvent) {
                alarmTriggeredThisEvent = true;
                uint8_t per = settingsGetAlarmPeriodicity();
                bool shouldRing = false;
                if (per == 0) shouldRing = true; // Zilnic
                else if (per == 1 && (localTm.tm_wday >= 1 && localTm.tm_wday <= 5)) shouldRing = true; // L-V
                else if (per == 2) { 
                    shouldRing = true; 
                    settingsSetAlarmEnabled(false); // O singura data
                }

                if (shouldRing) {
                    Serial.printf("[ALARM] Declansare alarma ceas: %02d:%02d LT (zi=%d, per=%u)\n", 
                                  localTm.tm_hour, localTm.tm_min, localTm.tm_wday, per);
                    soundStartAlarmClock();
                }
            }
        } else {
            // S-a parasit minutul alarmei -> resetam flagul pentru urmatorul eveniment
            alarmTriggeredThisEvent = false;
        }
    } else {
        alarmTriggeredThisEvent = false;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// PRIORITATE 1: Smart Refresh Ceas & Secundar de Timp Real (Edge-Trigger)
// ----------------------------------------------------------------------------
void guiClockTick() {
    struct tm utcTm, localTm;
    if (!getTimeSafe(utcTm, localTm)) {
        return;
    }

    static int lastSec = -1;
    static int lastMin = -1;
    static int lastMday = -1;
    static char lastSyncSrc[16] = "";
    static int lastSlewBlinkPhase = -1;

    bool secChanged = (localTm.tm_sec != lastSec);
    bool minChanged = (localTm.tm_min != lastMin);
    bool dayChanged = (localTm.tm_mday != lastMday);

    // 1. Tranziție secundă (se execută instantaneu pe frontul fiecărei secunde)
    if (secChanged) {
        lastSec = localTm.tm_sec;
        char secBuf[16];
        snprintf(secBuf, sizeof(secBuf), ":%02d", localTm.tm_sec);
        lv_label_set_text(objects.time_seconds, secBuf);

        // Dacă s-a schimbat și minutul
        if (minChanged) {
            lastMin = localTm.tm_min;
            char hmBuf[16];
            snprintf(hmBuf, sizeof(hmBuf), "%02d:%02d", localTm.tm_hour, localTm.tm_min);
            lv_label_set_text(objects.time_hour_minute, hmBuf);

            if (objects.utc_time_hour_minute) {
                char utcBuf[16];
                snprintf(utcBuf, sizeof(utcBuf), "%02d:%02d", utcTm.tm_hour, utcTm.tm_min);
                lv_label_set_text(objects.utc_time_hour_minute, utcBuf);
            }

            // Verificare alarme si chime orar
            guiCheckAlarms();
        }

        // Dacă s-a schimbat ziua
        if (dayChanged) {
            lastMday = localTm.tm_mday;
            static const char* const dayNamesEn[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
            static const char* const monthNamesEn[12] = {"Jan.", "Feb.", "Mar.", "Apr.", "May.", "Jun.", 
                                                         "Jul.", "Aug.", "Sep.", "Oct.", "Nov.", "Dec."};
            if (objects.day_date_label && localTm.tm_wday >= 0 && localTm.tm_wday < 7) {
                lv_label_set_text(objects.day_date_label, dayNamesEn[localTm.tm_wday]);
            }
            if (objects.local_date_label && localTm.tm_mon >= 0 && localTm.tm_mon < 12) {
                char dateBuf[16];
                snprintf(dateBuf, sizeof(dateBuf), "%02d %s", localTm.tm_mday, monthNamesEn[localTm.tm_mon]);
                lv_label_set_text(objects.local_date_label, dateBuf);
            }
        }

        // Flush instantaneu doar pentru dreptunghiul modificat al secundarului
        if (settingsGetDisplayPage() == 0) {
            lv_refr_now(NULL);
        }
    }

    // 2. Sursa de sincronizare (actualizare la orice schimbare a ultimei surse)
    const char* curSyncSrc = clockGetSyncSourceStr();
    if (curSyncSrc && strcmp(curSyncSrc, lastSyncSrc) != 0) {
        strncpy(lastSyncSrc, curSyncSrc, sizeof(lastSyncSrc) - 1);
        lastSyncSrc[sizeof(lastSyncSrc) - 1] = '\0';
        if (objects.sync_source) {
            lv_label_set_text(objects.sync_source, curSyncSrc);
        }
    }

    // 3. Indicator Slew Sincronizare (sync_running / "SYN"): clipire 500ms doar cand este activ Slew
    if (objects.sync_running) {
        if (clockIsSlewing()) {
            int blinkPhase = (millis() / 500) % 2;
            if (blinkPhase != lastSlewBlinkPhase) {
                lastSlewBlinkPhase = blinkPhase;
                if (blinkPhase == 0) {
                    lv_obj_clear_flag(objects.sync_running, LV_OBJ_FLAG_HIDDEN);
                } else {
                    lv_obj_add_flag(objects.sync_running, LV_OBJ_FLAG_HIDDEN);
                }
            }
        } else if (lastSlewBlinkPhase != -1) {
            lastSlewBlinkPhase = -1;
            lv_obj_add_flag(objects.sync_running, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

// ----------------------------------------------------------------------------
// PRIORITATE 2: Smart Refresh Senzori, Rețea & Grafice (Dirty-Check)
// ----------------------------------------------------------------------------
void guiUpdateSensors() {
    const GpsData &g = gpsGetData();
    const BmeData &b = bmeGetData();
    const WeatherForecastInfo &fc = weatherGetForecast();

    // 1. Pictograma receptor GNSS (fix / no fix)
    static bool lastFixState = false;
    bool curFixState = (g.present && g.fix);
    if (curFixState != lastFixState) {
        lastFixState = curFixState;
        if (objects.gps_icon) {
            if (curFixState) lv_obj_add_state(objects.gps_icon, LV_STATE_CHECKED);
            else lv_obj_remove_state(objects.gps_icon, LV_STATE_CHECKED);
        }
    }

    // 2. Maidenhead Locator & Sursa Pozitie (GPS / USR)
    static char lastLocator[8] = "";
    if (strncmp(lastLocator, g.maidenhead, sizeof(lastLocator)) != 0) {
        strncpy(lastLocator, g.maidenhead, sizeof(lastLocator) - 1);
        if (objects.maidenhead) {
            lv_label_set_text(objects.maidenhead, g.maidenhead);
        }
    }
    static bool lastGpsPosSrc = false;
    if (curFixState != lastGpsPosSrc) {
        lastGpsPosSrc = curFixState;
        if (objects.pos_source_label) {
            lv_label_set_text(objects.pos_source_label, curFixState ? "GPS" : "USR");
        }
    }

    // 3. Presiune & Unitate
    static float lastPressure = -999.0f;
    static uint8_t lastPUnit = 255;
    uint8_t pUnit = settingsGetPressUnit();
    if (fabsf(b.pressureCalibratedMb - lastPressure) > 0.05f || pUnit != lastPUnit) {
        lastPressure = b.pressureCalibratedMb;
        lastPUnit = pUnit;
        if (objects.pressure_value) {
            char pBuf[16];
            if (pUnit == 1) snprintf(pBuf, sizeof(pBuf), "%.1f", b.pressureCalibratedMb);
            else if (pUnit == 2) snprintf(pBuf, sizeof(pBuf), "%.1f", b.pressureCalibratedMb * 0.750062f);
            else snprintf(pBuf, sizeof(pBuf), "%.1f", b.pressureCalibratedMb);
            lv_label_set_text(objects.pressure_value, pBuf);
        }
        if (objects.pressure_unit) {
            const char *unitStr = (pUnit == 1) ? "hPa" : ((pUnit == 2) ? "mmHg" : "mb");
            lv_label_set_text(objects.pressure_unit, unitStr);
        }
    }

    // 4. Temperatura interioara (BMP280)
    static float lastTemp = -999.0f;
    static bool lastIsFahr = false;
    bool isFahr = settingsGetTempUnitF();
    float tVal = isFahr ? b.tempF : b.tempC;
    if (fabsf(tVal - lastTemp) > 0.05f || isFahr != lastIsFahr) {
        lastTemp = tVal;
        lastIsFahr = isFahr;
        if (objects.interior_temp) {
            char tBuf[16];
            snprintf(tBuf, sizeof(tBuf), "%.1f", tVal);
            lv_label_set_text(objects.interior_temp, tBuf);
        }
        if (objects.temp_unit_indoor) {
            lv_label_set_text(objects.temp_unit_indoor, isFahr ? "F." : "C.");
        }
    }

    // 4b. Temperatura exterioara (Senzor Slot 1 din ext_sensors)
    ExtSensorSlot extSlot1;
    bool hasExt1 = extSensorGetSlot(1, extSlot1);

    static String lastExtLabel1 = "";
    static String lastExtVal1 = "";
    static String lastExtUnit1 = "";

    String curExtLabel1 = "--";
    String curExtVal1 = "--";
    String curExtUnit1 = isFahr ? "F." : "C.";

    if (hasExt1 && extSlot1.isAssigned) {
        curExtLabel1 = (strlen(extSlot1.alias) > 0) ? String(extSlot1.alias) : String(extSlot1.id);

        if (!extSlot1.hasData) {
            curExtVal1 = "--.-";
        } else if (!extSlot1.isOnline) {
            curExtVal1 = "OFF";
            curExtUnit1 = "";
        } else {
            float extVal = isFahr ? (extSlot1.lastTempC * 1.8f + 32.0f) : extSlot1.lastTempC;
            char extBuf[16];
            snprintf(extBuf, sizeof(extBuf), "%.1f", extVal);
            curExtVal1 = String(extBuf);
        }
    }

    if (curExtLabel1 != lastExtLabel1) {
        lastExtLabel1 = curExtLabel1;
        if (objects.temp_label_1) lv_label_set_text(objects.temp_label_1, curExtLabel1.c_str());
    }
    if (curExtVal1 != lastExtVal1) {
        lastExtVal1 = curExtVal1;
        if (objects.ext_temp_1) lv_label_set_text(objects.ext_temp_1, curExtVal1.c_str());
    }
    if (curExtUnit1 != lastExtUnit1) {
        lastExtUnit1 = curExtUnit1;
        if (objects.temp_unit_outd_1) lv_label_set_text(objects.temp_unit_outd_1, curExtUnit1.c_str());
    }

    // 5. Header WiFi & Sateliti
    static String lastSsid = "";
    String curSsid = wifiMgrIsAP() ? "AP: BaroClock" : wifiMgrGetShortSSID();
    if (curSsid != lastSsid) {
        lastSsid = curSsid;
        if (objects.wifi_info) lv_label_set_text(objects.wifi_info, curSsid.c_str());
    }

    static String lastIp = "";
    String curIp = wifiMgrGetIP();
    if (curIp != lastIp) {
        lastIp = curIp;
        if (objects.ip_address_info) lv_label_set_text(objects.ip_address_info, curIp.c_str());
    }

    static uint8_t lastSatsVis = 255, lastSatsFix = 255;
    const GnssSatellitesData &sats = gpsGetSatellitesData();
    if (sats.satsVisible != lastSatsVis || sats.satsInFix != lastSatsFix) {
        lastSatsVis = sats.satsVisible;
        lastSatsFix = sats.satsInFix;
        if (objects.sat_no_info) {
            char satBuf[16];
            if (g.present) snprintf(satBuf, sizeof(satBuf), "%02d/%02d", sats.satsVisible, sats.satsInFix);
            else snprintf(satBuf, sizeof(satBuf), "--/--");
            lv_label_set_text(objects.sat_no_info, satBuf);
        }
    }

    // 6. Indicatori Stare Alerte (ALM, CHM, ALRT)
    static int8_t lastChm = -1, lastAlm = -1, lastAlrt = -1;
    int8_t curChm = settingsGetChimeEnabled() ? 1 : 0;
    if (curChm != lastChm) {
        lastChm = curChm;
        if (objects.chime_active_indicator) {
            if (curChm) lv_obj_add_state(objects.chime_active_indicator, LV_STATE_CHECKED);
            else lv_obj_remove_state(objects.chime_active_indicator, LV_STATE_CHECKED);
        }
    }
    int8_t curAlm = settingsGetAlarmEnabled() ? 1 : 0;
    if (curAlm != lastAlm) {
        lastAlm = curAlm;
        if (objects.watch_alarm_active_indicator) {
            if (curAlm) lv_obj_add_state(objects.watch_alarm_active_indicator, LV_STATE_CHECKED);
            else lv_obj_remove_state(objects.watch_alarm_active_indicator, LV_STATE_CHECKED);
        }
    }
    int8_t curAlrt = settingsGetStormAlertEnabled() ? 1 : 0;
    if (curAlrt != lastAlrt) {
        lastAlrt = curAlrt;
        if (objects.weather_alert_active_indicator) {
            if (curAlrt) lv_obj_add_state(objects.weather_alert_active_indicator, LV_STATE_CHECKED);
            else lv_obj_remove_state(objects.weather_alert_active_indicator, LV_STATE_CHECKED);
        }
    }

    // 7. Prognoza Meteo
    if (objects.forecast_text) {
        static String lastStatus = "";
        String curStatus = String(fc.statusText);
        if (curStatus != lastStatus) {
            lastStatus = curStatus;
            lv_label_set_text(objects.forecast_text, curStatus.c_str());
        }
    }

    // 8. Randare canvas-uri si actualizare scala de timp
    updateBaroTimeSpanScale();
    renderBaroCanvas();
    renderWeatherCanvas();
    renderTrendCanvas();

    // 9. Verificare automata alerte meteo la schimbarea riscului
    static uint8_t lastWeatherRisk = 255;
    if (fc.risk != lastWeatherRisk) {
        if (fc.risk == RISK_DETERIORATION) {
            soundPlayWeatherDeterioration();
        } else if (fc.risk == RISK_STORM) {
            soundStartStormAlert();
        }
        lastWeatherRisk = fc.risk;
    }
}

// Actualizare completa (folosita la initializare sau trecerea pe ecranul principal)
void guiUpdate() {
    guiClockTick();
    guiUpdateSensors();
}

// Tick periodic apelat din loop() pentru animatii si refresh LVGL
void guiTick() {
    lv_timer_handler();
}

// Fortare invalidare si re-randare ecran principal
void guiInvalidate() {
    if (objects.main) {
        lv_obj_invalidate(objects.main);
    }
}

