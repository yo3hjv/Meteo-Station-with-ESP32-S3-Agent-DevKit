#include "gps_handler.h"
#include "setup.h"
#include "wifi_mgr.h"
#include <HardwareSerial.h>
#include <TinyGPSPlus.h>

static HardwareSerial gpsSerial(1);
static TinyGPSPlus gps;
static GpsData currentGpsData = {false, false, 0.0f, 0.0f, "-----", 0, 99.0f, 0.0f, 0, 0, 0, 0, 0, 0, false};
static GnssSatellitesData currentSatData = {0, {}, 0, 0, 0, 0, false};

static unsigned long lastUartByteTime = 0;
static unsigned long lastBlinkToggle = 0;
static bool blinkCurrent = false;

// Buffer pentru asamblarea liniilor NMEA brute
static char nmeaLineBuf[160];
static uint8_t nmeaLinePos = 0;

// Tokenizer simplu si robust pentru campuri NMEA ce pastreaza campurile vide (,,)
static bool getNextNmeaField(const char* &p, char* outField, size_t maxLen) {
    if (!p || *p == '\0' || *p == '*' || *p == '\r' || *p == '\n') return false;
    size_t idx = 0;
    while (*p != '\0' && *p != ',' && *p != '*' && *p != '\r' && *p != '\n') {
        if (idx + 1 < maxLen) {
            outField[idx++] = *p;
        }
        p++;
    }
    outField[idx] = '\0';
    if (*p == ',') p++; // consuma separatorul
    return true;
}

// Trimitere comanda CASIC proprietara AT6558 cu calcul checksum automat
void gpsSendCasicCmd(const char *cmdBody) {
    if (!cmdBody) return;
    uint8_t cs = 0;
    for (size_t i = 0; cmdBody[i] != '\0'; i++) {
        cs ^= (uint8_t)cmdBody[i];
    }
    char buf[80];
    snprintf(buf, sizeof(buf), "$%s*%02X\r\n", cmdBody, cs);
    gpsSerial.print(buf);
#if (DEBUG_GPS || DEBUG_PONSTEST)
    Serial.printf("[CASIC SENT] %s", buf);
#endif
}

// 1 = GPS only, 2 = BDS only, 3 = GPS + BDS (ALL)
void gpsSetConstellationMode(uint8_t mode) {
    if (mode < 1 || mode > 3) mode = 3;
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "PCAS04,%u", mode);
    gpsSendCasicCmd(cmd);
}

// true = doar GGA, GSA, GSV, RMC; false = toate (inclusiv GLL, VTG, ZDA, TXT)
void gpsSetNmeaFilter(bool essentialOnly) {
    if (essentialOnly) {
        gpsSendCasicCmd("PCAS03,1,0,1,1,1,0,0,0,0,0,0,0,0,0");
    } else {
        gpsSendCasicCmd("PCAS03,1,1,1,1,1,1,1,1,0,1,0,0,1,0");
    }
}

// Salveaza configuratia curenta in memoria Flash a cipului AT6558
void gpsSaveToFlash() {
    gpsSendCasicCmd("PCAS00");
}

static void updateSatInList(char type, uint8_t id, uint8_t elev, uint16_t azim, uint8_t snr) {
    unsigned long now = millis();
    // Cauta daca exista deja
    for (uint8_t i = 0; i < currentSatData.count; i++) {
        if (currentSatData.sats[i].type == type && currentSatData.sats[i].id == id) {
            currentSatData.sats[i].elevation = elev;
            currentSatData.sats[i].azimuth = azim;
            currentSatData.sats[i].snr = snr;
            currentSatData.sats[i].lastSeen = now;
            return;
        }
    }

    // Daca nu exista si mai este loc
    if (currentSatData.count < MAX_SATELLITES_TRACKED) {
        uint8_t idx = currentSatData.count++;
        currentSatData.sats[idx].type = type;
        currentSatData.sats[idx].id = id;
        currentSatData.sats[idx].elevation = elev;
        currentSatData.sats[idx].azimuth = azim;
        currentSatData.sats[idx].snr = snr;
        currentSatData.sats[idx].inUse = false;
        currentSatData.sats[idx].lastSeen = now;
        return;
    }

    // Daca lista e plina, inlocuieste satelitul cel mai vechi
    uint8_t oldestIdx = 0;
    unsigned long oldestTime = currentSatData.sats[0].lastSeen;
    for (uint8_t i = 1; i < MAX_SATELLITES_TRACKED; i++) {
        if (currentSatData.sats[i].lastSeen < oldestTime) {
            oldestTime = currentSatData.sats[i].lastSeen;
            oldestIdx = i;
        }
    }
    currentSatData.sats[oldestIdx].type = type;
    currentSatData.sats[oldestIdx].id = id;
    currentSatData.sats[oldestIdx].elevation = elev;
    currentSatData.sats[oldestIdx].azimuth = azim;
    currentSatData.sats[oldestIdx].snr = snr;
    currentSatData.sats[oldestIdx].inUse = false;
    currentSatData.sats[oldestIdx].lastSeen = now;
}

