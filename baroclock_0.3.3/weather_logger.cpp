#include "weather_logger.h"
#include "setup.h"
#include "bme_sensor.h"
#include "gps_handler.h"
#include "wifi_mgr.h"
#include <LittleFS.h>
#include <time.h>

static const char* WEATHER_14D_PATH = "/baro_14d.dat";
static const char* WEATHER_1Y_PATH  = "/baro_1y.dat";
static const char* WEATHER_LEGACY_PATH = "/weather.dat";

// Header binar pentru Stratul 2 (/baro_14d.dat)
struct __attribute__((packed)) Weather14dFileHeader {
    uint32_t magic;      // 0x57313444 ("W14D")
    uint16_t version;    // 1
    uint16_t count;      // Numarul de inregistrari valide (max 6720)
    uint16_t head;       // Indexul urmatorului slot liber
    uint16_t reserved;
};

// Stratul 2: Istoric 14 zile la 3 min in PSRAM (6720 * 20B = 134.4 KB)
static WeatherRecord *weather14dRecords = nullptr;
static uint16_t weather14dCount = 0;
static uint16_t weather14dHead = 0;

static uint16_t ramBufferPending = 0; // Contor pana la salvarea pe Flash (la 20 esantioane = 1 ora)
static uint32_t archiveCount = 0;     // Numar inregistrari orare in Stratul 3 (/baro_1y.dat)
static uint32_t lastArchivedHour = 0; // Epoch al ultimei ore arhivate in Stratul 3

static unsigned long lastSampleTime = 0;
static WeatherForecastInfo currentForecast;

// Cache UI pentru a preveni flicker-ul la desenare
static WeatherRiskLevel lastDrawnRisk = (WeatherRiskLevel)99;
static WeatherTrendDir  lastDrawnTrend = (WeatherTrendDir)99;
static WeatherIconType  lastDrawnIcon = (WeatherIconType)99;
static String           lastDrawnStatus = "";

// Verifica daca timpul este sincronizat cert via NTP sau GPS
bool isTimeSynchronized() {
    time_t nowSec;
    time(&nowSec);
    if (nowSec > 1672531199) { // NTP valid dupa 1 Ianuarie 2023
        return true;
    }
    const GpsData &g = gpsGetData();
    if (g.present && g.fix && g.year >= 2023) {
        return true;
    }
    return false;
}

// Obtine timestamp-ul UTC curent DOAR daca este sincronizat (NTP sau GPS).
// Daca nu este sincronizat, returneaza 0 (fara fallback fictiv).
uint32_t getCurrentTimestamp() {
    time_t nowSec;
    time(&nowSec);
    if (nowSec > 1672531199) { // NTP valid dupa 1 Ianuarie 2023
        return (uint32_t)nowSec;
    }

    const GpsData &g = gpsGetData();
    if (g.present && g.fix && g.year >= 2023) {
        struct tm t;
        memset(&t, 0, sizeof(t));
        t.tm_year = g.year - 1900;
        t.tm_mon  = g.month - 1;
        t.tm_mday = g.day;
        t.tm_hour = g.hour;
        t.tm_min  = g.minute;
        t.tm_sec  = g.second;
        return (uint32_t)mktime(&t);
    }

    return 0; // Nicio sincronizare -> nu emitem timestamp
}

// Determina codul curent de locatie (Maidenhead 4..6-char GPS/manual sau "INDOOR")
void getCurrentLocation(char *outLoc) {
    if (!outLoc) return;
    const GpsData &g = gpsGetData();
    if (strlen(g.maidenhead) >= 4 && g.maidenhead[0] != '-' && g.maidenhead[0] != '\0') {
        strncpy(outLoc, g.maidenhead, 6);
        outLoc[6] = '\0';
        return;
    }
    String savedLoc = settingsGetManualLocator();
    savedLoc.trim();
    if (savedLoc.length() >= 4 && savedLoc[0] != '-') {
        strncpy(outLoc, savedLoc.c_str(), 6);
        outLoc[6] = '\0';
        return;
    }
    strcpy(outLoc, "INDOOR");
}

