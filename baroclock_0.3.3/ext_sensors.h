#pragma once
#include <Arduino.h>
#include "setup.h"

// ============================================================================
// MODUL RECEPTIE SENZORI EXTERNI WI-FI (UDP BROADCAST PROTOCOL METEO_V1)
// ============================================================================

struct ExtSensorSlot {
    char id[17];                  // ID unic derivat din MAC (ex: "A1B2C3D4E5F6")
    char alias[21];               // Nume arbitrar definit de utilizator (ex: "TERASA")
    char lastIp[16];              // Ultimul IP cunoscut al senzorului
    float rawTempC;               // Temperatura bruta nemodificata primita din retea (C)
    float tempOffset;             // Offset de calibrare memorat in NVS (C)
    float lastTempC;              // Temperatura finala calibrata (rawTempC + tempOffset) (C)
    unsigned long lastSeenMillis; // Timestamp millis() la ultima receptie valida
    bool isAssigned;              // true daca slotul are un senzor ID asociat
    bool hasData;                 // true daca s-a primit cel putin o citire
    bool isOnline;                // true daca senzorul a transmis in ultimele 10 min
};

struct ExtSensorDiscovered {
    char id[17];
    char ip[16];
    float lastTempC;
    bool hasData;
    unsigned long lastSeenMillis;
    bool isOnline;
};

void extSensorsInit();
void extSensorsUpdate();

// Interogare si configurare sloturi active (slotNum: 1 .. 3)
bool extSensorGetSlot(int slotNum, ExtSensorSlot &outSlot);
void extSensorSetSlot(int slotNum, const char* id, const char* alias);
void extSensorClearSlot(int slotNum);
void extSensorCalibrate(int slotNum, float referenceTempC);

// Interogare cache senzori descoperiti automat pe retea
uint8_t extSensorGetDiscoveredCount();
bool extSensorGetDiscovered(int index, ExtSensorDiscovered &outDev);