// Curata satelitii care nu au mai fost vazuti de 5 secunde si actualizeaza contorii
static void pruneStaleSatellites() {
    unsigned long now = millis();
    uint8_t writeIdx = 0;
    uint8_t gpsCount = 0;
    uint8_t bdsCount = 0;
    uint8_t visibleCount = 0;
    uint8_t inFixCount = 0;

    for (uint8_t i = 0; i < currentSatData.count; i++) {
        if (now - currentSatData.sats[i].lastSeen <= 5000) {
            if (writeIdx != i) {
                currentSatData.sats[writeIdx] = currentSatData.sats[i];
            }
            if (currentSatData.sats[writeIdx].snr > 0) visibleCount++;
            if (currentSatData.sats[writeIdx].type == 'G') gpsCount++;
            else if (currentSatData.sats[writeIdx].type == 'B') bdsCount++;
            if (currentSatData.sats[writeIdx].inUse) inFixCount++;
            writeIdx++;
        }
    }
    currentSatData.count = writeIdx;
    currentSatData.satsVisible = visibleCount;
    currentSatData.totalGps = gpsCount;
    currentSatData.totalBds = bdsCount;
    currentSatData.satsInFix = inFixCount;
}

static void parseGsvSentence(const char *line, char type) {
    const char *p = strchr(line, ',');
    if (!p) return;
    p++; // Dupa primul camp ($GPGSV,)

    char totalMsgs[8], msgNum[8], totalSats[8];
    if (!getNextNmeaField(p, totalMsgs, sizeof(totalMsgs))) return;
    if (!getNextNmeaField(p, msgNum, sizeof(msgNum))) return;
    if (!getNextNmeaField(p, totalSats, sizeof(totalSats))) return;

    // Parsam pana la 4 sateliti din fraza GSV
    for (int i = 0; i < 4; i++) {
        char prnStr[8], elevStr[8], azimStr[8], snrStr[8];
        if (!getNextNmeaField(p, prnStr, sizeof(prnStr))) break;
        if (!getNextNmeaField(p, elevStr, sizeof(elevStr))) break;
        if (!getNextNmeaField(p, azimStr, sizeof(azimStr))) break;
        if (!getNextNmeaField(p, snrStr, sizeof(snrStr))) break;

        if (prnStr[0] != '\0') {
            uint8_t id = (uint8_t)atoi(prnStr);
            uint8_t elev = (uint8_t)atoi(elevStr);
            uint16_t azim = (uint16_t)atoi(azimStr);
            uint8_t snr = (snrStr[0] != '\0') ? (uint8_t)atoi(snrStr) : 0;
            if (id > 0) {
                updateSatInList(type, id, elev, azim, snr);
            }
        }
    }
}