// Salveaza o inregistrare in istoricul circular din Stratul 2 (PSRAM)
static void addToWeather14d(const WeatherRecord &rec) {
    if (!weather14dRecords) return;
    weather14dRecords[weather14dHead] = rec;
    weather14dHead = (weather14dHead + 1) % WEATHER_14D_MAX_RECORDS;
    if (weather14dCount < WEATHER_14D_MAX_RECORDS) {
        weather14dCount++;
    }
}

// Scrie Stratul 2 din PSRAM in LittleFS (/baro_14d.dat)
static void flushWeather14dToFlash() {
    if (!weather14dRecords || weather14dCount == 0) return;

    File f = LittleFS.open(WEATHER_14D_PATH, FILE_WRITE);
    if (!f) {
        Serial.println("[WEATHER] EROARE: Nu s-a putut deschide /baro_14d.dat pentru scriere!");
        return;
    }

    Weather14dFileHeader hdr;
    hdr.magic = 0x57313444; // "W14D"
    hdr.version = 1;
    hdr.count = weather14dCount;
    hdr.head = weather14dHead;
    hdr.reserved = 0;
    f.write((const uint8_t*)&hdr, sizeof(hdr));

    uint8_t *ptr = (uint8_t*)weather14dRecords;
    size_t totalBytes = WEATHER_14D_MAX_RECORDS * sizeof(WeatherRecord);
    size_t written = 0;
    while (written < totalBytes) {
        size_t chunk = (totalBytes - written > 4096) ? 4096 : (totalBytes - written);
        f.write(ptr + written, chunk);
        written += chunk;
        yield();
    }
    f.close();
    ramBufferPending = 0;
#if DEBUG_WEATHER
    Serial.printf("[WEATHER] Stratul 2 salvat in LittleFS: %u inregistrari (%u KB)\n",
                  weather14dCount, (uint32_t)(totalBytes / 1024));
#endif
}

// Verifica si arhiveaza o inregistrare orara in Stratul 3 (/baro_1y.dat)
static void checkAndRecordHourlyArchive(uint32_t nowTs) {
    if (!weather14dRecords || weather14dCount == 0 || nowTs < 1700000000UL) return;

    uint32_t hourEpoch = (nowTs / 3600UL) * 3600UL;
    if (hourEpoch <= lastArchivedHour) return;

    // Calculam min, max, avg din esantioanele din ultima ora (pana la 20 esantioane)
    uint32_t minP = 0xFFFFFFFF;
    uint32_t maxP = 0;
    uint64_t sumP = 0;
    int32_t  sumT = 0;
    uint16_t n = 0;

    int newestIdx = (weather14dHead + WEATHER_14D_MAX_RECORDS - 1) % WEATHER_14D_MAX_RECORDS;
    uint16_t samplesToCheck = (weather14dCount < 20) ? weather14dCount : 20;

    for (uint16_t i = 0; i < samplesToCheck; i++) {
        int idx = (newestIdx + WEATHER_14D_MAX_RECORDS - i) % WEATHER_14D_MAX_RECORDS;
        const WeatherRecord &r = weather14dRecords[idx];
        if (r.timestamp >= hourEpoch - 3600UL && r.timestamp <= hourEpoch + 60UL) {
            if (r.pressure_pa < minP) minP = r.pressure_pa;
            if (r.pressure_pa > maxP) maxP = r.pressure_pa;
            sumP += r.pressure_pa;
            sumT += r.temp_c_x100;
            n++;
        }
    }

    if (n == 0) {
        // Daca nu gasim esantioane in fereastra exacta, folosim cel mai recent
        const WeatherRecord &r = weather14dRecords[newestIdx];
        minP = maxP = sumP = r.pressure_pa;
        sumT = r.temp_c_x100;
        n = 1;
    }

    const WeatherRecord &rLatest = weather14dRecords[newestIdx];
    WeatherArchiveRecord arc;
    arc.timestamp = hourEpoch;
    // Conversie Pa in hPa x 10 (ex: 101325 Pa -> 10133 adica 1013.3 hPa)
    arc.pres_avg_pa_10 = (uint16_t)(((sumP / n) + 5) / 10);
    arc.pres_min_pa_10 = (uint16_t)((minP + 5) / 10);
    arc.pres_max_pa_10 = (uint16_t)((maxP + 5) / 10);
    arc.temp_avg_c_100 = (int16_t)(sumT / n);
    arc.altitude_m = rLatest.altitude_m;
    memcpy(arc.maidenhead, rLatest.maidenhead, 6);

    File fa = LittleFS.open(WEATHER_1Y_PATH, FILE_APPEND);
    if (fa) {
        fa.write((const uint8_t*)&arc, sizeof(WeatherArchiveRecord));
        fa.close();
        archiveCount++;
        lastArchivedHour = hourEpoch;
#if DEBUG_WEATHER
        Serial.printf("[WEATHER] Stratul 3: Arhivat esantion orar in %s (Total: %u ore, P_avg: %.1f hPa)\n",
                      WEATHER_1Y_PATH, archiveCount, (float)arc.pres_avg_pa_10 / 10.0f);
#endif
    }
}

