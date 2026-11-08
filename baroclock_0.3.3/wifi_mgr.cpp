#include "wifi_mgr.h"
#include "setup.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>

static Preferences prefs;
static bool isAPMode = false;
static String connectedSSID = "";

void wifiMgrInit() {
    pinMode(PIN_WIFI_RESET_BOOT, INPUT_PULLUP);
    delay(50);

    prefs.begin("baroclock", false);

    // Q&A 7: GPIO46 pullup, checked at boot: if pressed to GND, reset WiFi and start in AP
    if (digitalRead(PIN_WIFI_RESET_BOOT) == LOW) {
        Serial.println("[WIFI] GPIO46 LOW detected at boot! Clearing WiFi credentials...");
        wifiMgrResetAll();
        delay(500);
    }
}

void wifiMgrGetSlot(uint8_t slot, String &ssid, String &pass) {
    if (slot < 1 || slot > 3) return;
    char keyS[16], keyP[16];
    snprintf(keyS, sizeof(keyS), "ssid%d", slot);
    snprintf(keyP, sizeof(keyP), "pass%d", slot);
    ssid = prefs.getString(keyS, "");
    pass = prefs.getString(keyP, "");
}

void wifiMgrSaveSlot(uint8_t slot, const String &ssid, const String &pass) {
    if (slot < 1 || slot > 3) return;
    char keyS[16], keyP[16];
    snprintf(keyS, sizeof(keyS), "ssid%d", slot);
    snprintf(keyP, sizeof(keyP), "pass%d", slot);
    prefs.putString(keyS, ssid);
    prefs.putString(keyP, pass);
    Serial.printf("[WIFI] Saved Slot %d: %s\n", slot, ssid.c_str());
}

void wifiMgrResetSlot(uint8_t slot) {
    if (slot < 1 || slot > 3) return;
    char keyS[16], keyP[16];
    snprintf(keyS, sizeof(keyS), "ssid%d", slot);
    snprintf(keyP, sizeof(keyP), "pass%d", slot);
    prefs.remove(keyS);
    prefs.remove(keyP);
    Serial.printf("[WIFI] Reset Slot %d\n", slot);
}

void wifiMgrResetAll() {
    for (uint8_t i = 1; i <= 3; i++) {
        wifiMgrResetSlot(i);
    }
}

void wifiMgrStartAP() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID_DEFAULT, AP_PASS_DEFAULT);
    isAPMode = true;
    connectedSSID = AP_SSID_DEFAULT;
#if DEBUG_PONSTEST
    Serial.printf("[WIFI] Started AP '%s', IP: %s\n", AP_SSID_DEFAULT, WiFi.softAPIP().toString().c_str());
#endif
}

bool wifiMgrConnect() {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname("baroclock");

    for (uint8_t slot = 1; slot <= 3; slot++) {
        String s, p;
        wifiMgrGetSlot(slot, s, p);
        if (s.length() > 0) {
#if DEBUG_PONSTEST
            Serial.printf("[WIFI] Trying Slot %d: '%s'...\n", slot, s.c_str());
#endif
            WiFi.begin(s.c_str(), p.c_str());

            unsigned long startMs = millis();
            while (WiFi.status() != WL_CONNECTED && (millis() - startMs < 8000)) {
                delay(250);
#if DEBUG_PONSTEST
                Serial.print(".");
#endif
            }
#if DEBUG_PONSTEST
            Serial.println();
#endif

            if (WiFi.status() == WL_CONNECTED) {
                isAPMode = false;
                connectedSSID = s;

                // Pornire serviciu mDNS pentru acces direct prin http://baroclock.local
                if (MDNS.begin("baroclock")) {
                    MDNS.addService("http", "tcp", 80);
#if DEBUG_PONSTEST
                    Serial.println("[mDNS] Responder started: http://baroclock.local sau http://baroclock/");
#endif
                }

#if DEBUG_PONSTEST
                Serial.printf("[WIFI] Connected to '%s'! IP: %s\n", s.c_str(), WiFi.localIP().toString().c_str());
#endif
                return true;
            } else {
#if DEBUG_PONSTEST
                Serial.printf("[WIFI] Slot %d connection failed.\n", slot);
#endif
            }
        }
    }

#if DEBUG_PONSTEST
    Serial.println("[WIFI] No WiFi network available or login failed. Falling back to AP mode...");
#endif
    wifiMgrStartAP();
    return false;
}

