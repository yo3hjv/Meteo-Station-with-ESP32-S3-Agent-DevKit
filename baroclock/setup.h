#pragma once
#include <Arduino.h>

// ============================================================================
// BAROCLOCK - CONFIGURATION & HARDWARE SETTINGS
// ESP32 Agent Dev Kit
// ============================================================================

extern const char* FIRMWARE_VERSION;

// ----------------------------------------------------------------------------
// DEBUG FLAGS (Set 1 to enable Serial monitor diagnostics, 0 to disable)
// ----------------------------------------------------------------------------
#define DEBUG_PONSTEST        0  // 1 = Afisare mesaje autotest / initializare module la boot (POST), 0 = silentios la pornire
#define DEBUG_GPS             0  // Prints incoming NMEA characters & TinyGPS stats
#define DEBUG_BME             0  // Scans I2C bus at boot & prints raw BME280 readings
#define DEBUG_PROXIMITY       0  // 0 = Inactiv (implicit), 1 = Dump registri la boot & log citiri senzor proximitate
#define DEBUG_TOUCH           0  // 1 = Afisare Serial coordonate touch & stare toggle alarme (0 = dezactivat)
#define DEBUG_SOUND           0  // 1 = Afisare actualizari volum ES8311 & stare alarme (0 = dezactivat)
#define DEBUG_WEATHER         0  // 1 = Afisare salvari LittleFS & diagnoza meteo (0 = dezactivat)
#define DEBUG_BARO            0  // 1 = Afisare detectie gesturi slide & istoric barograf (0 = dezactivat)

// ----------------------------------------------------------------------------
// PIN DEFINITIONS
// ----------------------------------------------------------------------------
// Factory reset pin for WiFi credentials (Active LOW with internal Pull-up)
#define PIN_WIFI_RESET_BOOT   46

// BME280 I2C (Secondary I2C bus)
#define BME_I2C_SDA           48
#define BME_I2C_SCL           38
#define BME280_I2C_ADDR       0x76

// GPS Hardware Serial (UART1)
#define GPS_RX_PIN            41  // ESP32 RX <- semnalul GPS (TXD GP-02)
#define GPS_TX_PIN            40  // ESP32 TX -> portul RX UART GP-02 (RXD GP-02)
#define GPS_BAUDRATE          9600
#define GPS_BOOT_TIMEOUT_MS   5000

// Internal I2C Bus for AW9523 IO Expander
#define INTERNAL_I2C_SDA      5
#define INTERNAL_I2C_SCL      4
#define AW9523_ADDR           0x59

// LCD 8080 8-bit Parallel Pins
#define LCD_RS                45
#define LCD_WR                10
#define LCD_DB0               9
#define LCD_DB1               3
#define LCD_DB2               8
#define LCD_DB3               18
#define LCD_DB4               17
#define LCD_DB5               16
#define LCD_DB6               15
#define LCD_DB7               7

// AW9523 Register Bits
#define LCD_RST_BIT           0x40  // P1_6 controls LCD Reset
#define AW9523_PA_CTRL_BIT    0x20  // P1_5 activeaza amplificatorul audio difuzor

// ----------------------------------------------------------------------------
// AUDIO CODEC (ES8311) & DIFUZOR
// ----------------------------------------------------------------------------
#define ES8311_I2C_ADDR             0x18
#define I2S_MCLK_PIN                47
#define I2S_BCLK_PIN                14
#define I2S_WS_PIN                  13
#define I2S_DOUT_PIN                12

// Parametri Anti-Pop & Sincronizare Cale Audio (PA / ES8311 / I2S)
#define AUDIO_PA_SETTLING_MS        100   // Pauza de stabilizare polarizare PA dupa pornire (ms)
#define AUDIO_RAMP_ATTACK_MS        10     // Rampa de atac fade-in (ms)
#define AUDIO_RAMP_DECAY_MS         5     // Rampa de stingere fade-out (ms)
#define AUDIO_POST_TONE_SILENCE_MS  10    // Pauza post-ton inainte de oprire PA (ms)

// Parametri Test Sunet (Difuzor)
#define BEEP_FREQUENCY_HZ           1200  // Frecventa bip test: 1200 Hz
#define BEEP_DURATION_MS            300   // Durata bip test: 300 ms
#define BEEP_VOLUME_PERCENT         60    // Volum bip difuzor (10% .. 100%)
#define BEEP_TEST_DEFAULT_ON        true  // Activ implicit (ON)