// Calculeaza prognoza si nivelul de risc pe baza istoricului corelat cu locatia
static void computeForecast() {
    char curLoc[7];
    getCurrentLocation(curLoc);
    strncpy(currentForecast.locationTag, curLoc, 6);
    currentForecast.locationTag[6] = '\0';

    if (weather14dCount == 0 || !weather14dRecords) {
        currentForecast.risk = RISK_NORMAL;
        currentForecast.trend = TREND_STEADY;
        currentForecast.icon = WEATHER_ICON_SUN;
        currentForecast.deltaP_30m = 0.0f;
        currentForecast.deltaP_1h = 0.0f;
        currentForecast.deltaP_3h = 0.0f;
        currentForecast.rateOfChange = 0.0f;
        currentForecast.acceleration = 0.0f;
        currentForecast.localSamplesCount = 0;
        strcpy(currentForecast.statusText, WEATHER_TXT_INIT);
        return;
    }

    // Cel mai recent esantion
    int newestIdx = (weather14dHead + WEATHER_14D_MAX_RECORDS - 1) % WEATHER_14D_MAX_RECORDS;
    const WeatherRecord &recNew = weather14dRecords[newestIdx];
    float pNow = (float)recNew.pressure_pa / 100.0f;
    uint32_t tNow = recNew.timestamp;

    // Cautare inapoi prin bufferul circular doar a inregistrarilor din ACEEASI LOCATIE
    uint16_t matchCount = 0;
    const WeatherRecord *rec30m = nullptr;
    const WeatherRecord *rec1h  = nullptr;
    const WeatherRecord *rec3h  = nullptr;

    uint32_t bestDiff30m = 0xFFFFFFFF;
    uint32_t bestDiff1h  = 0xFFFFFFFF;
    uint32_t bestDiff3h  = 0xFFFFFFFF;

    // Cautam in istoricul operational (ultimele 480 esantioane = 24 ore)
    uint16_t searchLimit = (weather14dCount < 480) ? weather14dCount : 480;
    for (uint16_t i = 0; i < searchLimit; i++) {
        int idx = (newestIdx + WEATHER_14D_MAX_RECORDS - i) % WEATHER_14D_MAX_RECORDS;
        const WeatherRecord &r = weather14dRecords[idx];

        // Filtrare stricta: doar datele inregistrate in locatia curenta
        if (strncmp(r.maidenhead, curLoc, 6) != 0) {
            continue;
        }

        matchCount++;

        if (r.timestamp > tNow) continue; // anomalie timestamp
        uint32_t dtSec = tNow - r.timestamp;

        // Cautare esantion cel mai apropiat de 30 min (1800 sec, fereastra 10..45 min)
        if (dtSec >= 600 && dtSec <= 2700) {
            uint32_t diff = abs((long)dtSec - 1800);
            if (diff < bestDiff30m) {
                bestDiff30m = diff;
                rec30m = &r;
            }
        }

        // Cautare esantion cel mai apropiat de 1 ora (3600 sec, fereastra 45..90 min)
        if (dtSec >= 2700 && dtSec <= 5400) {
            uint32_t diff = abs((long)dtSec - 3600);
            if (diff < bestDiff1h) {
                bestDiff1h = diff;
                rec1h = &r;
            }
        }

        // Cautare esantion cel mai apropiat de 3 ore (10800 sec, fereastra 2h..4h)
        if (dtSec >= 7200 && dtSec <= 14400) {
            uint32_t diff = abs((long)dtSec - 10800);
            if (diff < bestDiff3h) {
                bestDiff3h = diff;
                rec3h = &r;
            }
        }
    }

    currentForecast.localSamplesCount = matchCount;

    const GpsData &g = gpsGetData();
    // Daca suntem intr-o locatie mobila cu fix GPS proaspat si avem mai putin de 4 esantioane (sub 12 minute)
    if (g.fix && matchCount < 4) {
        currentForecast.risk = RISK_NEW_LOCATION;
        currentForecast.trend = TREND_STEADY;
        currentForecast.icon = WEATHER_ICON_NEW_LOC;
        currentForecast.deltaP_30m = 0.0f;
        currentForecast.deltaP_1h = 0.0f;
        currentForecast.deltaP_3h = 0.0f;
        currentForecast.rateOfChange = 0.0f;
        currentForecast.acceleration = 0.0f;
        strcpy(currentForecast.statusText, WEATHER_TXT_NEW_LOC);
        return;
    }

    // Pentru statie fixa (fara fix GPS), daca avem sub 2 esantioane afisam initializare barica fara blocare
    if (!g.fix && matchCount < 2) {
        currentForecast.risk = RISK_NORMAL;
        currentForecast.trend = TREND_STEADY;
        currentForecast.icon = WEATHER_ICON_SUN;
        currentForecast.deltaP_30m = 0.0f;
        currentForecast.deltaP_1h = 0.0f;
        currentForecast.deltaP_3h = 0.0f;
        currentForecast.rateOfChange = 0.0f;
        currentForecast.acceleration = 0.0f;
        strcpy(currentForecast.statusText, WEATHER_TXT_ACQUIRING);
        return;
    }

    // Calcul deltaP
    float dP30m = rec30m ? (pNow - ((float)rec30m->pressure_pa / 100.0f)) : 0.0f;
    float dP1h  = rec1h  ? (pNow - ((float)rec1h->pressure_pa / 100.0f))  : (dP30m * 2.0f);
    float dP3h  = rec3h  ? (pNow - ((float)rec3h->pressure_pa / 100.0f))  : (dP1h * 3.0f);

    currentForecast.deltaP_30m = dP30m;
    currentForecast.deltaP_1h  = dP1h;
    currentForecast.deltaP_3h  = dP3h;

    // Viteza de variatie (hPa / ora)
    currentForecast.rateOfChange = rec3h ? (dP3h / 3.0f) : (rec1h ? dP1h : (dP30m * 2.0f));

    // Acceleratia variatiei (derivata a doua: diferenta de viteza)
    currentForecast.acceleration = (dP30m * 2.0f) - dP1h;

    // Tendinta barica (directie)
    if (currentForecast.rateOfChange >= 0.25f || dP3h >= 0.5f) {
        currentForecast.trend = TREND_RISING;
    } else if (currentForecast.rateOfChange <= -0.25f || dP3h <= -0.5f) {
        currentForecast.trend = TREND_FALLING;
    } else {
        currentForecast.trend = TREND_STEADY;
    }

    // Citim starea de referinta setata de utilizator in pagina web:
    // 0 = WEATHER_BASE_CLEAR (Senin / Frumos)
    // 1 = WEATHER_BASE_CLOUDY (Cer innorat / Inchis)
    // 2 = WEATHER_BASE_RAIN (Ploaie activa / Vreme ploioasa)
    uint8_t baseRef = settingsGetWeatherBaseRef();

    // Clasificare nivel de risc si sinteza iconita / text
    if (dP3h < -3.5f || dP1h < -2.0f || (dP30m < -1.2f && currentForecast.acceleration < -0.8f)) {
        currentForecast.risk = RISK_STORM;
        currentForecast.icon = WEATHER_ICON_STORM;
        strcpy(currentForecast.statusText, WEATHER_TXT_STORM);
    } else if (dP3h < -2.0f || dP1h < -1.2f) {
        if (baseRef == WEATHER_BASE_RAIN) {
            currentForecast.risk = RISK_STORM;
            currentForecast.icon = WEATHER_ICON_STORM;
            strcpy(currentForecast.statusText, WEATHER_TXT_STORM);
        } else {
            currentForecast.risk = RISK_DETERIORATION;
            currentForecast.icon = WEATHER_ICON_RAIN;
            strcpy(currentForecast.statusText, WEATHER_TXT_DETERIORATE);
        }
    } else if (dP3h < -1.0f || dP1h < -0.6f) {
        if (baseRef == WEATHER_BASE_RAIN) {
            currentForecast.risk = RISK_DETERIORATION;
            currentForecast.icon = WEATHER_ICON_RAIN;
            strcpy(currentForecast.statusText, WEATHER_TXT_RAIN_CONT);
        } else if (baseRef == WEATHER_BASE_CLOUDY) {
            currentForecast.risk = RISK_DETERIORATION;
            currentForecast.icon = WEATHER_ICON_RAIN;
            strcpy(currentForecast.statusText, WEATHER_TXT_RAIN_POSS);
        } else {
            currentForecast.risk = RISK_CAUTION;
            currentForecast.icon = WEATHER_ICON_CLOUD;
            strcpy(currentForecast.statusText, WEATHER_TXT_CAUTION);
        }
    } else {
        // Vreme stabila sau crestere barica
        currentForecast.risk = RISK_NORMAL;

        if (baseRef == WEATHER_BASE_RAIN) {
            if (currentForecast.trend == TREND_RISING) {
                currentForecast.icon = WEATHER_ICON_CLOUD;
                strcpy(currentForecast.statusText, WEATHER_TXT_IMPROVING);
            } else {
                currentForecast.icon = WEATHER_ICON_RAIN;
                strcpy(currentForecast.statusText, WEATHER_TXT_STEADY_RAIN);
            }
        } else if (baseRef == WEATHER_BASE_CLOUDY) {
            if (currentForecast.trend == TREND_RISING) {
                currentForecast.icon = WEATHER_ICON_SUN_CLOUD;
                strcpy(currentForecast.statusText, WEATHER_TXT_IMPROVING);
            } else {
                currentForecast.icon = WEATHER_ICON_CLOUD;
                strcpy(currentForecast.statusText, WEATHER_TXT_STEADY_CLOUDY);
            }
        } else {
            // WEATHER_BASE_CLEAR (Senin / Frumos)
            if (currentForecast.trend == TREND_RISING) {
                currentForecast.icon = WEATHER_ICON_SUN;
                strcpy(currentForecast.statusText, WEATHER_TXT_STEADY_CLEAR);
            } else {
                currentForecast.icon = WEATHER_ICON_SUN_CLOUD;
                strcpy(currentForecast.statusText, WEATHER_TXT_NORMAL);
            }
        }
    }
}

