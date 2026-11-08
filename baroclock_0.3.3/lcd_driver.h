#pragma once
#include <Arduino.h>
#include "setup.h"

void lcdInitHardware(bool rotate180);
void lcdSetRotation(bool rotate180);

void lcdFillScreen(uint16_t color);
void lcdFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void lcdDrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void lcdDrawPixel(int16_t x, int16_t y, uint16_t color);
void lcdDrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

void lcdDrawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t scale);
void lcdDrawString(int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t scale);
void lcdDrawString(int16_t x, int16_t y, const String &str, uint16_t color, uint16_t bg, uint8_t scale);

// Specialized UI draw functions
void lcdDrawSatIcon(int16_t x, int16_t y, uint16_t color, uint16_t bg);
void lcdDrawBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pixels);

