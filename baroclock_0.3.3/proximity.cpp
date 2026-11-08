#include "proximity.h"
#include "sound.h"
#include "wifi_mgr.h"
#include <Wire.h>

static uint16_t lastRawValue = 0;
static bool sensorReady = false;
static bool isDetected = false;
static unsigned long lastPollTime = 0;
static unsigned long lastTriggerTime = 0;

static uint8_t readReg(uint8_t reg) {
    Wire.beginTransmission(PROXIMITY_I2C_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        return 0;
    }
    if (Wire.requestFrom((uint8_t)PROXIMITY_I2C_ADDR, (uint8_t)1) == 1) {
        return Wire.read();
    }
    return 0;
}

static void writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(PROXIMITY_I2C_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

void proximityInit() {
#if !ENABLE_PROXIMITY_SENSOR
#if DEBUG_PONSTEST
    Serial.println("[PROXIMITY] Senzorul de proximitate este DEZACTIVAT din setup.h.");
#endif
    // Daca senzorul a fost activ anterior, il adormim complet (Sleep total, 0 uA)
    Wire.beginTransmission(PROXIMITY_I2C_ADDR);
    if (Wire.endTransmission() == 0) {
        writeReg(0x00, 0x00); // ENABLE = 0 (PON=0, PEN=0, oscilator oprit)
    }
    sensorReady = false;
    return;
#endif

#if (DEBUG_PROXIMITY || DEBUG_PONSTEST)
    Serial.println("[PROXIMITY] Initializare senzor MK-TP2016 (I2C 0x39)...");
#endif

    Wire.beginTransmission(PROXIMITY_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("[PROXIMITY] AVERTISMENT: Senzorul MK-TP2016 nu raspunde la adresa 0x39!");
        sensorReady = false;
        return;
    }

    uint8_t chipId = readReg(0x12);
    Serial.printf("[PROXIMITY] Detectat chip ID la reg 0x12: 0x%02X (SO6103)\n", chipId);

    // Activare senzor (ENABLE reg 0x00 = PON | PEN | WEN) conform datasheet MK-PB2016/SO6103
    writeReg(0x00, 0x0D);
    writeReg(0x0E, 0x08); // PPULSE = 8 impulsuri IR per ciclu
    writeReg(0x0F, 0x08); // CONTROL = 125mA drive current, 4x Gain pentru reflexie optima

#if DEBUG_PROXIMITY
    Serial.println("[PROXIMITY] === DUMP REGISTRI MK-TP2016 (0x39) LA BOOT ===");
    for (uint8_t r = 0x00; r <= 0x1F; r++) {
        uint8_t val = readReg(r);
        Serial.printf("  Reg 0x%02X: 0x%02X (%3u)\n", r, val, val);
    }
    Serial.println("[PROXIMITY] ==============================================");
#endif

    sensorReady = true;
    Serial.println("[PROXIMITY] Senzor MK-TP2016 configurat si activat cu succes.");
}

void proximityUpdate() {
#if !ENABLE_PROXIMITY_SENSOR
    return;
#endif
    if (!sensorReady) return;

    unsigned long now = millis();
    if (now - lastPollTime < PROXIMITY_READ_INTERVAL_MS) {
        return;
    }
    lastPollTime = now;

    // Citire directa registre 0x18 (Low) si 0x19 (High) fara masca conform mkpb2016.c
    uint8_t low = readReg(0x18);
    uint8_t high = readReg(0x19);
    lastRawValue = ((uint16_t)high << 8) | low;

#if DEBUG_PROXIMITY
    static unsigned long lastDbgPrint = 0;
    if (now - lastDbgPrint >= 500) {
        lastDbgPrint = now;
        uint8_t st = readReg(0x13);
        Serial.printf("[PROXIMITY_DBG] Raw: %u (L: 0x%02X, H: 0x%02X), STATUS 0x13: 0x%02X (PVALID=%d)\n",
                      lastRawValue, low, high, st, (st & 0x02) ? 1 : 0);
    }
#endif

    // Verificare prag detectie apropiere
    if (lastRawValue >= PROXIMITY_THRESHOLD) {
        isDetected = true;
        // Daca testul de sunet este activat in pagina Web / NVS
        if (settingsGetSoundTestEnabled()) {
            if (now - lastTriggerTime >= PROXIMITY_COOLDOWN_MS) {
                lastTriggerTime = now;
                Serial.printf("[PROXIMITY] Obiect sesizat! Valoare: %u >= Prag %u -> Declanșare Bip %u Hz (%u ms)\n",
                              lastRawValue, PROXIMITY_THRESHOLD, BEEP_FREQUENCY_HZ, BEEP_DURATION_MS);
                soundPlayBeep(BEEP_FREQUENCY_HZ, BEEP_DURATION_MS);
            }
        }
    } else {
        isDetected = false;
    }
}

uint16_t proximityGetRawValue() {
    return lastRawValue;
}

bool proximityIsDetected() {
    return isDetected;
}

