#include "ext_sensors.h"
#include "temp_calib.h"
#include "setup.h"
#include <WiFiUdp.h>
#include <Preferences.h>

static WiFiUDP udpListener;
static bool udpStarted = false;
static Preferences prefs;

static ExtSensorSlot slots[MAX_EXT_SENSOR_SLOTS];
static ExtSensorDiscovered discovered[MAX_DISCOVERED_SENSORS];
static uint8_t discoveredCount = 0;

// Parser JSON ultra-rapid fara alocari dinamice pe heap
static bool extractJsonString(const char* json, const char* key, char* outVal, size_t maxLen) {
    char searchKey[32];
    snprintf(searchKey, sizeof(searchKey), "\"%s\"", key);
    const char* p = strstr(json, searchKey);
    if (!p) return false;
    p += strlen(searchKey);
    while (*p && (*p == ' ' || *p == ':' || *p == '\t')) p++;
    if (*p != '\"') return false;
    p++;
    size_t i = 0;
    while (*p && *p != '\"' && i < maxLen - 1) {
        outVal[i++] = *p++;
    }
    outVal[i] = '\0';
    return true;
}

static bool extractJsonFloat(const char* json, const char* key, float &outVal) {
    char searchKey[32];
    snprintf(searchKey, sizeof(searchKey), "\"%s\"", key);
    const char* p = strstr(json, searchKey);
    if (!p) return false;
    p += strlen(searchKey);
    while (*p && (*p == ' ' || *p == ':' || *p == '\t')) p++;
    char *endPtr = nullptr;
    float val = strtof(p, &endPtr);
    if (endPtr == p) return false;
    outVal = val;
    return true;
}

static void updateDiscoveredCache(const char* id, const char* ip, float tempC, bool hasTemp) {
    int foundIdx = -1;
    for (int i = 0; i < discoveredCount; i++) {
        if (strncmp(discovered[i].id, id, sizeof(discovered[i].id)) == 0) {
            foundIdx = i;
            break;
        }
    }

    if (foundIdx >= 0) {
        strncpy(discovered[foundIdx].ip, ip, sizeof(discovered[foundIdx].ip) - 1);
        if (hasTemp) {
            discovered[foundIdx].lastTempC = tempC;
            discovered[foundIdx].hasData = true;
        }
        discovered[foundIdx].lastSeenMillis = millis();
        discovered[foundIdx].isOnline = true;
    } else if (discoveredCount < MAX_DISCOVERED_SENSORS) {
        strncpy(discovered[discoveredCount].id, id, sizeof(discovered[discoveredCount].id) - 1);
        strncpy(discovered[discoveredCount].ip, ip, sizeof(discovered[discoveredCount].ip) - 1);
        discovered[discoveredCount].lastTempC = hasTemp ? tempC : 0.0f;
        discovered[discoveredCount].hasData = hasTemp;
        discovered[discoveredCount].lastSeenMillis = millis();
        discovered[discoveredCount].isOnline = true;
        discoveredCount++;
    }
}

void extSensorsInit() {
    prefs.begin("ext_sensors", false);

    // Initializare sloturi din NVS
    for (int i = 0; i < MAX_EXT_SENSOR_SLOTS; i++) {
        char kId[16], kAlias[16], kIp[16], kOff[16];
        snprintf(kId, sizeof(kId), "s%d_id", i + 1);
        snprintf(kAlias, sizeof(kAlias), "s%d_alias", i + 1);
        snprintf(kIp, sizeof(kIp), "s%d_ip", i + 1);
        snprintf(kOff, sizeof(kOff), "s%d_off", i + 1);

        String idVal = prefs.getString(kId, "");
        String aliasVal = prefs.getString(kAlias, "");
        String ipVal = prefs.getString(kIp, "");
        float offVal = prefs.getFloat(kOff, 0.0f);

        memset(&slots[i], 0, sizeof(ExtSensorSlot));
        if (idVal.length() > 0) {
            strncpy(slots[i].id, idVal.c_str(), sizeof(slots[i].id) - 1);
            strncpy(slots[i].alias, aliasVal.c_str(), sizeof(slots[i].alias) - 1);
            strncpy(slots[i].lastIp, ipVal.c_str(), sizeof(slots[i].lastIp) - 1);
            slots[i].tempOffset = offVal;
            slots[i].isAssigned = true;
            slots[i].isOnline = false; // Va deveni online la primul pachet
        }
    }

    if (!udpStarted) {
        udpListener.begin(EXT_SENSORS_UDP_PORT);
        udpStarted = true;
#if DEBUG_PONSTEST
        Serial.printf("[EXT SENSORS] Socket UDP de receptie pornit pe portul %d.\n", EXT_SENSORS_UDP_PORT);
#endif
    }
}