bool wifiMgrIsAP() {
    return isAPMode;
}

String wifiMgrGetIP() {
    if (isAPMode) return WiFi.softAPIP().toString();
    return WiFi.localIP().toString();
}

String wifiMgrGetSSID() {
    return connectedSSID;
}

String wifiMgrGetShortSSID() {
    if (connectedSSID.length() <= 5) return connectedSSID;
    return connectedSSID.substring(0, 5);
}

int32_t settingsGetTimezoneMin() {
    return prefs.getInt("tz_min", DEFAULT_TIMEZONE_MIN);
}

void settingsSetTimezoneMin(int32_t offsetMin) {
    prefs.putInt("tz_min", offsetMin);
}

bool settingsGetDst() {
    return prefs.getBool("dst", DEFAULT_DST);
}

void settingsSetDst(bool dstActive) {
    prefs.putBool("dst", dstActive);
}

bool settingsGetRotate180() {
    return prefs.getBool("rot180", DEFAULT_ROTATION_180);
}

void settingsSetRotate180(bool rot) {
    prefs.putBool("rot180", rot);
}

bool settingsGetTempUnitF() {
    return prefs.getBool("temp_f", DEFAULT_TEMP_UNIT_F);
}

void settingsSetTempUnitF(bool isFahrenheit) {
    prefs.putBool("temp_f", isFahrenheit);
}

uint8_t settingsGetPressUnit() {
    return prefs.getUChar("p_unit", 0); // Default: 0 = mb
}

void settingsSetPressUnit(uint8_t unit) {
    if (unit > 2) unit = 0;
    prefs.putUChar("p_unit", unit);
}

bool settingsGetHighSens() {
    return prefs.getBool("high_sens", DEFAULT_HIGH_SENS);
}

void settingsSetHighSens(bool highSens) {
    prefs.putBool("high_sens", highSens);
}

float settingsGetBaroIntervalSec() {
    return prefs.getFloat("baro_sec", DEFAULT_BARO_INTERVAL_SEC);
}

void settingsSetBaroIntervalSec(float intervalSec) {
    if (intervalSec < 0.1f) intervalSec = 0.1f;
    if (intervalSec > 300.0f) intervalSec = 300.0f; // max 5 min
    prefs.putFloat("baro_sec", intervalSec);
}

uint8_t settingsGetBaroTimeWindow() {
    return prefs.getUChar("baro_win", 6); // Implicit 24 ore (index 6: BARO_SPAN_24H)
}

void settingsSetBaroTimeWindow(uint8_t win) {
    if (win >= 10) win = 9; // maxim 14 zile (index 9)
    prefs.putUChar("baro_win", win);
}

uint32_t settingsGetForecastIntervalMin() {
    return prefs.getUInt("fc_min", DEFAULT_FORECAST_INT_MIN);
}

void settingsSetForecastIntervalMin(uint32_t intervalMin) {
    if (intervalMin < 1) intervalMin = 1;
    if (intervalMin > 30) intervalMin = 30; // max 30 min
    prefs.putUInt("fc_min", intervalMin);
}

float settingsGetQnhOffset() {
    return prefs.getFloat("qnh_off", 0.0f);
}

void settingsSetQnhOffset(float offsetHpa) {
    prefs.putFloat("qnh_off", offsetHpa);
}

float settingsGetTempOffset() {
    return prefs.getFloat("temp_off", 0.0f);
}

void settingsSetTempOffset(float offsetC) {
    prefs.putFloat("temp_off", offsetC);
}

String settingsGetManualLocator() {
    return prefs.getString("man_loc", "");
}

void settingsSetManualLocator(const String &loc) {
    prefs.putString("man_loc", loc);
}

uint8_t settingsGetWeatherBaseRef() {
    return prefs.getUChar("base_wtr", 0);
}

