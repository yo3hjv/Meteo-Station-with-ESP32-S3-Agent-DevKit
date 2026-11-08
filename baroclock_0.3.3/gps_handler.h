#pragma once
#include <Arduino.h>

struct GpsData {
    bool present;
    bool fix;
    float lat;
    float lon;
    char maidenhead[7]; // 6 chars + null terminator (e.g. "KN34bk")
    uint8_t satellites;
    float hdop;
    float altitude; // Altitudine in metri
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    bool blinkState; // Derived from UART reception
};

#define MAX_SATELLITES_TRACKED 24

struct SatInfo {
    char type;             // 'G' = GPS, 'B' = BeiDou
    uint8_t id;            // PRN / Satellite ID
    uint8_t elevation;     // 0..90 deg
    uint16_t azimuth;      // 0..359 deg
    uint8_t snr;           // 0..99 dB-Hz (C/N0)
    bool inUse;            // true if used in active fix solution (from GSA)
    unsigned long lastSeen;// millis() timestamp
};

struct GnssSatellitesData {
    uint8_t count;
    SatInfo sats[MAX_SATELLITES_TRACKED];
    uint8_t satsVisible;   // Sateliti receptionati activ cu semnal (SNR > 0)
    uint8_t satsInFix;
    uint8_t totalGps;
    uint8_t totalBds;
    bool antennaOpen;      // true if $GPTXT ANTENNA OPEN detected
};

void gpsHandlerInit();
void gpsHandlerUpdate();
bool gpsIsPresent();
bool gpsHasFix();
const GpsData& gpsGetData();
const GnssSatellitesData& gpsGetSatellitesData();

// Comenzi CASIC pentru modulul Ai-Thinker GP-02 / AT6558
void gpsSendCasicCmd(const char *cmdBody);
void gpsSetConstellationMode(uint8_t mode);
void gpsSetNmeaFilter(bool essentialOnly);
void gpsSaveToFlash();


// Convert latitude and longitude to 6-character Maidenhead grid
void calcMaidenhead(float lat, float lon, char *outLoc6);

// Setare manuala a locatorului (cand nu exista GPS sau suprascris la nevoie)
void gpsSetManualLocator(const char *loc);