void weatherLoggerInit() {
#if DEBUG_PONSTEST
    Serial.println("[WEATHER] Initializing LittleFS weather logger (3-Layer Architecture)...");
#endif
    if (!LittleFS.begin(true)) {
        Serial.println("[WEATHER] EROARE: LittleFS mount a esuat!");
        return;
    }

    // Alocare buffer Stratul 2 in PSRAM (6720 * 20B = 134.4 KB)
    if (psramFound()) {
        size_t allocSize = sizeof(WeatherRecord) * WEATHER_14D_MAX_RECORDS;
        weather14dRecords = (WeatherRecord*)ps_malloc(allocSize);
        if (weather14dRecords) {
            memset(weather14dRecords, 0, allocSize);
#if DEBUG_PONSTEST
            Serial.printf("[WEATHER] Stratul 2 alocat in PSRAM: %u esantioane (%u KB, 14 zile la 3 min)\n",
                          WEATHER_14D_MAX_RECORDS, (uint32_t)(allocSize / 1024));
#endif
        } else {
            Serial.println("[WEATHER] EROARE: Alocarea PSRAM pentru Stratul 2 a esuat!");
        }
    } else {
        Serial.println("[WEATHER] ATENTIE: PSRAM indisponibil! Incercare alocare in SRAM...");
        weather14dRecords = (WeatherRecord*)malloc(sizeof(WeatherRecord) * WEATHER_14D_MAX_RECORDS);
    }

    if (!weather14dRecords) {
        Serial.println("[WEATHER] CRITIC: Nu s-a putut aloca memoria pentru Stratul 2!");
        return;
    }

    // 1. Verificare si migrare din fisierul vechi /weather.dat daca exista
    if (LittleFS.exists(WEATHER_LEGACY_PATH) && !LittleFS.exists(WEATHER_14D_PATH)) {
#if DEBUG_PONSTEST
        Serial.println("[WEATHER] Detectat fisier vechi /weather.dat. Migrare la Stratul 2 (/baro_14d.dat)...");
#endif
        File fOld = LittleFS.open(WEATHER_LEGACY_PATH, FILE_READ);
        if (fOld) {
            size_t oldSize = fOld.size();
            size_t oldRecCount = oldSize / sizeof(WeatherRecord);
            size_t toLoad = (oldRecCount > WEATHER_14D_MAX_RECORDS) ? WEATHER_14D_MAX_RECORDS : oldRecCount;
            if (toLoad > 0) {
                size_t startPos = oldSize - (toLoad * sizeof(WeatherRecord));
                fOld.seek(startPos);
                for (size_t i = 0; i < toLoad; i++) {
                    WeatherRecord rec;
                    if (fOld.read((uint8_t*)&rec, sizeof(WeatherRecord)) == sizeof(WeatherRecord)) {
                        addToWeather14d(rec);
                    }
                }
            }
            fOld.close();
            LittleFS.remove(WEATHER_LEGACY_PATH);
            flushWeather14dToFlash();
#if DEBUG_PONSTEST
            Serial.printf("[WEATHER] Migrare finalizata: %u inregistrari transferate in /baro_14d.dat, /weather.dat sters.\n", toLoad);
#endif
        }
    } else if (LittleFS.exists(WEATHER_14D_PATH)) {
        File f = LittleFS.open(WEATHER_14D_PATH, FILE_READ);
        if (f) {
            Weather14dFileHeader hdr;
            if (f.read((uint8_t*)&hdr, sizeof(hdr)) == sizeof(hdr) && hdr.magic == 0x57313444) {
                weather14dCount = (hdr.count > WEATHER_14D_MAX_RECORDS) ? WEATHER_14D_MAX_RECORDS : hdr.count;
                weather14dHead = hdr.head % WEATHER_14D_MAX_RECORDS;
                f.read((uint8_t*)weather14dRecords, WEATHER_14D_MAX_RECORDS * sizeof(WeatherRecord));

                // Curatare inregistrari parazite (< 1700000000UL) si re-ordonare cronologica stricta
                if (weather14dCount > 0) {
                    WeatherRecord* cleanBuf = (WeatherRecord*)malloc(weather14dCount * sizeof(WeatherRecord));
                    if (cleanBuf) {
                        uint16_t oldestIdx = (weather14dHead >= weather14dCount) ? 
                                             (weather14dHead - weather14dCount) : 
                                             (WEATHER_14D_MAX_RECORDS + weather14dHead - weather14dCount);
                        uint16_t validCount = 0;
                        for (uint16_t i = 0; i < weather14dCount; i++) {
                            uint16_t phys = (oldestIdx + i) % WEATHER_14D_MAX_RECORDS;
                            if (weather14dRecords[phys].timestamp >= 1700000000UL) {
                                cleanBuf[validCount++] = weather14dRecords[phys];
                            }
                        }
                        if (validCount > 0) {
                            qsort(cleanBuf, validCount, sizeof(WeatherRecord), [](const void* a, const void* b) -> int {
                                uint32_t tA = ((const WeatherRecord*)a)->timestamp;
                                uint32_t tB = ((const WeatherRecord*)b)->timestamp;
                                if (tA < tB) return -1;
                                if (tA > tB) return 1;
                                return 0;
                            });
                            memcpy(weather14dRecords, cleanBuf, validCount * sizeof(WeatherRecord));
                            weather14dCount = validCount;
                            weather14dHead = validCount % WEATHER_14D_MAX_RECORDS;
                        }
                        free(cleanBuf);
                    }
                }

#if DEBUG_PONSTEST
                Serial.printf("[WEATHER] Incarcat Stratul 2 din LittleFS: %u inregistrari valide (%u KB, head=%u)\n",
                              weather14dCount, (uint32_t)(weather14dCount * sizeof(WeatherRecord) / 1024), weather14dHead);
#endif
            }
            f.close();
            if (LittleFS.exists(WEATHER_LEGACY_PATH)) {
                LittleFS.remove(WEATHER_LEGACY_PATH);
            }
        }
    } else {
#if DEBUG_PONSTEST
        Serial.println("[WEATHER] Fisierul /baro_14d.dat nu exista inca. Se va crea la primul flush orar.");
#endif
    }

    // 2. Verificare Stratul 3 (/baro_1y.dat)
    if (LittleFS.exists(WEATHER_1Y_PATH)) {
        File fa = LittleFS.open(WEATHER_1Y_PATH, FILE_READ);
        if (fa) {
            size_t arcBytes = fa.size();
            archiveCount = arcBytes / sizeof(WeatherArchiveRecord);
            if (archiveCount > 0) {
                fa.seek(arcBytes - sizeof(WeatherArchiveRecord));
                WeatherArchiveRecord lastArc;
                if (fa.read((uint8_t*)&lastArc, sizeof(WeatherArchiveRecord)) == sizeof(WeatherArchiveRecord)) {
                    lastArchivedHour = lastArc.timestamp;
                }
            }
#if DEBUG_PONSTEST
            Serial.printf("[WEATHER] Stratul 3 (Arhiva 1 an) detectat: %u inregistrari orare (%u KB, ultima ora: %u)\n",
                          archiveCount, (uint32_t)(arcBytes / 1024), lastArchivedHour);
#endif
            fa.close();
        }
    }

    // Efectuam o prima citire a senzorului DOAR daca timpul este deja sincronizat
    if (isTimeSynchronized()) {
        weatherLoggerForceSample();
    }

    // Actualizam barograful dupa incarcarea istoricului
    baroGraphResetBaseline();
}