void extSensorsUpdate() {
    if (!udpStarted) return;

    // 1. Parsare pachete UDP sosite
    int packetSize = udpListener.parsePacket();
    while (packetSize > 0) {
        char packetBuf[384];
        int len = udpListener.read(packetBuf, sizeof(packetBuf) - 1);
        if (len > 0) {
            packetBuf[len] = '\0';

            char protocol[16] = "";
            char type[16] = "";
            char id[17] = "";
            char ip[16] = "";

            extractJsonString(packetBuf, "protocol", protocol, sizeof(protocol));
            extractJsonString(packetBuf, "type", type, sizeof(type));
            extractJsonString(packetBuf, "id", id, sizeof(id));
            extractJsonString(packetBuf, "ip", ip, sizeof(ip));

            // Daca IP-ul nu este in JSON, il luam de la expeditorul UDP
            if (strlen(ip) == 0) {
                strncpy(ip, udpListener.remoteIP().toString().c_str(), sizeof(ip) - 1);
            }

            if (strcmp(protocol, "METEO_V1") == 0 && strlen(id) > 0) {
                float tempC = 0.0f;
                bool hasTemp = false;

                if (strcmp(type, "data") == 0) {
                    if (extractJsonFloat(packetBuf, "temperature_c", tempC)) {
                        hasTemp = true;
                    }
                }

                // Actualizare cache descoperire
                updateDiscoveredCache(id, ip, tempC, hasTemp);

                // Verificare daca senzorul este asociat unui slot activ
                for (int s = 0; s < MAX_EXT_SENSOR_SLOTS; s++) {
                    if (slots[s].isAssigned && strcasecmp(slots[s].id, id) == 0) {
                        slots[s].lastSeenMillis = millis();
                        slots[s].isOnline = true;

                        // Actualizare IP daca a variat
                        if (strncmp(slots[s].lastIp, ip, sizeof(slots[s].lastIp)) != 0) {
                            strncpy(slots[s].lastIp, ip, sizeof(slots[s].lastIp) - 1);
                            char kIp[16];
                            snprintf(kIp, sizeof(kIp), "s%d_ip", s + 1);
                            prefs.putString(kIp, slots[s].lastIp);
                        }

                        if (hasTemp) {
                            slots[s].rawTempC = tempC;
                            if (s == 0) {
                                slots[s].lastTempC = tempCalibrateExt1(tempC);
                            } else {
                                slots[s].lastTempC = tempC + slots[s].tempOffset;
                            }
                            slots[s].hasData = true;
#if DEBUG_EXT_SENSORS
                            Serial.printf("[EXT SENSORS] Receptie Slot %d [%s / %s]: Brut=%.2f C, Calibrat=%.2f C (Offset=%+.2f C, IP: %s)\n",
                                          s + 1, slots[s].alias, slots[s].id, tempC, slots[s].lastTempC, slots[s].tempOffset, ip);
#endif
                        }
                    }
                }
            }
        }
        packetSize = udpListener.parsePacket();
    }

    // 2. Verificare periodica timeout OFFLINE (10 minute)
    unsigned long now = millis();
    for (int s = 0; s < MAX_EXT_SENSOR_SLOTS; s++) {
        if (slots[s].isAssigned) {
            if (slots[s].hasData && (now - slots[s].lastSeenMillis > (EXT_SENSORS_OFFLINE_SEC * 1000UL))) {
                if (slots[s].isOnline) {
                    slots[s].isOnline = false;
#if DEBUG_EXT_SENSORS
                    Serial.printf("[EXT SENSORS] AVERTISMENT: Senzorul Slot %d [%s] a intrat in stare OFFLINE!\n",
                                  s + 1, slots[s].alias);
#endif
                }
            }
        }
    }
}

bool extSensorGetSlot(int slotNum, ExtSensorSlot &outSlot) {
    if (slotNum < 1 || slotNum > MAX_EXT_SENSOR_SLOTS) return false;
    int idx = slotNum - 1;
    if (idx == 0 && slots[idx].hasData) {
        slots[idx].lastTempC = tempCalibrateExt1(slots[idx].rawTempC);
    }
    outSlot = slots[idx];
    return true;
}

