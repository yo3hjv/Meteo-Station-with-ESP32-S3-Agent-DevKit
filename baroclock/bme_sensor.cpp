#include "bme_sensor.h"
#include "weather_logger.h"
#include "temp_calib.h"
#include "setup.h"
#include "wifi_mgr.h"
#include "lcd_driver.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <LittleFS.h>
#include <time.h>

static TwoWire bmeWire = TwoWire(1);
static Adafruit_BME280 bme;
static BmeData currentBmeData = {false, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

// Direct BMP280 calibration and reading fallback if chip is BMP280 (ID 0x58)
struct Bmp280Calib {
    uint16_t dig_T1;
    int16_t  dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
    int32_t  t_fine;
};
static Bmp280Calib bmpCalib;
static bool isBmp280 = false;
static uint8_t activeI2cAddr = 0x76;

static uint8_t i2cRead8(uint8_t addr, uint8_t reg) {
    bmeWire.beginTransmission(addr);
    bmeWire.write(reg);
    bmeWire.endTransmission(false);
    bmeWire.requestFrom(addr, (uint8_t)1);
    return bmeWire.available() ? bmeWire.read() : 0xFF;
}

static uint16_t i2cRead16LE(uint8_t addr, uint8_t reg) {
    bmeWire.beginTransmission(addr);
    bmeWire.write(reg);
    bmeWire.endTransmission(false);
    bmeWire.requestFrom(addr, (uint8_t)2);
    uint8_t lo = bmeWire.read();
    uint8_t hi = bmeWire.read();
    return (uint16_t)(hi << 8) | lo;
}

static int16_t i2cReadS16LE(uint8_t addr, uint8_t reg) {
    return (int16_t)i2cRead16LE(addr, reg);
}

static void i2cWrite8(uint8_t addr, uint8_t reg, uint8_t val) {
    bmeWire.beginTransmission(addr);
    bmeWire.write(reg);
    bmeWire.write(val);
    bmeWire.endTransmission();
}

static bool initBmp280Direct(uint8_t addr) {
    activeI2cAddr = addr;
    bmpCalib.dig_T1 = i2cRead16LE(addr, 0x88);
    bmpCalib.dig_T2 = i2cReadS16LE(addr, 0x8A);
    bmpCalib.dig_T3 = i2cReadS16LE(addr, 0x8C);
    bmpCalib.dig_P1 = i2cRead16LE(addr, 0x8E);
    bmpCalib.dig_P2 = i2cReadS16LE(addr, 0x90);
    bmpCalib.dig_P3 = i2cReadS16LE(addr, 0x92);
    bmpCalib.dig_P4 = i2cReadS16LE(addr, 0x94);
    bmpCalib.dig_P5 = i2cReadS16LE(addr, 0x96);
    bmpCalib.dig_P6 = i2cReadS16LE(addr, 0x98);
    bmpCalib.dig_P7 = i2cReadS16LE(addr, 0x9A);
    bmpCalib.dig_P8 = i2cReadS16LE(addr, 0x9C);
    bmpCalib.dig_P9 = i2cReadS16LE(addr, 0x9E);

    i2cWrite8(addr, 0xF4, 0x57); // normal mode, osrs_t x2, osrs_p x16
    i2cWrite8(addr, 0xF5, 0x10); // filter 16
    isBmp280 = true;
    return true;
}

static void readBmp280Values(float &tempC, float &pressureMb) {
    bmeWire.beginTransmission(activeI2cAddr);
    bmeWire.write(0xF7);
    bmeWire.endTransmission(false);
    bmeWire.requestFrom(activeI2cAddr, (uint8_t)6);

    uint32_t pMsb = bmeWire.read();
    uint32_t pLsb = bmeWire.read();
    uint32_t pXlsb = bmeWire.read();
    int32_t adc_P = (pMsb << 12) | (pLsb << 4) | (pXlsb >> 4);

    uint32_t tMsb = bmeWire.read();
    uint32_t tLsb = bmeWire.read();
    uint32_t tXlsb = bmeWire.read();
    int32_t adc_T = (tMsb << 12) | (tLsb << 4) | (tXlsb >> 4);

    int32_t var1 = ((((adc_T >> 3) - ((int32_t)bmpCalib.dig_T1 << 1))) * ((int32_t)bmpCalib.dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)bmpCalib.dig_T1)) * ((adc_T >> 4) - ((int32_t)bmpCalib.dig_T1))) >> 12) * ((int32_t)bmpCalib.dig_T3)) >> 14;
    bmpCalib.t_fine = var1 + var2;
    tempC = ((bmpCalib.t_fine * 5 + 128) >> 8) / 100.0f;

    int64_t p_var1 = ((int64_t)bmpCalib.t_fine) - 128000;
    int64_t p_var2 = p_var1 * p_var1 * (int64_t)bmpCalib.dig_P6;
    p_var2 = p_var2 + ((p_var1 * (int64_t)bmpCalib.dig_P5) << 17);
    p_var2 = p_var2 + (((int64_t)bmpCalib.dig_P4) << 35);
    p_var1 = ((p_var1 * p_var1 * (int64_t)bmpCalib.dig_P3) >> 8) + ((p_var1 * (int64_t)bmpCalib.dig_P2) << 12);
    p_var1 = (((((int64_t)1) << 47) + p_var1)) * ((int64_t)bmpCalib.dig_P1) >> 33;

    if (p_var1 == 0) {
        pressureMb = 0.0f;
    } else {
        int64_t p = 1048576 - adc_P;
        p = (((p << 31) - p_var2) * 3125) / p_var1;
        p_var1 = (((int64_t)bmpCalib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
        p_var2 = (((int64_t)bmpCalib.dig_P8) * p) >> 19;
        p = ((p + p_var1 + p_var2) >> 8) + (((int64_t)bmpCalib.dig_P7) << 4);
        pressureMb = (float)p / 25600.0f;
    }
}

// Stratul 1: Buffer circular de mare rezolutie in PSRAM (3 ore la 5 secunde = 2.160 esantioane, 17.28 KB)
#define BARO_FINE_SAMPLE_INTERVAL_SEC  5
#define BARO_FINE_HOURS                3
#define BARO_FINE_MAX_SAMPLES          (BARO_FINE_HOURS * 3600 / BARO_FINE_SAMPLE_INTERVAL_SEC) // 2160
#define BARO_FINE_FLASH_PATH           "/baro_fine.dat"
#define BARO_FINE_FLASH_SAVE_INTERVAL  (15UL * 60UL * 1000UL) // 15 minute in milisecunde
#define BARO_LEGACY_FLASH_PATH         "/baro_history.dat"

static BaroSample *baroSamples = nullptr;
static uint32_t baroSampleCount = 0;
static uint32_t baroSampleHead = 0;
static unsigned long lastBaroSampleMillis = 0;
static unsigned long lastBaroFlashSaveMillis = 0;
static uint8_t currentBaroSpan = BARO_SPAN_24H;

// Puncte fizice proiectate pentru ecran (index 31 la 479)
static float baroPressHistory[SCREEN_WIDTH];

static const uint32_t SPAN_SECONDS[BARO_SPAN_COUNT] = {
    600,        // 10m
    1800,       // 30m
    3600,       // 1h
    10800,      // 3h
    21600,      // 6h
    43200,      // 12h
    86400,      // 24h
    259200,     // 3d
    604800,     // 7d
    1209600     // 14d
};

static const char* SPAN_NAMES[BARO_SPAN_COUNT] = {
    "10m", "30m", "1h", "3h", "6h", "12h", "24h", "3d", "7d", "14d"
};

struct __attribute__((packed)) BaroFineHeader {
    uint32_t magic; // 0x4241524F ("BARO")
    uint32_t version; // 2
    uint32_t sampleCount;
    uint32_t head;
    uint32_t sampleIntervalSec;
    uint32_t lastSavedTimestamp;
};


void bmeSensorInit() {
#if DEBUG_PONSTEST
    Serial.printf("[BME280] Starting I2C bus on SDA=IO%d, SCL=IO%d...\n", BME_I2C_SDA, BME_I2C_SCL);
#endif
    bmeWire.begin(BME_I2C_SDA, BME_I2C_SCL, 100000);
    delay(50);

    uint8_t detectedAddr = 0;
#if DEBUG_PONSTEST
    Serial.println("[BME280] Scanning I2C bus (IO48/IO38)...");
#endif
    for (uint8_t addr = 1; addr < 127; addr++) {
        bmeWire.beginTransmission(addr);
        if (bmeWire.endTransmission() == 0) {
#if DEBUG_PONSTEST
            Serial.printf("  -> I2C device found at address 0x%02X\n", addr);
#endif
            if (addr == 0x76 || addr == 0x77) {
                detectedAddr = addr;
            }
        }
    }

    if (detectedAddr == 0) {
        detectedAddr = BME280_I2C_ADDR;
    }

    // Read chip ID at register 0xD0
    uint8_t chipId = i2cRead8(detectedAddr, 0xD0);
    currentBmeData.chipId = chipId;
#if DEBUG_PONSTEST
    Serial.printf("[BME280] Chip ID at 0x%02X is: 0x%02X\n", detectedAddr, chipId);
#endif

    if (chipId == 0x60) {
#if DEBUG_PONSTEST
        Serial.println("[BME280] Detected genuine BME280 (Temp+Pressure+Humidity).");
#endif
        if (bme.begin(detectedAddr, &bmeWire)) {
            currentBmeData.ready = true;
            isBmp280 = false;
        }
    } else if (chipId == 0x58 || chipId == 0x56 || chipId == 0x57) {
#if DEBUG_PONSTEST
        Serial.printf("[BME280] Detected BMP280 series chip (ID 0x%02X). Initializing direct BMP280 driver...\n", chipId);
#endif
        if (initBmp280Direct(detectedAddr)) {
            currentBmeData.ready = true;
            isBmp280 = true;
        }
    } else {
        // Fallback: try Adafruit_BME280 anyway
#if DEBUG_PONSTEST
        Serial.printf("[BME280] Unknown Chip ID 0x%02X, attempting standard begin...\n", chipId);
#endif
        if (bme.begin(detectedAddr, &bmeWire)) {
            currentBmeData.ready = true;
            isBmp280 = false;
        }
    }

    if (currentBmeData.ready) {
#if DEBUG_PONSTEST
        Serial.println("[BME280] Sensor ready and operational!");
#endif
    } else {
        Serial.println("[BME280] ERROR: Sensor initialization failed!");
    }
}

void bmeSensorUpdate() {
    if (!currentBmeData.ready) return;

    float rawT = 0.0f;
    if (isBmp280) {
        readBmp280Values(rawT, currentBmeData.pressureRawMb);
    } else {
        rawT = bme.readTemperature();
        currentBmeData.pressureRawMb = bme.readPressure() / 100.0f; // Pa -> hPa/mb
    }

    currentBmeData.tempRawC = rawT;
    currentBmeData.tempC = tempCalibrateBme(rawT);
    currentBmeData.tempF = currentBmeData.tempC * 1.8f + 32.0f;

    float offset = settingsGetQnhOffset();
    currentBmeData.pressureCalibratedMb = currentBmeData.pressureRawMb + offset;

    // La prima citire valida, daca timpul este sincronizat si nu avem istoric, adaugam primul punct
    if (isTimeSynchronized() && baroSampleCount == 0 && currentBmeData.pressureCalibratedMb > 300.0f) {
        baroPressHistory[SCREEN_WIDTH - 1] = currentBmeData.pressureCalibratedMb;
    }

#if DEBUG_BME
    static unsigned long lastBmeDebug = 0;
    if (millis() - lastBmeDebug >= 3000) {
        lastBmeDebug = millis();
        Serial.printf("[BME280 DEBUG] Chip ID: 0x%02X | Temp: %.2f C (%.2f F) | Raw Pres: %.2f mb | Calibrated: %.2f mb (Offset: %.2f)\n",
                      currentBmeData.chipId,
                      currentBmeData.tempC, currentBmeData.tempF,
                      currentBmeData.pressureRawMb, currentBmeData.pressureCalibratedMb, offset);
    }
#endif
}

const BmeData& bmeGetData() {
    return currentBmeData;
}

// ----------------------------------------------------------------------------
// BAROGRAPH DRAWING & LOGIC (ROLLING STRIP CHART & STATIC RIGHT-ALIGNED TIME AXIS)
// ----------------------------------------------------------------------------

void baroGraphDrawTimeAxis() {
    // Deprecated: desenarea scalei de timp este gestionata de LVGL pe lv_scale (objects.time_span_scale) in gui.cpp
}

void baroGraphRedrawBox() {
    // Deprecated: desenarea se face acum pe canvas LVGL in gui.cpp
}

void baroGraphFlushToFlash() {
    if (!baroSamples || baroSampleCount == 0) return;
    File f = LittleFS.open(BARO_FINE_FLASH_PATH, FILE_WRITE);
    if (!f) {
        Serial.println("[BARO] EROARE: Nu s-a putut deschide /baro_fine.dat pentru scriere!");
        return;
    }
    BaroFineHeader hdr;
    hdr.magic = 0x4241524F;
    hdr.version = 2;
    hdr.sampleCount = baroSampleCount;
    hdr.head = baroSampleHead;
    hdr.sampleIntervalSec = BARO_FINE_SAMPLE_INTERVAL_SEC;
    hdr.lastSavedTimestamp = getCurrentTimestamp();
    f.write((const uint8_t*)&hdr, sizeof(hdr));

    uint8_t *ptr = (uint8_t*)baroSamples;
    size_t totalBytes = baroSampleCount * sizeof(BaroSample);
    size_t written = 0;
    while (written < totalBytes) {
        size_t chunk = (totalBytes - written > 4096) ? 4096 : (totalBytes - written);
        f.write(ptr + written, chunk);
        written += chunk;
        yield();
    }
    f.close();
    lastBaroFlashSaveMillis = millis();
#if DEBUG_BARO
    Serial.printf("[BARO] Stratul 1 salvat in LittleFS: %u esantioane (%u KB)\n",
                  baroSampleCount, (uint32_t)(totalBytes / 1024));
#endif
}

static void baroGraphLoadFromFlash() {
    if (!baroSamples) return;
    if (!LittleFS.exists(BARO_FINE_FLASH_PATH)) {
#if DEBUG_PONSTEST
        Serial.println("[BARO] Niciun fisier /baro_fine.dat salvat anterior pe LittleFS.");
#endif
        return;
    }
    File f = LittleFS.open(BARO_FINE_FLASH_PATH, FILE_READ);
    if (!f) return;
    BaroFineHeader hdr;
    if (f.read((uint8_t*)&hdr, sizeof(hdr)) == sizeof(hdr) && hdr.magic == 0x4241524F) {
        if (hdr.sampleCount > 0 && hdr.sampleCount <= BARO_FINE_MAX_SAMPLES) {
            uint8_t *ptr = (uint8_t*)baroSamples;
            size_t totalBytes = hdr.sampleCount * sizeof(BaroSample);
            size_t readBytes = 0;
            while (readBytes < totalBytes) {
                size_t chunk = (totalBytes - readBytes > 4096) ? 4096 : (totalBytes - readBytes);
                size_t r = f.read(ptr + readBytes, chunk);
                if (r == 0) break;
                readBytes += r;
                yield();
            }
            if (readBytes == totalBytes) {
                baroSampleCount = hdr.sampleCount;
                baroSampleHead = hdr.head % BARO_FINE_MAX_SAMPLES;

                // Curatare eșantioane parazite (< 1700000000UL) si re-ordonare cronologica
                if (baroSampleCount > 0) {
                    BaroSample* cleanBuf = (BaroSample*)malloc(baroSampleCount * sizeof(BaroSample));
                    if (cleanBuf) {
                        uint16_t oldestIdx = (baroSampleHead >= baroSampleCount) ?
                                             (baroSampleHead - baroSampleCount) :
                                             (BARO_FINE_MAX_SAMPLES + baroSampleHead - baroSampleCount);
                        uint16_t validCount = 0;
                        for (uint16_t i = 0; i < baroSampleCount; i++) {
                            uint16_t phys = (oldestIdx + i) % BARO_FINE_MAX_SAMPLES;
                            if (baroSamples[phys].timestamp >= 1700000000UL) {
                                cleanBuf[validCount++] = baroSamples[phys];
                            }
                        }
                        if (validCount > 0) {
                            qsort(cleanBuf, validCount, sizeof(BaroSample), [](const void* a, const void* b) -> int {
                                uint32_t tA = ((const BaroSample*)a)->timestamp;
                                uint32_t tB = ((const BaroSample*)b)->timestamp;
                                if (tA < tB) return -1;
                                if (tA > tB) return 1;
                                return 0;
                            });
                            memcpy(baroSamples, cleanBuf, validCount * sizeof(BaroSample));
                            baroSampleCount = validCount;
                            baroSampleHead = validCount % BARO_FINE_MAX_SAMPLES;
                        } else {
                            baroSampleCount = 0;
                            baroSampleHead = 0;
                        }
                        free(cleanBuf);
                    }
                }

#if (DEBUG_BARO || DEBUG_PONSTEST)
                Serial.printf("[BARO] Incarcat Stratul 1 din LittleFS in PSRAM: %u esantioane valide (%u KB, head=%u)\n",
                              baroSampleCount, (uint32_t)(baroSampleCount * sizeof(BaroSample) / 1024), baroSampleHead);
#endif
            }
        }
    }
    f.close();
}

static void addBaroSample(float pressureMb) {
    if (!baroSamples) return;
    if (!isTimeSynchronized()) return; // Fara sincronizare valida nu se genereaza timestamp!
    uint32_t ts = getCurrentTimestamp();
    if (ts == 0) return;
    baroSamples[baroSampleHead] = { ts, pressureMb };
    baroSampleHead = (baroSampleHead + 1) % BARO_FINE_MAX_SAMPLES;
    if (baroSampleCount < BARO_FINE_MAX_SAMPLES) {
        baroSampleCount++;
    }
}

uint8_t baroGraphGetTimeSpan() {
    return currentBaroSpan;
}

uint32_t baroGraphGetSpanSeconds(uint8_t spanIdx) {
    if (spanIdx >= BARO_SPAN_COUNT) spanIdx = BARO_SPAN_24H;
    return SPAN_SECONDS[spanIdx];
}

const char* baroGraphGetSpanName(uint8_t spanIdx) {
    if (spanIdx >= BARO_SPAN_COUNT) spanIdx = BARO_SPAN_24H;
    return SPAN_NAMES[spanIdx];
}

void baroGraphSetTimeSpan(uint8_t spanIdx) {
    if (spanIdx >= BARO_SPAN_COUNT) spanIdx = BARO_SPAN_COUNT - 1;
    currentBaroSpan = spanIdx;
    settingsSetBaroTimeWindow(spanIdx);
    baroGraphResetBaseline();
}

void baroGraphZoomIn() {
    if (currentBaroSpan > 0) {
        baroGraphSetTimeSpan(currentBaroSpan - 1);
    }
}

void baroGraphZoomOut() {
    if (currentBaroSpan < BARO_SPAN_COUNT - 1) {
        baroGraphSetTimeSpan(currentBaroSpan + 1);
    }
}

void baroGraphInit() {
    currentBaroSpan = settingsGetBaroTimeWindow();
    if (currentBaroSpan >= BARO_SPAN_COUNT) currentBaroSpan = BARO_SPAN_24H;

    for (int i = 0; i < SCREEN_WIDTH; i++) {
        baroPressHistory[i] = -1.0f;
    }

    if (psramFound()) {
        size_t allocSize = BARO_FINE_MAX_SAMPLES * sizeof(BaroSample);
        baroSamples = (BaroSample*)ps_malloc(allocSize);
        if (baroSamples) {
#if DEBUG_PONSTEST
            Serial.printf("[BARO] Buffer fin PSRAM alocat: %u esantioane (%u KB, 3h la 5s)\n",
                          BARO_FINE_MAX_SAMPLES, (uint32_t)(allocSize / 1024));
#endif
            memset(baroSamples, 0, allocSize);
        } else {
            Serial.println("[BARO] EROARE: Alocarea PSRAM a esuat!");
        }
    } else {
        Serial.println("[BARO] ATENTIE: PSRAM nedisponibil!");
    }

    // Initializare LittleFS
    LittleFS.begin(true);

    // Daca exista fisierul vechi urias de 1.84 MB, il stergem pentru a elibera spatiul LittleFS
    if (LittleFS.exists(BARO_LEGACY_FLASH_PATH)) {
        LittleFS.remove(BARO_LEGACY_FLASH_PATH);
#if DEBUG_PONSTEST
        Serial.println("[BARO] Sters fisierul vechi /baro_history.dat (1.84 MB) pentru a elibera LittleFS.");
#endif
    }

    baroGraphLoadFromFlash();

    lastBaroSampleMillis = millis();
    lastBaroFlashSaveMillis = millis();

    baroGraphResetBaseline();
}

void baroGraphResetBaseline() {
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        baroPressHistory[i] = -1.0f;
    }

    // Afisam DOAR DUPA obtinerea sincronizarii (NTP sau GPS)
    if (!isTimeSynchronized()) {
        return;
    }

    uint32_t nowTs = getCurrentTimestamp();
    if (nowTs == 0) return;

    // Obtinem locatia curenta pentru filtrare stricta (GPS fix -> manual user -> fallback)
    char curLoc[7];
    getCurrentLocation(curLoc);

    uint32_t spanSec = baroGraphGetSpanSeconds(currentBaroSpan);
    const int rightHistIdx = SCREEN_WIDTH - 1; // 479
    const int leftHistIdx = 31;
    const int activeWidth = rightHistIdx - leftHistIdx; // 448
    uint32_t pixelSec = spanSec / activeWidth;

    const WeatherRecord* records = weatherGet14dRecords();
    uint16_t wCount = weatherGet14dCount();
    uint16_t wHead = weatherGet14dHead();
    uint32_t oldestIdx2 = (wCount > 0) ? ((wHead - wCount + WEATHER_14D_MAX_RECORDS) % WEATHER_14D_MAX_RECORDS) : 0;

    uint32_t oldestIdx1 = (baroSampleCount > 0) ? ((baroSampleHead - baroSampleCount + BARO_FINE_MAX_SAMPLES) % BARO_FINE_MAX_SAMPLES) : 0;

    uint32_t maxGap1 = (pixelSec * 2 > 30) ? (pixelSec * 2) : 30;    // toleranta esantioane 5s
    uint32_t maxGap2 = (pixelSec * 3 > 600) ? (pixelSec * 3) : 600;  // minim 10m toleranta pentru esantioane 3m

    for (int histIdx = rightHistIdx; histIdx >= leftHistIdx; histIdx--) {
        int dx = rightHistIdx - histIdx;
        float tOffset = (float)dx * ((float)spanSec / (float)activeWidth);
        uint32_t targetTs = (nowTs >= (uint32_t)tOffset) ? (nowTs - (uint32_t)tOffset) : 0;

        if (currentBaroSpan <= BARO_SPAN_3H) {
            // --- 1. Stratul 1 (5s): Cautare binara ---
            if (baroSamples && baroSampleCount > 0) {
                int low = 0;
                int high = (int)baroSampleCount - 1;
                while (low <= high) {
                    int mid = low + (high - low) / 2;
                    int phys = (oldestIdx1 + mid) % BARO_FINE_MAX_SAMPLES;
                    if (baroSamples[phys].timestamp < targetTs) {
                        low = mid + 1;
                    } else {
                        high = mid - 1;
                    }
                }

                uint32_t bestDiff = 0xFFFFFFFF;
                int bestPhys = -1;
                for (int k = low - 4; k <= low + 4; k++) {
                    if (k < 0 || k >= (int)baroSampleCount) continue;
                    int phys = (oldestIdx1 + k) % BARO_FINE_MAX_SAMPLES;
                    uint32_t sTs = baroSamples[phys].timestamp;
                    if (sTs < 1700000000UL) continue;
                    uint32_t diff = (sTs >= targetTs) ? (sTs - targetTs) : (targetTs - sTs);
                    if (diff < bestDiff) {
                        bestDiff = diff;
                        bestPhys = phys;
                    }
                }

                if (bestPhys >= 0 && bestDiff <= maxGap1) {
                    baroPressHistory[histIdx] = baroSamples[bestPhys].pressure;
                }
            }

            // --- 2. Fallback inteligent pe Stratul 2 (3min) filtrat strict pe curLoc ---
            if (baroPressHistory[histIdx] < 0.0f && records && wCount > 0) {
                int low = 0;
                int high = (int)wCount - 1;
                while (low <= high) {
                    int mid = low + (high - low) / 2;
                    int phys = (oldestIdx2 + mid) % WEATHER_14D_MAX_RECORDS;
                    if (records[phys].timestamp < targetTs) {
                        low = mid + 1;
                    } else {
                        high = mid - 1;
                    }
                }

                uint32_t bestDiff = 0xFFFFFFFF;
                int bestPhys = -1;
                for (int k = low - 16; k <= low + 16; k++) {
                    if (k < 0 || k >= (int)wCount) continue;
                    int phys = (oldestIdx2 + k) % WEATHER_14D_MAX_RECORDS;
                    if (records[phys].timestamp < 1700000000UL) continue;
                    // Filtrare case-insensitive pe locatie (KN34bk == kn34bk)
                    if (strncasecmp(records[phys].maidenhead, curLoc, 6) != 0) continue;

                    uint32_t sTs = records[phys].timestamp;
                    uint32_t diff = (sTs >= targetTs) ? (sTs - targetTs) : (targetTs - sTs);
                    if (diff < bestDiff) {
                        bestDiff = diff;
                        bestPhys = phys;
                    }
                }

                if (bestPhys >= 0 && bestDiff <= maxGap2) {
                    baroPressHistory[histIdx] = (float)records[bestPhys].pressure_pa / 100.0f;
                }
            }
        } else {
            // --- Stratul 2 (6h .. 14d): Cautare binara filtrata strict pe curLoc ---
            if (!records || wCount == 0) continue;

            int low = 0;
            int high = (int)wCount - 1;
            while (low <= high) {
                int mid = low + (high - low) / 2;
                int phys = (oldestIdx2 + mid) % WEATHER_14D_MAX_RECORDS;
                if (records[phys].timestamp < targetTs) {
                    low = mid + 1;
                } else {
                    high = mid - 1;
                }
            }

            uint32_t bestDiff = 0xFFFFFFFF;
            int bestPhys = -1;
            for (int k = low - 16; k <= low + 16; k++) {
                if (k < 0 || k >= (int)wCount) continue;
                int phys = (oldestIdx2 + k) % WEATHER_14D_MAX_RECORDS;
                if (records[phys].timestamp < 1700000000UL) continue;
                // Filtrare case-insensitive pe locatie (KN34bk == kn34bk)
                if (strncasecmp(records[phys].maidenhead, curLoc, 6) != 0) continue;

                uint32_t sTs = records[phys].timestamp;
                uint32_t diff = (sTs >= targetTs) ? (sTs - targetTs) : (targetTs - sTs);
                if (diff < bestDiff) {
                    bestDiff = diff;
                    bestPhys = phys;
                }
            }

            if (bestPhys >= 0 && bestDiff <= maxGap2) {
                baroPressHistory[histIdx] = (float)records[bestPhys].pressure_pa / 100.0f;
            } else {
                baroPressHistory[histIdx] = -1.0f;
            }
        }
    }
}