// ----------------------------------------------------------------------------
// SEMNALIZARI ACUSTICE & ALARME (DIFUZOR I2S ES8311)
// ----------------------------------------------------------------------------
// 1. Chime (Bip orar dublu la fix, tip ceas electronic de mana)
#define CHIME_DEFAULT_ENABLED       true
#define CHIME_FREQUENCY_HZ          3200  // Frecventa bip orar dublu (Hz, default 3200 Hz)
#define CHIME_DURATION_MS           70   // Durata fiecarui ton orar (ms, default 100 ms)
#define CHIME_PAUSE_MS              60   // Pauza dintre cele doua tonuri (ms, default 100 ms)
#define CHIME_VOLUME_PERCENT        50    // Volum bip orar (10% .. 100%)

// 2. Alarma Ceas Desteptator (5 bipuri rapide + pauza, repetat ciclic)
#define ALARM_DEFAULT_ENABLED       false
#define ALARM_DEFAULT_HOUR          7
#define ALARM_DEFAULT_MINUTE        0
#define ALARM_DEFAULT_PERIODICITY   0     // 0 = Zilnic, 1 = Luni-Vineri, 2 = O singura data
#define ALARM_FREQUENCY_HZ          2400  // Frecventa bip ceas (Hz)
#define ALARM_BEEP_DURATION_MS      100    // Durata fiecarui bip rapid (ms)
#define ALARM_BEEP_PAUSE_MS         70    // Pauza intre bipuri in salva de 5 (ms)
#define ALARM_CYCLE_PAUSE_MS        800   // Pauza intre ciclurile de 5 bipuri (ms)
#define ALARM_VOLUME_PERCENT        80    // Volum alarma ceas (10% .. 100%)
#define ALARM_AUTO_STOP_SEC         120   // Oprire automata dupa 2 minute daca nu este atins ecranul

// 3. Alarma Deteriorare Vreme (3 bipuri - emisa o singura data la intrarea in risc)
#define WEATHER_ALERT_DEFAULT_EN    true
#define WTR_DET_FREQUENCY_HZ        1800  // Frecventa avertizare deteriorare (Hz)
#define WTR_DET_BEEP_DURATION_MS    150   // Durata bip (ms)
#define WTR_DET_VOLUME_PERCENT      70    // Volum deteriorare (10% .. 100%)

// 4. Alarma Furtuna (Morse S.O.S: ... --- ..., 1 minut sau pana la anulare prin touch)
#define STORM_ALERT_DEFAULT_EN      true
#define STORM_FREQUENCY_HZ          1200  // Frecventa sunet furtuna (Hz)
#define STORM_DOT_DURATION_MS       150   // Durata punct Morse (ms)
#define STORM_DASH_DURATION_MS      280   // Durata linie Morse (ms)
#define STORM_VOLUME_PERCENT        90    // Volum alerta furtuna (10% .. 100%)
#define STORM_TIMEOUT_SEC           60    // 1 minut durata maxima de sunat

// 5. Touch Feedback Click (Confirmare tactila la tap / gesturi / apasare satelit)
#define FEEDBACK_DEFAULT_ENABLED    true
#define FEEDBACK_FREQUENCY_HZ       2400  // Frecventa click tactil (Hz, default 2400 Hz)
#define FEEDBACK_DURATION_MS        200   // Durata click tactil (ms, default 200 ms)
#define FEEDBACK_VOLUME_PERCENT     45    // Volum click tactil (10% .. 100%, default 45%)

// ----------------------------------------------------------------------------
// TOUCH CONTROLLER (FT6336 pe magistrala I2C interna)
// ----------------------------------------------------------------------------
#define TOUCH_I2C_ADDR              0x38
#define TOUCH_INT_PIN               6


// ----------------------------------------------------------------------------
// SENZOR PROXIMITATE (MK-TP2016)
// ----------------------------------------------------------------------------
#define ENABLE_PROXIMITY_SENSOR     0     // 1 = Activ, 0 = Dezactivat complet (fara I2C, fara consum)
#define PROXIMITY_I2C_ADDR          0x39
#define PROXIMITY_THRESHOLD         300   // Prag detectie apropiere (raw counts, calibrabil)
#define PROXIMITY_COOLDOWN_MS       1000  // Timp minim intre declansari consecutive (ms)
#define PROXIMITY_READ_INTERVAL_MS  100   // Interval verificare senzor (ms)

// Display Dimensions (Landscape: 480 x 320)
#define SCREEN_WIDTH          480
#define SCREEN_HEIGHT         320

// ----------------------------------------------------------------------------
// COLOR DEFINITIONS (RGB565 - Utilizate pentru Diagnoza GNSS si Curatare Display)
// Nota: Ecranul principal este gestionat vectorial prin LVGL / EEZ Studio.
// ----------------------------------------------------------------------------
#define COLOR_BLACK           0x0000
#define COLOR_WHITE           0xFFFF
#define COLOR_RED             0xF800
#define COLOR_GREEN           0x07E0
#define COLOR_YELLOW          0xFFE0
#define COLOR_CYAN            0x07FF
#define COLOR_ORANGE          0xFD20
#define COLOR_DARK_GRAY       0x39E7
#define COLOR_GRAY            0x8410
#define COLOR_BG              COLOR_BLACK