static void parseGsaSentence(const char *line) {
    const char *p = strchr(line, ',');
    if (!p) return;
    p++; // dupa identificator

    char mode[4], fixType[4];
    if (!getNextNmeaField(p, mode, sizeof(mode))) return;
    if (!getNextNmeaField(p, fixType, sizeof(fixType))) return;

    char targetType = 0;
    if (strncmp(line, "$GPGSA", 6) == 0) targetType = 'G';
    else if (strncmp(line, "$BDGSA", 6) == 0 || strncmp(line, "$GBGSA", 6) == 0) targetType = 'B';

    // Resetare inUse pentru satelitii constelatiei curente inainte de a citi noile PRN-uri active
    for (uint8_t s = 0; s < currentSatData.count; s++) {
        if (!targetType || currentSatData.sats[s].type == targetType) {
            currentSatData.sats[s].inUse = false;
        }
    }

    // Daca nu avem fix valid (fixType == 1 inseamna no fix), nu setam niciun satelit ca inUse
    if (fixType[0] == '1') return;

    // Campurile 3..14 contin PRN-urile utilizate in fix
    for (int i = 0; i < 12; i++) {
        char prnStr[8];
        if (!getNextNmeaField(p, prnStr, sizeof(prnStr))) break;
        if (prnStr[0] != '\0') {
            uint8_t id = (uint8_t)atoi(prnStr);
            if (id > 0) {
                for (uint8_t s = 0; s < currentSatData.count; s++) {
                    if ((!targetType || currentSatData.sats[s].type == targetType) && currentSatData.sats[s].id == id) {
                        currentSatData.sats[s].inUse = true;
                    }
                }
            }
        }
    }
}

static void processCompleteNmeaLine(const char *line) {
    if (!line || line[0] == '\0') return;

    // Afisare pe Serial Debug daca optiunea este activa
    if (settingsGetNmeaSerialDebug()) {
        Serial.println(line);
    }

    // Verificare antena supervizata
    if (strstr(line, "$GPTXT") != nullptr) {
        if (strstr(line, "ANTENNA OPEN") != nullptr) {
            currentSatData.antennaOpen = true;
        } else if (strstr(line, "ANTENNA OK") != nullptr) {
            currentSatData.antennaOpen = false;
        }
    }
    // Parsare GSV (sateliti in camp vizual)
    else if (strncmp(line, "$GPGSV", 6) == 0) {
        parseGsvSentence(line, 'G');
    } else if (strncmp(line, "$BDGSV", 6) == 0 || strncmp(line, "$GBGSV", 6) == 0) {
        parseGsvSentence(line, 'B');
    }
    // Parsare GSA (sateliti in fix activ)
    else if (strncmp(line, "$GNGSA", 6) == 0 || strncmp(line, "$GPGSA", 6) == 0 || strncmp(line, "$BDGSA", 6) == 0) {
        parseGsaSentence(line);
    }
}

const GnssSatellitesData& gpsGetSatellitesData() {
    pruneStaleSatellites();
    return currentSatData;
}


void calcMaidenhead(float lat, float lon, char *outLoc6) {
    if (!outLoc6) return;
    if (lat < -90.0f || lat > 90.0f || lon < -180.0f || lon > 180.0f) {
        strcpy(outLoc6, "-----");
        return;
    }

    double adjLon = lon + 180.0;
    double adjLat = lat + 90.0;

    char f1 = 'A' + (int)(adjLon / 20.0);
    char f2 = 'A' + (int)(adjLat / 10.0);

    double remLon = adjLon - ((int)(adjLon / 20.0) * 20.0);
    double remLat = adjLat - ((int)(adjLat / 10.0) * 10.0);

    char s1 = '0' + (int)(remLon / 2.0);
    char s2 = '0' + (int)(remLat / 1.0);

    double subLon = remLon - ((int)(remLon / 2.0) * 2.0);
    double subLat = remLat - ((int)(remLat / 1.0) * 1.0);

    char sub1 = 'a' + (int)(subLon / (2.0 / 24.0));
    char sub2 = 'a' + (int)(subLat / (1.0 / 24.0));

    outLoc6[0] = f1;
    outLoc6[1] = f2;
    outLoc6[2] = s1;
    outLoc6[3] = s2;
    outLoc6[4] = sub1;
    outLoc6[5] = sub2;
    outLoc6[6] = '\0';
}

