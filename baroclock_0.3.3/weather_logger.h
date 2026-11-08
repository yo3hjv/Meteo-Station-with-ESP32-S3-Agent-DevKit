#pragma once
#include <Arduino.h>

#define WEATHER_RAM_FLUSH_COUNT   20   // Salvare in Flash la fiecare 20 esantioane (la 3 min = 1 ora)
#define WEATHER_RAM_HISTORY_COUNT 480  // Istoric recent in RAM: 24h la 3 min (480 * 20B = 9.6 KB)

enum WeatherBaseRef : uint8_t {
    WEATHER_BASE_CLEAR  = 0, // Senin / Frumos
    WEATHER_BASE_CLOUDY = 1, // Cer innorat / Inchis
    WEATHER_BASE_RAIN   = 2  // Ploaie activa / Vreme ploioasa
};

enum WeatherRiskLevel {
    RISK_NORMAL = 0,        // Verde: Presiune stabila sau in crestere
    RISK_CAUTION = 1,       // Galben: Scadere moderata (-1.0 .. -2.0 hPa / 3h)
    RISK_DETERIORATION = 2, // Portocaliu: Scadere rapida (-2.0 .. -3.5 hPa / 3h sau -1.2 hPa / 1h)
    RISK_STORM = 3,         // Rosu: Alerta Furtuna (<-3.5 hPa / 3h sau <-2.0 hPa / 1h)
    RISK_NEW_LOCATION = 4   // Bleu: Locatie noua, acumulare date locale
};

enum WeatherTrendDir {
    TREND_STEADY = 0,       // -> Orizontala (-0.5 .. +0.5 hPa / 3h)
    TREND_RISING = 1,       // ^ Crestere (>= +0.5 hPa / 3h)
    TREND_FALLING = 2       // v Scadere (<= -0.5 hPa / 3h)
};

enum WeatherIconType {
    WEATHER_ICON_SUN = 0,
    WEATHER_ICON_SUN_CLOUD = 1,
    WEATHER_ICON_CLOUD = 2,
    WEATHER_ICON_RAIN = 3,
    WEATHER_ICON_STORM = 4,
    WEATHER_ICON_NEW_LOC = 5
};

// Structura binara compacta (exact 20 octeti per inregistrare - Stratul 2: 14 zile la 3 min)
struct __attribute__((packed)) WeatherRecord {
    uint32_t timestamp;      // Unix timestamp (secunde)
    uint32_t pressure_pa;    // Presiune absoluta in Pascali (rezolutie 0.01 hPa)
    int16_t  temp_c_x100;    // Temperatura in grade C x 100
    int16_t  altitude_m;     // Altitudine GPS in metri (-9999 daca lipseste)
    char     maidenhead[6];  // Grid Maidenhead (ex: "KN34bk" sau "INDOOR")
    uint8_t  flags;          // bit 0: GPS fix valid, bit 1: calibrat
    uint8_t  reserved;       // Padding aliniere
};

// Structura arhivei climatice anuale (Stratul 3: 1 an la 1 ora - exact 20 octeti)
struct __attribute__((packed)) WeatherArchiveRecord {
    uint32_t timestamp;      // Unix epoch rotunjit la ora fixa
    uint16_t pres_avg_pa_10; // Presiune medie a orei (hPa x 10, ex: 10132 = 1013.2 hPa)
    uint16_t pres_min_pa_10; // Presiune minima a orei (hPa x 10)
    uint16_t pres_max_pa_10; // Presiune maxima a orei (hPa x 10)
    int16_t  temp_avg_c_100; // Temp medie a orei (0.01 C)
    int16_t  altitude_m;     // Altitudine GPS (-9999 daca lipseste)
    char     maidenhead[6];  // Grid Maidenhead (ex: "KN34bk")
};

#define WEATHER_14D_MAX_RECORDS 6720  // 14 zile la 3 min (134.4 KB in PSRAM)
#define WEATHER_1Y_MAX_RECORDS  8760  // 365 zile la 1 ora (175.2 KB pe LittleFS)

struct WeatherForecastInfo {
    WeatherRiskLevel risk;
    WeatherTrendDir trend;
    WeatherIconType icon;
    float deltaP_30m;       // hPa
    float deltaP_1h;        // hPa
    float deltaP_3h;        // hPa
    float rateOfChange;     // hPa/ora
    float acceleration;     // hPa/h^2
    char locationTag[7];    // 6 chars + null terminator
    uint16_t localSamplesCount; // Numar de esantioane potrivite pentru locatia curenta
    char statusText[32];    // "[STABIL]", "[ATENTIE]", "[ALERTA FURTUNA]", etc.
};

void weatherLoggerInit();
void weatherLoggerUpdate();
void weatherLoggerForceSample();
void weatherLoggerCalibrate(uint8_t state);
const char* weatherGetStateName(uint8_t state);
const WeatherForecastInfo& weatherGetForecast();
uint32_t weatherGetTotalHistoryRecords();
uint16_t weatherGetRamBufferPending();

// Functii acces Stratul 2 si 3 pentru barograf si diagnoza
uint16_t weatherGet14dCount();
uint16_t weatherGet14dHead();
const WeatherRecord* weatherGet14dRecords();
uint32_t weatherGetArchiveCount();

// Stare sincronizare timp & timestamp UTC cert (fara fallback fictiv)
bool isTimeSynchronized();
uint32_t getCurrentTimestamp();

// Obtinere cod locatie curenta (Maidenhead 4..6-char GPS/manual sau "INDOOR")
void getCurrentLocation(char *outLoc);