// ----------------------------------------------------------------------------
// PROFILURI CROMATICE ECRAN PRINCIPAL (LVGL GUI THEMES)
// ----------------------------------------------------------------------------
#define DEFAULT_UI_THEME              0     // 0 = Monochrome, 1 = Amber CRT, 2 = Green Matrix, 3 = Cyan Electric, 4 = Night Red, 5 = Original EEZ
#define UI_THEME_COUNT                6

struct UiThemePalette {
    const char* name;
    uint32_t bgColor;       // Fundal ecran si ecran interior
    uint32_t panelColor;    // Fundal panouri / casete
    uint32_t borderColor;  // Chenare, separatoare, scala
    uint32_t textPrimary;   // Ora, minute, presiune, temperatura mare
    uint32_t textSecondary; // Etichete, unitati, status
    uint32_t textDate;      // Data calendaristica (ex: 05 NOV 2026)
    uint32_t textDay;       // Ziua saptamanii (ex: THU / JOI)
    uint32_t accentColor;   // Iconita satelit 3D FIX, indicatori activi
    bool isEezOriginal;
};

// 0: Monochrome High-Contrast (Alb / Gri pe Negru)
#define THEME_0_NAME        "Monochrome"
#define THEME_0_BG          0x000000
#define THEME_0_PANEL       0x000000
#define THEME_0_BORDER      0x404040
#define THEME_0_TEXT_PRI    0xFFFFFF
#define THEME_0_TEXT_SEC    0xB0B0B0
#define THEME_0_TEXT_DATE   0xFFFFFF
#define THEME_0_TEXT_DAY    0xFFFFFF
#define THEME_0_ACCENT      0xFFFFFF

// 1: Amber CRT (Vintage Chihlimbar)
#define THEME_1_NAME        "Amber CRT"
#define THEME_1_BG          0x000000
#define THEME_1_PANEL       0x080400
#define THEME_1_BORDER      0x553000
#define THEME_1_TEXT_PRI    0xFFB000
#define THEME_1_TEXT_SEC    0xD08800
#define THEME_1_TEXT_DATE   0xFFB000
#define THEME_1_TEXT_DAY    0xFFC020
#define THEME_1_ACCENT      0xFFC020

// 2: Green Matrix (Vintage Fosfor Verde - Luminozitate Uniforma)
#define THEME_2_NAME        "Green Matrix"
#define THEME_2_BG          0x000000
#define THEME_2_PANEL       0x000C04
#define THEME_2_BORDER      0x004A18
#define THEME_2_TEXT_PRI    0x00FF66
#define THEME_2_TEXT_SEC    0x00FF66
#define THEME_2_TEXT_DATE   0x00FF66
#define THEME_2_TEXT_DAY    0x00FF66
#define THEME_2_ACCENT      0x33FF88

// 3: Cyan Electric (Cyberpunk High-Contrast - Luminozitate Uniforma)
#define THEME_3_NAME        "Cyan Electric"
#define THEME_3_BG          0x000000
#define THEME_3_PANEL       0x000A14
#define THEME_3_BORDER      0x004560
#define THEME_3_TEXT_PRI    0x00E5FF
#define THEME_3_TEXT_SEC    0x00E5FF
#define THEME_3_TEXT_DATE   0x00E5FF
#define THEME_3_TEXT_DAY    0x00E5FF
#define THEME_3_ACCENT      0x33ECFF

// 4: Night Red (Rubin / LED Rosu Vintage)
#define THEME_4_NAME        "Night Red"
#define THEME_4_BG          0x000000
#define THEME_4_PANEL       0x0E0202
#define THEME_4_BORDER      0x551212
#define THEME_4_TEXT_PRI    0xFF3B30
#define THEME_4_TEXT_SEC    0xCC292B
#define THEME_4_TEXT_DATE   0xFF3B30
#define THEME_4_TEXT_DAY    0xFF6961
#define THEME_4_ACCENT      0xFF6961

// 5: Original EEZ (Reflective Cyan LCD)
#define THEME_5_NAME        "Original EEZ"
#define THEME_5_BG          0x000000
#define THEME_5_PANEL       0x4FC1C5
#define THEME_5_BORDER      0x191919
#define THEME_5_TEXT_PRI    0x212121
#define THEME_5_TEXT_SEC    0x101010
#define THEME_5_TEXT_DATE   0x212121
#define THEME_5_TEXT_DAY    0x212121
#define THEME_5_ACCENT      0x000000