void settingsSetWeatherBaseRef(uint8_t baseRef) {
    if (baseRef > 2) baseRef = 0;
    prefs.putUChar("base_wtr", baseRef);
}

bool settingsGetSoundTestEnabled() {
    return prefs.getBool("snd_test", BEEP_TEST_DEFAULT_ON);
}

void settingsSetSoundTestEnabled(bool en) {
    prefs.putBool("snd_test", en);
}

uint8_t settingsGetAudioVolume() {
    return prefs.getUChar("snd_vol", BEEP_VOLUME_PERCENT);
}

void settingsSetAudioVolume(uint8_t vol) {
    if (vol > 100) vol = 100;
    prefs.putUChar("snd_vol", vol);
}

uint8_t settingsGetDisplayPage() {
    return prefs.getUChar("disp_page", DEFAULT_DISPLAY_PAGE);
}

void settingsSetDisplayPage(uint8_t page) {
    prefs.putUChar("disp_page", page);
}

uint8_t settingsGetUiTheme() {
    uint8_t thm = prefs.getUChar("ui_thm", DEFAULT_UI_THEME);
    if (thm >= UI_THEME_COUNT) thm = DEFAULT_UI_THEME;
    return thm;
}

void settingsSetUiTheme(uint8_t theme) {
    if (theme >= UI_THEME_COUNT) theme = DEFAULT_UI_THEME;
    prefs.putUChar("ui_thm", theme);
}

uint8_t settingsGetGnssMode() {
    return prefs.getUChar("gnss_mode", DEFAULT_GNSS_MODE);
}

void settingsSetGnssMode(uint8_t mode) {
    if (mode < 1 || mode > 3) mode = DEFAULT_GNSS_MODE;
    prefs.putUChar("gnss_mode", mode);
}

bool settingsGetNmeaEssential() {
    return prefs.getBool("nmea_ess", DEFAULT_NMEA_ESSENTIAL);
}

void settingsSetNmeaEssential(bool en) {
    prefs.putBool("nmea_ess", en);
}

bool settingsGetNmeaSerialDebug() {
    return prefs.getBool("nmea_dbg", DEFAULT_NMEA_SERIAL_DEBUG);
}

void settingsSetNmeaSerialDebug(bool en) {
    prefs.putBool("nmea_dbg", en);
}

// ----------------------------------------------------------------------------
// Semnalizari Acustice & Alarme
// ----------------------------------------------------------------------------
bool settingsGetChimeEnabled() {
    return prefs.getBool("chime_en", CHIME_DEFAULT_ENABLED);
}
void settingsSetChimeEnabled(bool en) {
    prefs.putBool("chime_en", en);
}
uint16_t settingsGetChimeFreq() {
    return prefs.getUShort("chime_fq", CHIME_FREQUENCY_HZ);
}
void settingsSetChimeFreq(uint16_t f) {
    prefs.putUShort("chime_fq", f);
}
uint16_t settingsGetChimeDuration() {
    return prefs.getUShort("chime_dur", CHIME_DURATION_MS);
}
void settingsSetChimeDuration(uint16_t d) {
    prefs.putUShort("chime_dur", d);
}
uint16_t settingsGetChimePause() {
    return prefs.getUShort("chime_pause", CHIME_PAUSE_MS);
}
void settingsSetChimePause(uint16_t p) {
    prefs.putUShort("chime_pause", p);
}
uint8_t settingsGetChimeVolume() {
    return prefs.getUChar("chime_vol", CHIME_VOLUME_PERCENT);
}
void settingsSetChimeVolume(uint8_t v) {
    if (v > 100) v = 100;
    prefs.putUChar("chime_vol", v);
}

