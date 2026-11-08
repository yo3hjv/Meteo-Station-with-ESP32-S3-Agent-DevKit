#pragma once
#include <Arduino.h>
#include "setup.h"

void soundInit();
void soundPlayBeep(uint16_t freqHz = BEEP_FREQUENCY_HZ, uint16_t durationMs = BEEP_DURATION_MS);
void soundPlayFeedbackClick(); // Click feedback tactil: minim 200 ms, volum 45%
void soundSetPa(bool enable);
void soundSetVolume(uint8_t volumePercent);
uint8_t soundGetVolume();

// Semnalizari Acustice & Alarme specifice
void soundPlayChime();

void soundStartAlarmClock();
void soundStopAlarmClock();
bool soundIsAlarmClockRinging();

void soundPlayWeatherDeterioration();

void soundStartStormAlert();
void soundStopStormAlert();
bool soundIsStormAlertRinging();

void soundStopAllAlarms();