void baroGraphResetTimer() {
    lastBaroSampleMillis = millis();
}

const float* baroGetPressHistory() {
    return baroPressHistory;
}

void baroGraphUpdate() {
    if (!currentBmeData.ready) return;

    // Detectare tranzitie de locatie pentru Stratul 1
    char curLoc[7];
    getCurrentLocation(curLoc);
    static char baroLastLoc[7] = "";
    if (baroLastLoc[0] != '\0' && strncmp(baroLastLoc, curLoc, 6) != 0) {
        // Schimbare de locatie detectata: resetam Stratul 1 pentru a nu amesteca datele istorice
        baroSampleCount = 0;
        baroSampleHead = 0;
        baroGraphResetBaseline();
    }
    strncpy(baroLastLoc, curLoc, 6);
    baroLastLoc[6] = '\0';

    static bool baroTimeWasSynced = false;
    bool synced = isTimeSynchronized();
    if (synced && !baroTimeWasSynced) {
        baroTimeWasSynced = true;
        // Prima sincronizare obtinuta: luam primul esantion si redesenam tot barograful
        addBaroSample(currentBmeData.pressureCalibratedMb);
        baroGraphResetBaseline();
    }

    if (!synced) return; // Fara timp cert, nu inregistram si nu afisam

    unsigned long now = millis();

    // 1. Esantionare la 5 secunde in bufferul PSRAM (Stratul 1)
    if (now - lastBaroSampleMillis >= 5000) {
        lastBaroSampleMillis = now;
        addBaroSample(currentBmeData.pressureCalibratedMb);
        baroGraphResetBaseline();
    }

    // 2. Salvare de siguranta pe LittleFS la fiecare 15 minute (Stratul 1)
    if (now - lastBaroFlashSaveMillis >= BARO_FINE_FLASH_SAVE_INTERVAL) {
        lastBaroFlashSaveMillis = now;
        baroGraphFlushToFlash();
    }
}