bool settingsGetAlarmEnabled() {
    return prefs.getBool("alm_en", ALARM_DEFAULT_ENABLED);
}
void settingsSetAlarmEnabled(bool en) {
    prefs.putBool("alm_en", en);
}
uint8_t settingsGetAlarmHour() {
    return prefs.getUChar("alm_hr", ALARM_DEFAULT_HOUR);
}
void settingsSetAlarmHour(uint8_t h) {
    if (h > 23) h = 0;
    prefs.putUChar("alm_hr", h);
}
uint8_t settingsGetAlarmMinute() {
    return prefs.getUChar("alm_min", ALARM_DEFAULT_MINUTE);
}
void settingsSetAlarmMinute(uint8_t m) {
    if (m > 59) m = 0;
    prefs.putUChar("alm_min", m);
}
uint8_t settingsGetAlarmPeriodicity() {
    return prefs.getUChar("alm_per", ALARM_DEFAULT_PERIODICITY);
}
void settingsSetAlarmPeriodicity(uint8_t p) {
    if (p > 2) p = 0;
    prefs.putUChar("alm_per", p);
}
uint16_t settingsGetAlarmFreq() {
    return prefs.getUShort("alm_fq", ALARM_FREQUENCY_HZ);
}
void settingsSetAlarmFreq(uint16_t f) {
    prefs.putUShort("alm_fq", f);
}
uint8_t settingsGetAlarmVolume() {
    return prefs.getUChar("alm_vol", ALARM_VOLUME_PERCENT);
}
void settingsSetAlarmVolume(uint8_t v) {
    if (v > 100) v = 100;
    prefs.putUChar("alm_vol", v);
}

bool settingsGetWtrDetAlertEnabled() {
    return prefs.getBool("wtr_det_en", WEATHER_ALERT_DEFAULT_EN);
}
void settingsSetWtrDetAlertEnabled(bool en) {
    prefs.putBool("wtr_det_en", en);
}
uint16_t settingsGetWtrDetFreq() {
    return prefs.getUShort("wtr_det_fq", WTR_DET_FREQUENCY_HZ);
}
void settingsSetWtrDetFreq(uint16_t f) {
    prefs.putUShort("wtr_det_fq", f);
}
uint8_t settingsGetWtrDetVolume() {
    return prefs.getUChar("wtr_det_vol", WTR_DET_VOLUME_PERCENT);
}
void settingsSetWtrDetVolume(uint8_t v) {
    if (v > 100) v = 100;
    prefs.putUChar("wtr_det_vol", v);
}

bool settingsGetStormAlertEnabled() {
    return prefs.getBool("storm_en", STORM_ALERT_DEFAULT_EN);
}
void settingsSetStormAlertEnabled(bool en) {
    prefs.putBool("storm_en", en);
}
uint16_t settingsGetStormFreq() {
    return prefs.getUShort("storm_fq", STORM_FREQUENCY_HZ);
}
void settingsSetStormFreq(uint16_t f) {
    prefs.putUShort("storm_fq", f);
}
uint8_t settingsGetStormVolume() {
    return prefs.getUChar("storm_vol", STORM_VOLUME_PERCENT);
}
void settingsSetStormVolume(uint8_t v) {
    if (v > 100) v = 100;
    prefs.putUChar("storm_vol", v);
}

bool settingsGetFeedbackEnabled() {
    return prefs.getBool("fb_en", FEEDBACK_DEFAULT_ENABLED);
}
void settingsSetFeedbackEnabled(bool en) {
    prefs.putBool("fb_en", en);
}

uint16_t settingsGetFeedbackFreq() {
    return prefs.getUShort("fb_fq", FEEDBACK_FREQUENCY_HZ);
}
void settingsSetFeedbackFreq(uint16_t f) {
    if (f < 500) f = 500;
    if (f > 5000) f = 5000;
    prefs.putUShort("fb_fq", f);
}

uint16_t settingsGetFeedbackDuration() {
    return prefs.getUShort("fb_dur", FEEDBACK_DURATION_MS);
}
void settingsSetFeedbackDuration(uint16_t d) {
    if (d < 20) d = 20;
    if (d > 1000) d = 1000;
    prefs.putUShort("fb_dur", d);
}

uint8_t settingsGetFeedbackVolume() {
    return prefs.getUChar("fb_vol", FEEDBACK_VOLUME_PERCENT);
}
void settingsSetFeedbackVolume(uint8_t v) {
    if (v > 100) v = 100;
    prefs.putUChar("fb_vol", v);
}