static int8_t pendingGroundTruthState = -1;

const char* weatherGetStateName(uint8_t state) {
    switch (state) {
        case 0: return "Clear / Sunny";
        case 1: return "Partly Cloudy";
        case 2: return "Overcast / Cloudy";
        case 3: return "Fog / Mist";
        case 4: return "Drizzle / Light Showers";
        case 5: return "Steady Rain";
        case 6: return "Thunderstorm / Downpour";
        case 7: return "Snow / Sleet";
        default: return "Unknown";
    }
}

void weatherLoggerCalibrate(uint8_t state) {
    if (state > 7) state = 0;

    uint8_t baseRef = WEATHER_BASE_CLEAR;
    if (state == 0 || state == 1) baseRef = WEATHER_BASE_CLEAR;
    else if (state == 2 || state == 3) baseRef = WEATHER_BASE_CLOUDY;
    else baseRef = WEATHER_BASE_RAIN;
    settingsSetWeatherBaseRef(baseRef);

    pendingGroundTruthState = (int8_t)state;
    weatherLoggerForceSample();
    pendingGroundTruthState = -1;

#if DEBUG_WEATHER
    Serial.printf("[WEATHER] Calibrare vreme reala Ground Truth: Stare=%u (%s), BaseRef=%u\n",
                  state, weatherGetStateName(state), baseRef);
#endif
}