void extSensorSetSlot(int slotNum, const char* id, const char* alias) {
    if (slotNum < 1 || slotNum > MAX_EXT_SENSOR_SLOTS) return;
    int idx = slotNum - 1;

    memset(slots[idx].id, 0, sizeof(slots[idx].id));
    memset(slots[idx].alias, 0, sizeof(slots[idx].alias));
    strncpy(slots[idx].id, id, sizeof(slots[idx].id) - 1);
    strncpy(slots[idx].alias, alias, sizeof(slots[idx].alias) - 1);

    slots[idx].isAssigned = (strlen(slots[idx].id) > 0);
    slots[idx].isOnline = false;

    // Incarcare offset existent pentru acest slot
    char kOff[16];
    snprintf(kOff, sizeof(kOff), "s%d_off", slotNum);
    slots[idx].tempOffset = prefs.getFloat(kOff, 0.0f);

    // Cautare IP in cache descoperit daca exista
    for (int i = 0; i < discoveredCount; i++) {
        if (strcasecmp(discovered[i].id, id) == 0) {
            strncpy(slots[idx].lastIp, discovered[i].ip, sizeof(slots[idx].lastIp) - 1);
            if (discovered[i].hasData) {
                slots[idx].rawTempC = discovered[i].lastTempC;
                if (idx == 0) {
                    slots[idx].lastTempC = tempCalibrateExt1(discovered[i].lastTempC);
                } else {
                    slots[idx].lastTempC = discovered[i].lastTempC + slots[idx].tempOffset;
                }
                slots[idx].hasData = true;
                slots[idx].lastSeenMillis = discovered[i].lastSeenMillis;
                slots[idx].isOnline = discovered[i].isOnline;
            }
            break;
        }
    }

    // Salvare in NVS
    char kId[16], kAlias[16], kIp[16];
    snprintf(kId, sizeof(kId), "s%d_id", slotNum);
    snprintf(kAlias, sizeof(kAlias), "s%d_alias", slotNum);
    snprintf(kIp, sizeof(kIp), "s%d_ip", slotNum);

    prefs.putString(kId, slots[idx].id);
    prefs.putString(kAlias, slots[idx].alias);
    prefs.putString(kIp, slots[idx].lastIp);

#if DEBUG_EXT_SENSORS
    Serial.printf("[EXT SENSORS] Configurat Slot %d: ID='%s', Alias='%s', IP='%s'\n",
                  slotNum, slots[idx].id, slots[idx].alias, slots[idx].lastIp);
#endif
}

void extSensorClearSlot(int slotNum) {
    if (slotNum < 1 || slotNum > MAX_EXT_SENSOR_SLOTS) return;
    int idx = slotNum - 1;

    memset(&slots[idx], 0, sizeof(ExtSensorSlot));

    char kId[16], kAlias[16], kIp[16], kOff[16];
    snprintf(kId, sizeof(kId), "s%d_id", slotNum);
    snprintf(kAlias, sizeof(kAlias), "s%d_alias", slotNum);
    snprintf(kIp, sizeof(kIp), "s%d_ip", slotNum);
    snprintf(kOff, sizeof(kOff), "s%d_off", slotNum);

    prefs.remove(kId);
    prefs.remove(kAlias);
    prefs.remove(kIp);
    prefs.remove(kOff);
}

void extSensorCalibrate(int slotNum, float referenceTempC) {
    if (slotNum < 1 || slotNum > MAX_EXT_SENSOR_SLOTS) return;
    int idx = slotNum - 1;
    if (!slots[idx].isAssigned || !slots[idx].hasData) return;

    float newOffset = referenceTempC - slots[idx].rawTempC;
    slots[idx].tempOffset = newOffset;

    if (idx == 0) {
        // Slot 1 foloseste tabela multi-punct din temp_calib: sincronizam uniform toate nodurile
        float curOffsets[TEMP_CALIB_BANDS_COUNT];
        tempCalibGetExt1Offsets(curOffsets);
        float delta = referenceTempC - slots[idx].lastTempC;
        for (int i = 0; i < TEMP_CALIB_BANDS_COUNT; i++) {
            curOffsets[i] += delta;
        }
        tempCalibSetExt1Offsets(curOffsets);
        slots[idx].lastTempC = referenceTempC;
    } else {
        slots[idx].lastTempC = slots[idx].rawTempC + newOffset;
    }

    char kOff[16];
    snprintf(kOff, sizeof(kOff), "s%d_off", slotNum);
    prefs.putFloat(kOff, newOffset);

#if DEBUG_EXT_SENSORS
    Serial.printf("[EXT SENSORS] Calibrare Slot %d [%s]: Ref=%.2f C, Brut=%.2f C, Offset=%+.2f C\n",
                  slotNum, slots[idx].alias, referenceTempC, slots[idx].rawTempC, newOffset);
#endif
}

uint8_t extSensorGetDiscoveredCount() {
    return discoveredCount;
}

bool extSensorGetDiscovered(int index, ExtSensorDiscovered &outDev) {
    if (index < 0 || index >= discoveredCount) return false;
    outDev = discovered[index];
    return true;
}

