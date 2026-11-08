#pragma once
#include <Arduino.h>

struct WiFiCred {
    char ssid[33];
    char pass[65];
};

void wifiMgrInit();
bool wifiMgrConnect();
void wifiMgrStartAP();
bool wifiMgrIsAP();
String wifiMgrGetIP();
String wifiMgrGetSSID();
String wifiMgrGetShortSSID(); // First 5 chars as requested in FSD

void wifiMgrGetSlot(uint8_t slot, String &ssid, String &pass);
void wifiMgrSaveSlot(uint8_t slot, const String &ssid, const String &pass);
void wifiMgrResetSlot(uint8_t slot);
void wifiMgrResetAll();

// Settings stored in Preferences
int32_t settingsGetTimezoneMin();
void settingsSetTimezoneMin(int32_t offsetMin);

bool settingsGetDst();
void settingsSetDst(bool dstActive);

bool settingsGetRotate180();
void settingsSetRotate180(bool rot);

bool settingsGetTempUnitF();
void settingsSetTempUnitF(bool isFahrenheit);

uint8_t settingsGetPressUnit(); // 0 = mb, 1 = hPa, 2 = mmHg
void settingsSetPressUnit(uint8_t unit);

bool settingsGetHighSens();
void settingsSetHighSens(bool highSens);

float settingsGetBaroIntervalSec();
void settingsSetBaroIntervalSec(float intervalSec);

uint8_t settingsGetBaroTimeWindow();
void settingsSetBaroTimeWindow(uint8_t win);

uint32_t settingsGetForecastIntervalMin();
void settingsSetForecastIntervalMin(uint32_t intervalMin);

float settingsGetQnhOffset();
void settingsSetQnhOffset(float offsetHpa);

float settingsGetTempOffset();
void settingsSetTempOffset(float offsetC);

String settingsGetManualLocator();
void settingsSetManualLocator(const String &loc);

uint8_t settingsGetWeatherBaseRef(); // 0 = Senin, 1 = Innorat, 2 = Ploaie
void settingsSetWeatherBaseRef(uint8_t baseRef);

bool settingsGetSoundTestEnabled();
void settingsSetSoundTestEnabled(bool en);

uint8_t settingsGetAudioVolume();
void settingsSetAudioVolume(uint8_t vol);

uint8_t settingsGetDisplayPage();
void settingsSetDisplayPage(uint8_t page);

uint8_t settingsGetUiTheme();
void settingsSetUiTheme(uint8_t theme);

uint8_t settingsGetGnssMode();
void settingsSetGnssMode(uint8_t mode);

bool settingsGetNmeaEssential();
void settingsSetNmeaEssential(bool en);

bool settingsGetNmeaSerialDebug();
void settingsSetNmeaSerialDebug(bool en);

// Setari Semnalizari Acustice & Alarme
bool settingsGetChimeEnabled();
void settingsSetChimeEnabled(bool en);
uint16_t settingsGetChimeFreq();
void settingsSetChimeFreq(uint16_t f);
uint16_t settingsGetChimeDuration();
void settingsSetChimeDuration(uint16_t d);
uint16_t settingsGetChimePause();
void settingsSetChimePause(uint16_t p);
uint8_t settingsGetChimeVolume();
void settingsSetChimeVolume(uint8_t v);

bool settingsGetAlarmEnabled();
void settingsSetAlarmEnabled(bool en);
uint8_t settingsGetAlarmHour();
void settingsSetAlarmHour(uint8_t h);
uint8_t settingsGetAlarmMinute();
void settingsSetAlarmMinute(uint8_t m);
uint8_t settingsGetAlarmPeriodicity(); // 0 = Zilnic, 1 = Luni-Vineri, 2 = O singura data
void settingsSetAlarmPeriodicity(uint8_t p);
uint16_t settingsGetAlarmFreq();
void settingsSetAlarmFreq(uint16_t f);
uint8_t settingsGetAlarmVolume();
void settingsSetAlarmVolume(uint8_t v);

bool settingsGetWtrDetAlertEnabled();
void settingsSetWtrDetAlertEnabled(bool en);
uint16_t settingsGetWtrDetFreq();
void settingsSetWtrDetFreq(uint16_t f);
uint8_t settingsGetWtrDetVolume();
void settingsSetWtrDetVolume(uint8_t v);

bool settingsGetStormAlertEnabled();
void settingsSetStormAlertEnabled(bool en);
uint16_t settingsGetStormFreq();
void settingsSetStormFreq(uint16_t f);
uint8_t settingsGetStormVolume();
void settingsSetStormVolume(uint8_t v);

bool settingsGetFeedbackEnabled();
void settingsSetFeedbackEnabled(bool en);
uint16_t settingsGetFeedbackFreq();
void settingsSetFeedbackFreq(uint16_t f);
uint16_t settingsGetFeedbackDuration();
void settingsSetFeedbackDuration(uint16_t d);
uint8_t settingsGetFeedbackVolume();
void settingsSetFeedbackVolume(uint8_t v);