void weatherLoggerForceSample() {
    if (!isTimeSynchronized()) return; // Nu inregistram nimic daca timpul nu este sincronizat!

    uint32_t curTs = getCurrentTimestamp();
    if (curTs == 0) return;

    const BmeData &b = bmeGetData();
    if (!b.ready) return;

    const GpsData &g = gpsGetData();

    WeatherRecord rec;
    rec.timestamp = curTs;
    rec.pressure_pa = (uint32_t)(b.pressureCalibratedMb * 100.0f + 0.5f);
    rec.temp_c_x100 = (int16_t)(b.tempC * 100.0f + 0.5f);
    rec.altitude_m = (g.present && g.fix) ? (int16_t)g.altitude : -9999;
    
    char curLoc[7];
    getCurrentLocation(curLoc);
    strncpy(rec.maidenhead, curLoc, 6);

    rec.flags = 0;
    if (g.present && g.fix) rec.flags |= 0x01;
    if (settingsGetQnhOffset() != 0.0f) rec.flags |= 0x02;
    if (pendingGroundTruthState >= 0) {
        rec.flags |= 0x04; // bit 2: ground truth point
        rec.reserved = (uint8_t)pendingGroundTruthState;
    } else {
        rec.reserved = 0;
    }

    // Adaugam in Stratul 2 (PSRAM)
    addToWeather14d(rec);
    ramBufferPending++;

    // Salvare in Flash Stratul 2 la fiecare 20 esantioane (la 3 min = 1 ora) sau la calibrare manuala
    if (ramBufferPending >= WEATHER_RAM_FLUSH_COUNT || pendingGroundTruthState >= 0) {
        flushWeather14dToFlash();
    }

    // Verificam si arhivam orar in Stratul 3
    checkAndRecordHourlyArchive(rec.timestamp);

    lastSampleTime = millis();
    computeForecast();
}