void gpsHandlerInit() {
    // Incarca locatorul manual salvat anterior in memorie (daca exista)
    String savedLoc = settingsGetManualLocator();
    if (savedLoc.length() >= 4) {
        strncpy(currentGpsData.maidenhead, savedLoc.c_str(), 6);
        currentGpsData.maidenhead[6] = '\0';
#if DEBUG_PONSTEST
        Serial.printf("[GPS] Incarcat locator salvat anterior: %s\n", currentGpsData.maidenhead);
#endif
    }

    // Array of standard baud rates to test
    const uint32_t bauds[] = { 9600, 38400, 115200, 4800, 19200 };
    const uint8_t numBauds = sizeof(bauds) / sizeof(bauds[0]);

#if DEBUG_PONSTEST
    Serial.printf("[GPS] Initializing GPS UART on RX=IO%d. Scanning baudrates...\n", GPS_RX_PIN);
#endif

    uint32_t chosenBaud = GPS_BAUDRATE;
    bool foundValidNmea = false;

    for (uint8_t i = 0; i < numBauds; i++) {
        uint32_t testBaud = bauds[i];
        gpsSerial.end();
        delay(10);
        gpsSerial.begin(testBaud, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
        delay(50);

        unsigned long startMs = millis();
        uint32_t charsAtBaud = 0;
        uint32_t validCharsBefore = gps.charsProcessed();

        while (millis() - startMs < 900) {
            while (gpsSerial.available()) {
                char c = gpsSerial.read();
                gps.encode(c);
                charsAtBaud++;
                if (c == '$') {
                    foundValidNmea = true;
                }
            }
            delay(10);
        }

        if (charsAtBaud > 0) {
#if DEBUG_PONSTEST
            Serial.printf("  -> At %u baud: received %u chars (NMEA marker '$': %s)\n",
                          testBaud, charsAtBaud, foundValidNmea ? "YES" : "no");
#endif
            if (foundValidNmea) {
                chosenBaud = testBaud;
                break;
            }
        }
    }

    gpsSerial.end();
    delay(10);
    gpsSerial.begin(chosenBaud, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
#if DEBUG_PONSTEST
    Serial.printf("[GPS] GPS UART configured at %u baud.\n", chosenBaud);
#endif

    // Boot sequence: check for incoming GPS data for 5 seconds
    unsigned long startMs = millis();
    bool detected = false;
    uint32_t bytesReceived = 0;
    while (millis() - startMs < GPS_BOOT_TIMEOUT_MS) {
        while (gpsSerial.available()) {
            char c = (char)gpsSerial.read();
            gps.encode(c);
            detected = true;
            bytesReceived++;
            lastUartByteTime = millis();

            if (c == '\n' || c == '\r') {
                if (nmeaLinePos > 0) {
                    nmeaLineBuf[nmeaLinePos] = '\0';
                    processCompleteNmeaLine(nmeaLineBuf);
                    nmeaLinePos = 0;
                }
            } else {
                if (nmeaLinePos < sizeof(nmeaLineBuf) - 1) {
                    nmeaLineBuf[nmeaLinePos++] = c;
                }
            }
        }
        delay(10);
    }

    currentGpsData.present = detected;
    if (detected) {
#if DEBUG_PONSTEST
        Serial.printf("\n[GPS] GPS hardware detected! Received %u bytes during boot at %u baud.\n", bytesReceived, chosenBaud);
#endif
        // Aplica configuratia salvata pentru GP-02 / AT6558
        delay(80);
        gpsSetConstellationMode(settingsGetGnssMode());
        delay(80);
        gpsSetNmeaFilter(settingsGetNmeaEssential());
    } else {
#if DEBUG_PONSTEST
        Serial.printf("\n[GPS] No GPS data received on RX (IO%d) within 5 seconds. Continuing without GPS...\n", GPS_RX_PIN);
#endif
    }
}

void gpsHandlerUpdate() {
    bool hadBytes = false;
    while (gpsSerial.available()) {
        char c = (char)gpsSerial.read();
        gps.encode(c);
        hadBytes = true;
        lastUartByteTime = millis();

        // Asamblare linie NMEA pentru parsare GSV/GSA/TXT si stream serial
        if (c == '\n' || c == '\r') {
            if (nmeaLinePos > 0) {
                nmeaLineBuf[nmeaLinePos] = '\0';
                processCompleteNmeaLine(nmeaLineBuf);
                nmeaLinePos = 0;
            }
        } else {
            if (nmeaLinePos < sizeof(nmeaLineBuf) - 1) {
                nmeaLineBuf[nmeaLinePos++] = c;
            }
        }
    }

    unsigned long nowMs = millis();

    // Verificare dinamica a prezentei fizice a fluxului de date GPS (hot-plug detection)
    if (hadBytes) {
        currentGpsData.present = true;
    } else if (nowMs - lastUartByteTime > 3000) {
        // Peste 3 secunde fara niciun octet -> firul a fost scos sau GPS-ul e oprit
        currentGpsData.present = false;
        currentGpsData.fix = false;
        currentGpsData.satellites = 0;
        strcpy(currentGpsData.maidenhead, "-----");
    }

#if DEBUG_GPS
    static unsigned long lastGpsStats = 0;
    if (nowMs - lastGpsStats >= 5000) {
        lastGpsStats = nowMs;
        Serial.printf("\n[GPS STATS] Present: %s | Fix: %s | Chars: %u | Valid Chk: %u | Age: %u ms | Sat: %u\n",
                      currentGpsData.present ? "YES" : "NO",
                      currentGpsData.fix ? "YES" : "NO",
                      gps.charsProcessed(), gps.passedChecksum(),
                      gps.location.isValid() ? gps.location.age() : 99999,
                      gps.satellites.isValid() ? gps.satellites.value() : 0);
    }
#endif

    // Rata de clipire derivata din primirea datelor pe UART
    if (currentGpsData.present) {
        if (nowMs - lastBlinkToggle >= 300) {
            lastBlinkToggle = nowMs;
            blinkCurrent = !blinkCurrent;
        }
        currentGpsData.blinkState = blinkCurrent;
    } else {
        currentGpsData.blinkState = false;
    }

    // 1. Extragere timp si data din sateliti (posibila chiar si fara fix de pozitie 2D/3D!)
    if (currentGpsData.present && gps.date.isValid() && gps.time.isValid() &&
        gps.date.age() < 3000 && gps.time.age() < 3000 && gps.date.year() >= 2023) {
        currentGpsData.year = gps.date.year();
        currentGpsData.month = gps.date.month();
        currentGpsData.day = gps.date.day();
        currentGpsData.hour = gps.time.hour();
        currentGpsData.minute = gps.time.minute();
        currentGpsData.second = gps.time.second();
    }

    // 2. Extragere sateliti si HDOP
    if (gps.satellites.isValid()) {
        currentGpsData.satellites = gps.satellites.value();
    }
    if (gps.hdop.isValid()) {
        currentGpsData.hdop = gps.hdop.hdop();
    }

    // 3. Fix-ul de pozitie geografica (lat, lon, altitudine, locator)
    bool hasFreshFix = currentGpsData.present &&
                       gps.location.isValid() &&
                       (gps.location.age() < 3000);

    if (hasFreshFix) {
        currentGpsData.lat = gps.location.lat();
        currentGpsData.lon = gps.location.lng();
        calcMaidenhead(currentGpsData.lat, currentGpsData.lon, currentGpsData.maidenhead);
        currentGpsData.fix = true;

        // GPS actualizeaza si rescrie automat locatorul activ
        settingsSetManualLocator(String(currentGpsData.maidenhead));

        if (gps.altitude.isValid()) {
            currentGpsData.altitude = gps.altitude.meters();
        }
    } else {
        currentGpsData.fix = false;
        // Daca nu avem fix GPS, folosim locatorul manual salvat (daca exista)
        String savedLoc = settingsGetManualLocator();
        if (savedLoc.length() >= 4) {
            strncpy(currentGpsData.maidenhead, savedLoc.c_str(), 6);
            currentGpsData.maidenhead[6] = '\0';
        } else if (!currentGpsData.present) {
            strcpy(currentGpsData.maidenhead, "-----");
        }
    }
}

void gpsSetManualLocator(const char *loc) {
    if (!loc) return;
    String cleanLoc = String(loc);
    cleanLoc.trim();
    if (cleanLoc.length() >= 4) {
        strncpy(currentGpsData.maidenhead, cleanLoc.c_str(), 6);
        currentGpsData.maidenhead[6] = '\0';
        settingsSetManualLocator(String(currentGpsData.maidenhead));
        Serial.printf("[GPS] Locator manual setat: %s\n", currentGpsData.maidenhead);
    }
}

bool gpsIsPresent() {
    return currentGpsData.present;
}

bool gpsHasFix() {
    return currentGpsData.fix;
}

const GpsData& gpsGetData() {
    return currentGpsData;
}

