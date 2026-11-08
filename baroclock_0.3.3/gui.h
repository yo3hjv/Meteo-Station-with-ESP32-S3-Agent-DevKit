#pragma once
#include <Arduino.h>

void guiInit();
void guiUpdate();
void guiClockTick();      // Prioritate 1: Ceas si secundar de timp real (edge-trigger la ms exacta)
void guiUpdateSensors();  // Prioritate 2: Senzori, diagnoza si canvas-uri (smart dirty-check)
void guiTick();
void guiInvalidate();
void guiCheckAlarms();

// Testare vizuala pe ecran din Web UI (-1 = mod normal/live)
void guiSetTestWeatherIcon(int8_t icon);
void guiSetTestTrendArrow(int8_t trend);

// Profiluri Cromatice (Teme) LVGL
void guiApplyTheme(uint8_t themeIndex);
uint8_t guiGetCurrentTheme();

