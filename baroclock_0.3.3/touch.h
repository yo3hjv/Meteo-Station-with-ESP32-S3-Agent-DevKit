#pragma once
#include <Arduino.h>

void touchInit();
bool touchReadRaw(uint16_t &x, uint16_t &y);
bool touchGetTap(uint16_t &x, uint16_t &y);