void weatherLoggerUpdate() {
    if (!isTimeSynchronized()) return; // Fara timp cert nu salvam si nu actualizam prognoza

    static bool weatherFirstSyncDone = false;
    if (!weatherFirstSyncDone) {
        weatherFirstSyncDone = true;
        weatherLoggerForceSample();
    }

    // Detectare tranzitie rapida de locatie (ex: GPS a obtinut fix dupa boot sau ne-am deplasat)
    char curLoc[7];
    getCurrentLocation(curLoc);
    if (strncmp(curLoc, currentForecast.locationTag, 6) != 0) {
#if DEBUG_WEATHER
        Serial.printf("[WEATHER] Tranzitie de locatie detectata: '%s' -> '%s'. Evaluare imediata...\n",
                      currentForecast.locationTag, curLoc);
#endif
        weatherLoggerForceSample();
        return;
    }

    uint32_t intervalMin = settingsGetForecastIntervalMin();
    if (intervalMin < 1) intervalMin = 1;
    unsigned long intervalMs = intervalMin * 60UL * 1000UL;

    if (millis() - lastSampleTime >= intervalMs) {
        weatherLoggerForceSample();
    } else {
        // Verificare de siguranta la schimbarea orei pentru arhivare
        checkAndRecordHourlyArchive(getCurrentTimestamp());
    }
}

const WeatherForecastInfo& weatherGetForecast() {
    return currentForecast;
}

uint32_t weatherGetTotalHistoryRecords() {
    return weather14dCount;
}

uint16_t weatherGetRamBufferPending() {
    return ramBufferPending;
}

uint16_t weatherGet14dCount() {
    return weather14dCount;
}

uint16_t weatherGet14dHead() {
    return weather14dHead;
}

const WeatherRecord* weatherGet14dRecords() {
    return weather14dRecords;
}

uint32_t weatherGetArchiveCount() {
    return archiveCount;
}