// Weather Forecast - Status Strings
#define WEATHER_TXT_NORMAL         "-STABLE-"
#define WEATHER_TXT_STEADY_CLEAR   "-STEADY - FAIR-"
#define WEATHER_TXT_STEADY_CLOUDY  "-STEADY-CLOUDY-"
#define WEATHER_TXT_STEADY_RAIN    "-STEADY - RAIN-"
#define WEATHER_TXT_IMPROVING      "-IMPROVING-"
#define WEATHER_TXT_CAUTION        "-CAUTION-"
#define WEATHER_TXT_DETERIORATE    "-DETERIORATING-"
#define WEATHER_TXT_RAIN_POSS      "-POSSIBLE RAIN-"
#define WEATHER_TXT_RAIN_CONT      "-CONTINUOUS RAIN-"
#define WEATHER_TXT_STORM          "-STORM WARNING-"
#define WEATHER_TXT_NEW_LOC        "-NEW LOCATION-"
#define WEATHER_TXT_INIT           "-INITIALIZING-"
#define WEATHER_TXT_ACQUIRING      "AQUIRING"

// ----------------------------------------------------------------------------
// MULTI-POINT TEMPERATURE CALIBRATION (11 BANDS WITH LINEAR INTERPOLATION)
// ----------------------------------------------------------------------------
#define TEMP_CALIB_BANDS_COUNT        11

// Bands: [-20..-10), [-10..-5), [-5..+5), [5..10), [10..15), [15..20), [20..25), [25..30), [30..35), [35..40), [40..45]
// Nodes: -15.0, -7.5, 0.0, 7.5, 12.5, 17.5, 22.5, 27.5, 32.5, 37.5, 42.5
// Default compile-time calibration offsets (Celsius) - 0.0f means no offset / uncalibrated
#define BME_DEFAULT_CALIB_OFFSETS     { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }
#define EXT_1_DEFAULT_CALIB_OFFSETS   { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }

// ----------------------------------------------------------------------------
// DEFAULT APPLICATION SETTINGS
// ----------------------------------------------------------------------------
#define AP_SSID_DEFAULT       "BaroClock"
#define AP_PASS_DEFAULT       ""                // Open network
#define DEFAULT_TIMEZONE_MIN  120               // Default UTC offset: +2 hours (120 min)
#define DEFAULT_DST           false             // Daylight Saving Time (+1h)
#define DEFAULT_ROTATION_180  false             // Default normal landscape
#define DEFAULT_TEMP_UNIT_F   false             // false = Celsius, true = Fahrenheit
#define DEFAULT_HIGH_SENS     true              // true = High sensitivity, false = Low
#define DEFAULT_BARO_INTERVAL_SEC  1.5f              // Secunde intre citiri pentru grafic (0.1s .. 300s / 5min)
#define DEFAULT_FORECAST_INT_MIN   3                 // Minute intre esantionari prognoza meteo (1min .. 30min)
#define BARO_BOX_HEIGHT            50                // Inaltime caseta barograf in pixeli (redus de la 75 la 50)
#define DEFAULT_DISPLAY_PAGE       0                 // 0 = Ecran Ceas Principal, 1 = Ecran Diagnostic GNSS
#define GNSS_PAGE_TIMEOUT_SEC      20                // Secunde pana la revenirea automata din ecranul GNSS Info
#define DEFAULT_GNSS_MODE          3                 // 1 = Doar GPS, 2 = Doar BeiDou, 3 = GPS + BeiDou (ALL)
#define DEFAULT_NMEA_ESSENTIAL     true              // true = Doar fraze esentiale, false = Toate
#define DEFAULT_NMEA_SERIAL_DEBUG  false             // true = Stream NMEA in Serial Monitor

// ----------------------------------------------------------------------------
// MOTOR DE CEAS & PROGRAMATOR SINCRONIZARE (CLOCK ENGINE & SLEW)
// ----------------------------------------------------------------------------
#define CLOCK_SYNC_INTERVAL_HOURS   12    // Sincronizare la fiecare 12 ore
#define CLOCK_SYNC_START_HOUR       3     // Prima sincronizare la ora 03:00 (apoi 15:00)
#define CLOCK_SLEW_DURATION_SEC     180   // Durata compensare slew: 3 minute (180 secunde)

// ----------------------------------------------------------------------------
// SENZORI EXTERNI WI-FI (UDP BROADCAST PROTOCOL METEO_V1)
// ----------------------------------------------------------------------------
#define EXT_SENSORS_UDP_PORT        5005  // Port UDP definit in FSD
#define EXT_SENSORS_OFFLINE_SEC     600   // 10 minute (3 cicluri x 180s) pana la declarare "OFF"
#define MAX_EXT_SENSOR_SLOTS        3     // 3 sloturi de senzori externi configurabili
#define MAX_DISCOVERED_SENSORS      8     // Cache senzori descoperiti in retea
#define DEBUG_EXT_SENSORS           0    // 1 = Loguri Serial mesaje UDP de la senzori




