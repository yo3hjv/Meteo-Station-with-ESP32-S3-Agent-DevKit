#include "touch.h"
#include "setup.h"
#include <Wire.h>

static uint8_t activeTouchAddr = 0x38;
static bool s_wasPressed = false;
static uint32_t s_lastTouchTime = 0;

void touchInit() {
    pinMode(TOUCH_INT_PIN, INPUT_PULLUP);

    // Auto-detectare adresa I2C: 0x38 (implicit FT6336) sau 0x39 (in functie de strapping hardware)
    Wire.beginTransmission(0x38);
    if (Wire.endTransmission() == 0) {
        activeTouchAddr = 0x38;
#if DEBUG_PONSTEST
        Serial.println("[TOUCH] FT6336 touchscreen detectat la adresa I2C 0x38.");
#endif
    } else {
        Wire.beginTransmission(0x39);
        if (Wire.endTransmission() == 0) {
            activeTouchAddr = 0x39;
#if DEBUG_PONSTEST
            Serial.println("[TOUCH] FT6336 touchscreen detectat la adresa I2C 0x39.");
#endif
        } else {
            Serial.println("[TOUCH] AVERTISMENT: FT6336 nu raspunde la 0x38 si nici la 0x39!");
            activeTouchAddr = 0x38;
            return;
        }
    }

    // Setare mod normal de functionare (Device Mode = 0x00)
    Wire.beginTransmission(activeTouchAddr);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.endTransmission();
}

bool touchReadRaw(uint16_t &x, uint16_t &y) {
    // Pas 1: Citire numar puncte atinse din TD_STATUS (0x02) - exact ca in Reveil-CYDGOLD / FT6336.cpp
    Wire.beginTransmission(activeTouchAddr);
    Wire.write(0x02);
    if (Wire.endTransmission() != 0) {
        return false;
    }

    if (Wire.requestFrom((uint8_t)activeTouchAddr, (uint8_t)1) != 1) {
        return false;
    }

    uint8_t touches = Wire.read() & 0x0F;
    if (touches == 0 || touches > 2) {
        return false;
    }

    // Pas 2: Citire bloc coordonate punct 1 din TOUCH1 (0x03..0x06: XH, XL, YH, YL)
    Wire.beginTransmission(activeTouchAddr);
    Wire.write(0x03);
    if (Wire.endTransmission() != 0) {
        return false;
    }

    if (Wire.requestFrom((uint8_t)activeTouchAddr, (uint8_t)4) != 4) {
        return false;
    }

    uint8_t xh = Wire.read();
    uint8_t xl = Wire.read();
    uint8_t yh = Wire.read();
    uint8_t yl = Wire.read();

    x = ((uint16_t)(xh & 0x0F) << 8) | xl;
    y = ((uint16_t)(yh & 0x0F) << 8) | yl;
    return true;
}

bool touchGetTap(uint16_t &x, uint16_t &y) {
    uint16_t curX = 0, curY = 0;
    bool isPressed = touchReadRaw(curX, curY);
    uint32_t now = millis();

    bool tapped = false;
    if (isPressed && !s_wasPressed && (now - s_lastTouchTime > 150)) {
        x = curX;
        y = curY;
        tapped = true;
        s_lastTouchTime = now;
        Serial.printf("[TOUCH TAP] Raw: X=%u, Y=%u\n", x, y);
    }

    s_wasPressed = isPressed;
    return tapped;
}

