#include "gns_info.h"
#include "setup.h"
#include "lcd_driver.h"
#include "gps_handler.h"
#include "wifi_mgr.h"

// Afisare Pagina Diagnostic Sateliti GNSS si Grafic SNR (cu Smart Refresh chirurgical)
void renderUiGpsDiag(bool fullRedraw) {
    const GpsData &g = gpsGetData();
    const GnssSatellitesData &sats = gpsGetSatellitesData();

    const int16_t chartX = 28;
    const int16_t chartY = 52;
    const int16_t chartW = 446;
    const int16_t chartH = 145;
    const int16_t chartBaseY = chartY + chartH; // 197
    const uint8_t maxDisplaySats = 18;
    const int16_t slotWidth = chartW / maxDisplaySats; // ~24px
    const int16_t barWidth = 16;

    // Variabile statice dirty-check pentru a nu redesena ce nu s-a schimbat
    static int8_t lastFixBadge = -1;
    static uint8_t lastInFix = 255, lastVis = 255;
    static float lastHdop = -99.0f;
    static uint8_t lastCfgMode = 255, lastTotGps = 255, lastTotBds = 255;
    static int lastUtcSec = -1;
    static char lastLoc[8] = "";
    static bool lastFixState = false;
    static double lastLat = -999.0, lastLon = -999.0;
    static float lastAlt = -999.0f;
    static int8_t lastAntOpen = -1;
    static bool lastNmeaEss = false;

    struct LastSatSlot {
        uint8_t id;
        uint8_t snr;
        int16_t elevation;
        bool inUse;
        char type;
        bool active;
        int16_t barH;
    };
    static LastSatSlot lastSlots[maxDisplaySats];

    // 0. Initializare elemente statice la intrarea in ecran (fullRedraw)
    if (fullRedraw) {
        lastFixBadge = -1;
        lastInFix = 255; lastVis = 255;
        lastHdop = -99.0f;
        lastCfgMode = 255; lastTotGps = 255; lastTotBds = 255;
        lastUtcSec = -1;
        lastLoc[0] = '\0';
        lastFixState = false;
        lastLat = -999.0; lastLon = -999.0; lastAlt = -999.0f;
        lastAntOpen = -1;
        lastNmeaEss = false;
        memset(lastSlots, 0, sizeof(lastSlots));

        // Header static
        lcdDrawString(8, 8, "GNSS DIAGNOSTIC", COLOR_CYAN, COLOR_BG, 1);

        // Separator sub header (Y = 38)
        lcdDrawLine(0, 38, SCREEN_WIDTH - 1, 38, COLOR_DARK_GRAY);

        // Linii de ghidare orizontale la 10, 20, 30, 40, 50 dB-Hz
        for (int db = 10; db <= 50; db += 10) {
            int16_t lineY = chartBaseY - (db * chartH) / 50;
            char dbLabel[8];
            snprintf(dbLabel, sizeof(dbLabel), "%2d", db);
            lcdDrawString(4, lineY - 4, dbLabel, COLOR_DARK_GRAY, COLOR_BG, 1);
            lcdDrawLine(chartX, lineY, chartX + chartW, lineY, COLOR_DARK_GRAY);
        }
        // Linia de baza (0 dB-Hz)
        lcdDrawLine(chartX, chartBaseY, chartX + chartW, chartBaseY, COLOR_WHITE);
        lcdDrawString(4, chartBaseY - 4, " 0", COLOR_DARK_GRAY, COLOR_BG, 1);

        // Separator sub grafic (Y = 236)
        lcdDrawLine(0, 236, SCREEN_WIDTH - 1, 236, COLOR_DARK_GRAY);
    }

    // 1. Badge FIX status
    int8_t curFixBadge = g.fix ? 2 : (g.present ? 1 : 0);
    if (curFixBadge != lastFixBadge) {
        lastFixBadge = curFixBadge;
        if (curFixBadge == 2) {
            lcdFillRect(105, 6, 48, 14, COLOR_GREEN);
            lcdDrawString(110, 9, "3D FIX", COLOR_BLACK, COLOR_GREEN, 1);
        } else if (curFixBadge == 1) {
            lcdFillRect(105, 6, 48, 14, COLOR_RED);
            lcdDrawString(110, 9, "NO FIX", COLOR_WHITE, COLOR_RED, 1);
        } else {
            lcdFillRect(105, 6, 54, 14, COLOR_DARK_GRAY);
            lcdDrawString(109, 9, "NO UART", COLOR_WHITE, COLOR_DARK_GRAY, 1);
        }
    }

    // 2. Sateliti & HDOP
    if (sats.satsInFix != lastInFix || sats.satsVisible != lastVis || fabsf(g.hdop - lastHdop) > 0.05f) {
        lastInFix = sats.satsInFix;
        lastVis = sats.satsVisible;
        lastHdop = g.hdop;
        lcdFillRect(165, 6, 310, 16, COLOR_BG);
        char satHeadBuf[48];
        snprintf(satHeadBuf, sizeof(satHeadBuf), "Sats: %u in fix / %u vizibili | HDOP: %.1f", sats.satsInFix, sats.satsVisible, g.hdop);
        lcdDrawString(165, 9, satHeadBuf, COLOR_WHITE, COLOR_BG, 1);
    }

    // 3. Sub-header Mod (GPS/BDS)
    uint8_t cfgMode = settingsGetGnssMode();
    if (cfgMode != lastCfgMode || sats.totalGps != lastTotGps || sats.totalBds != lastTotBds) {
        lastCfgMode = cfgMode;
        lastTotGps = sats.totalGps;
        lastTotBds = sats.totalBds;
        lcdFillRect(8, 24, 240, 12, COLOR_BG);
        const char* modeStr = (cfgMode == 1) ? "GPS Only" : (cfgMode == 2 ? "BDS Only" : "GPS+BDS");
        char modeBuf[48];
        snprintf(modeBuf, sizeof(modeBuf), "Mod: %s (G:%u B:%u)", modeStr, sats.totalGps, sats.totalBds);
        lcdDrawString(8, 24, modeBuf, COLOR_YELLOW, COLOR_BG, 1);
    }

    // 4. Ora UTC & Locator (Fara flicker prin scriere directa si afisare LOC doar cand exista 3D FIX)
    const char *locStr = g.fix ? g.maidenhead : "------";
    if (g.second != lastUtcSec || strcmp(lastLoc, locStr) != 0 || g.fix != lastFixState) {
        lastUtcSec = g.second;
        lastFixState = g.fix;
        strncpy(lastLoc, locStr, sizeof(lastLoc) - 1);
        lastLoc[sizeof(lastLoc) - 1] = '\0';
        char timeLocBuf[36];
        snprintf(timeLocBuf, sizeof(timeLocBuf), "UTC: %02d:%02d:%02d | LOC: %-6s", g.hour, g.minute, g.second, locStr);
        lcdDrawString(260, 24, timeLocBuf, COLOR_WHITE, COLOR_BG, 1);
    }

    // 5. Bare Sateliti (Desenare inteligenta chirurgicala doar pe coloanele modificate)
    uint8_t dispCount = (sats.count > maxDisplaySats) ? maxDisplaySats : sats.count;
    for (uint8_t i = 0; i < maxDisplaySats; i++) {
        int16_t xPos = chartX + 4 + (i * slotWidth);

        if (i < dispCount) {
            const SatInfo &s = sats.sats[i];
            bool changed = (!lastSlots[i].active ||
                            lastSlots[i].id != s.id ||
                            lastSlots[i].snr != s.snr ||
                            lastSlots[i].elevation != s.elevation ||
                            lastSlots[i].inUse != s.inUse ||
                            lastSlots[i].type != s.type);

            if (changed) {
                int16_t oldBarH = lastSlots[i].active ? lastSlots[i].barH : 0;
                int16_t oldBarY = chartBaseY - oldBarH;

                int16_t barH = (s.snr * chartH) / 50;
                if (barH > chartH) barH = chartH;
                if (barH < 0) barH = 0;
                int16_t barY = chartBaseY - barH;

                // Zona ocupata de vechiul text SNR (daca era deasupra barei)
                int16_t oldTextTop = (oldBarY > chartY + 12) ? (oldBarY - 12) : oldBarY;
                if (oldTextTop < chartY) oldTextTop = chartY;

                // Daca bara anterioara a fost mai inalta sau textul a fost mai sus, curatam DOAR felia expusa
                int16_t clearTop = oldTextTop;
                if (clearTop < barY) {
                    lcdFillRect(xPos - 2, clearTop, barWidth + 4, barY - clearTop, COLOR_BG);
                    // Redesenam DOAR liniile de caroiaj care au fost acoperite si sunt acum expuse (fara a atinge cele de deasupra!)
                    for (int db = 10; db <= 50; db += 10) {
                        int16_t lineY = chartBaseY - (db * chartH) / 50;
                        if (lineY >= clearTop && lineY < barY) {
                            lcdDrawLine(xPos - 2, lineY, xPos + barWidth + 2, lineY, COLOR_DARK_GRAY);
                        }
                    }
                }

                // Daca satelitul si-a pierdut starea inUse, curatam bordura cyan
                if (lastSlots[i].active && lastSlots[i].inUse && !s.inUse) {
                    lcdDrawLine(xPos - 1, oldBarY - 1, xPos + barWidth, oldBarY - 1, COLOR_BG);
                    lcdDrawLine(xPos - 1, oldBarY, xPos - 1, chartBaseY, COLOR_BG);
                    lcdDrawLine(xPos + barWidth, oldBarY, xPos + barWidth, chartBaseY, COLOR_BG);
                }

                uint16_t barColor = COLOR_DARK_GRAY;
                if (s.snr >= 30) barColor = COLOR_GREEN;
                else if (s.snr >= 20) barColor = COLOR_YELLOW;
                else if (s.snr > 0) barColor = COLOR_ORANGE;

                // Desenam bara de semnal
                if (barH > 0) {
                    lcdFillRect(xPos, barY, barWidth, barH, barColor);
                    if (s.inUse) {
                        lcdDrawRect(xPos - 1, barY - 1, barWidth + 2, barH + 1, COLOR_CYAN);
                    }
                }

                // Desenam eticheta numerica SNR
                char snrStr[6];
                snprintf(snrStr, sizeof(snrStr), "%u", s.snr);
                int16_t snrTextY = (barY > chartY + 12) ? (barY - 10) : (barY + 2);
                uint16_t snrTextColor = (barY > chartY + 12) ? COLOR_WHITE : COLOR_BLACK;
                lcdDrawString(xPos + 1, snrTextY, snrStr, snrTextColor, (barY > chartY + 12) ? COLOR_BG : barColor, 1);

                // Etichete PRN si elevatie sub grafic (actualizate doar la nevoie)
                if (!lastSlots[i].active || lastSlots[i].id != s.id || lastSlots[i].type != s.type || lastSlots[i].elevation != s.elevation) {
                    lcdFillRect(xPos - 2, chartBaseY + 1, barWidth + 4, 30, COLOR_BG);
                    lcdDrawLine(xPos - 2, chartBaseY, xPos + barWidth + 2, chartBaseY, COLOR_WHITE);

                    char prnStr[8];
                    snprintf(prnStr, sizeof(prnStr), "%c%02u", s.type, s.id);
                    uint16_t prnColor = (s.type == 'G') ? COLOR_CYAN : COLOR_YELLOW;
                    lcdDrawString(xPos - 2, chartBaseY + 4, prnStr, prnColor, COLOR_BG, 1);

                    char elevStr[8];
                    snprintf(elevStr, sizeof(elevStr), "%d*", s.elevation);
                    lcdDrawString(xPos - 2, chartBaseY + 16, elevStr, COLOR_GRAY, COLOR_BG, 1);
                }

                lastSlots[i].id = s.id;
                lastSlots[i].snr = s.snr;
                lastSlots[i].elevation = s.elevation;
                lastSlots[i].inUse = s.inUse;
                lastSlots[i].type = s.type;
                lastSlots[i].barH = barH;
                lastSlots[i].active = true;
            }
        } else if (lastSlots[i].active) {
            // Coloana devenita inactiva: stergem doar spatiul ocupat anterior si refacem doar caroiajul care a fost acoperit
            int16_t oldBarH = lastSlots[i].barH;
            int16_t oldBarY = chartBaseY - oldBarH;
            int16_t clearTop = (oldBarY > chartY + 12) ? (oldBarY - 12) : oldBarY;
            if (clearTop < chartY) clearTop = chartY;

            lcdFillRect(xPos - 2, clearTop, barWidth + 4, (chartBaseY - clearTop) + 31, COLOR_BG);
            for (int db = 10; db <= 50; db += 10) {
                int16_t lineY = chartBaseY - (db * chartH) / 50;
                if (lineY >= clearTop && lineY < chartBaseY) {
                    lcdDrawLine(xPos - 2, lineY, xPos + barWidth + 2, lineY, COLOR_DARK_GRAY);
                }
            }
            lcdDrawLine(xPos - 2, chartBaseY, xPos + barWidth + 2, chartBaseY, COLOR_WHITE);

            lastSlots[i].active = false;
            lastSlots[i].barH = 0;
        }
    }

    // 6. Footer Coordonate
    if (fabs(g.lat - lastLat) > 0.00001 || fabs(g.lon - lastLon) > 0.00001 || fabsf(g.altitude - lastAlt) > 0.1f) {
        lastLat = g.lat; lastLon = g.lon; lastAlt = g.altitude;
        lcdFillRect(8, 244, 465, 12, COLOR_BG);
        char coordBuf[64];
        snprintf(coordBuf, sizeof(coordBuf), "LAT: %.5f*  LON: %.5f*  ALT: %.1fm", g.lat, g.lon, g.altitude);
        lcdDrawString(8, 244, coordBuf, COLOR_WHITE, COLOR_BG, 1);
    }

    // 7. Footer Antena
    bool curNmeaEss = settingsGetNmeaEssential();
    if (sats.antennaOpen != lastAntOpen || curNmeaEss != lastNmeaEss) {
        lastAntOpen = sats.antennaOpen;
        lastNmeaEss = curNmeaEss;
        lcdFillRect(8, 258, 465, 12, COLOR_BG);
        char antBuf[80];
        if (sats.antennaOpen) {
            snprintf(antBuf, sizeof(antBuf), "ANTENA: PASIVA / OPEN | NMEA: %s | Comutare ecran: din Web UI",
                     curNmeaEss ? "Esentiale" : "Toate");
            lcdDrawString(8, 258, antBuf, COLOR_YELLOW, COLOR_BG, 1);
        } else {
            snprintf(antBuf, sizeof(antBuf), "ANTENA: ACTIVA (OK) | NMEA: %s | Comutare ecran: din Web UI",
                     curNmeaEss ? "Esentiale" : "Toate");
            lcdDrawString(8, 258, antBuf, COLOR_GREEN, COLOR_BG, 1);
        }
    }
}

