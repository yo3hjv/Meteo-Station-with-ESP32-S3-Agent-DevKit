/*
 * BaroClock 
 * On ESP32 Agent Dev Kit
 * by Adrian Florescu, yo3hjv
 * In Arduino IDE select: 
 * ESP32S3 Dev module cu OPI PSRAM activ, Flash 16 MB, Flash Mode QIO 80 MHz
 */
#define FW_VERSION "0.3.2"   // sursa unica de adevar pentru versiunile de firmware

extern const char* FIRMWARE_VERSION;
const char* FIRMWARE_VERSION = FW_VERSION;

#include <Arduino.h>
#include <Wire.h>
#include <time.h>

#include "setup.h"
#include "lcd_driver.h"
#include "wifi_mgr.h"
#include "gps_handler.h"
#include "bme_sensor.h"
#include "webpage.h"
#include "weather_logger.h"
#include "sound.h"
#include "proximity.h"
#include "clock.h"
#include "ext_sensors.h"
#include "gui.h"
#include "gns_info.h"
#include "temp_calib.h"


// Time state
static bool ntpConfigured = false;
static unsigned long lastUiUpdate = 0;
static unsigned long lastBmeUpdate = 0;

static void configureNtp() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    ntpConfigured = true;
#if DEBUG_PONSTEST
    Serial.println("[NTP] Configured NTP servers.");
#endif
}

void setup() {
    Serial.begin(115200);
    delay(400);
#if DEBUG_PONSTEST
    Serial.printf("\n\n========================================\n");
    Serial.printf("BaroClock v%s - ESP32 Agent Dev Kit\n", FW_VERSION);
    Serial.printf("========================================\n");
#endif

    // 1. Initialize internal I2C bus for AW9523 IO expander
    Wire.begin(INTERNAL_I2C_SDA, INTERNAL_I2C_SCL);
    delay(50);

    // 2. Initialize Preferences & check Boot GPIO46 for WiFi reset
    wifiMgrInit();
    tempCalibInit();

    // 3. Read rotation setting and initialize LCD
    bool rot180 = settingsGetRotate180();
    lcdInitHardware(rot180);
    lcdFillScreen(COLOR_BG);

    // 4. Boot check GPS: Incarca locatorul din Preferences si asculta UART 5 secunde pentru fix
    gpsHandlerInit();

    // 5. Connect to WiFi (Slots 1, 2, 3) or start AP & porneste sincronizarea NTP
    bool wifiOk = wifiMgrConnect();
    if (wifiOk) {
        configureNtp();
    }

    // 6. Start Web Server
    webpageInit();

    // 7. Initializare Receptie Senzori Externi UDP (METEO_V1 pe port 5005)
    extSensorsInit();

    // 8. Initializare Motor de Ceas (sub-ms RTC, step initial & periodic slew)
    clockInit();

    // 8. Initialize BME280 sensor
    bmeSensorInit();
    bmeSensorUpdate();

    // 9. Initializare Weather Logger (LittleFS si istoric corelat pe locatie)
    weatherLoggerInit();

    // 10. Initializare Barograph (Stratul 1 in PSRAM & restaurare din LittleFS)
    baroGraphInit();

    // 11. Initializare Sistem Audio Difuzor (ES8311 & I2S) si Senzor Proximitate (MK-TP2016)
    soundInit();
    proximityInit();

    // 12. Initializare Modul GUI (LVGL 9.4.0 & EEZ Studio)
    guiInit();

    // Initial sensor read and UI draw
    bmeSensorUpdate();
    guiUpdate();

#if DEBUG_PONSTEST
    Serial.println("[MAIN] Setup completed successfully.");
#endif
}

void loop() {
    // 1. Motor de ceas autonom (avans sub-milisecunda si calcul Slew lin)
    clockUpdate();

    uint8_t curPage = settingsGetDisplayPage();
    static uint8_t lastPage = 255;
    static unsigned long gnssPageEnterMs = 0;

    // Detectare comutare pagina (0 = Main LVGL, 1 = GNSS Info)
    if (curPage != lastPage) {
        lastPage = curPage;
        // Curatare completa a ecranului pe ambele treceri
        lcdFillScreen(COLOR_BG);

        if (curPage == 0) {
            // Trecere in Main (LVGL): re-desenare completa fara reziduuri GNSS
            guiInvalidate();
            guiUpdate();
            guiTick();
        } else {
            // Trecere in GNSS Info: reconstructie integrala fundal si elemente statice
            gnssPageEnterMs = millis();
            renderUiGpsDiag(true);
        }
    }

    // Revenire automata din ecranul GNSS Info dupa expirarea timerului (setabil in setup.h)
    if (curPage == 1 && (millis() - gnssPageEnterMs >= (uint32_t)GNSS_PAGE_TIMEOUT_SEC * 1000UL)) {
        settingsSetDisplayPage(0);
        curPage = 0;
    }

    // PRIORITATE 1: Smart Refresh Ceas & Secundar (Edge-Trigger instant pe milisecunda 0 a fiecarei secunde)
    if (curPage == 0) {
        guiClockTick();
        guiTick(); // Gestioneaza animatiile, flush-ul LVGL si touch FT6336
    }

    // Keep GPS parser fed
    gpsHandlerUpdate();

    // Receptie pachete UDP de la senzorii externi (METEO_V1 pe port 5005)
    extSensorsUpdate();

    // Handle incoming Web Server HTTP requests
    webpageHandleClient();

    // PRIORITATE 2: Esantionare senzori & Smart Refresh widget-uri secundare (la fiecare 1 secunda)
    if (millis() - lastBmeUpdate >= 1000) {
        lastBmeUpdate = millis();
        bmeSensorUpdate();
        if (curPage == 0) {
            guiUpdateSensors(); // Dirty-Check: doar ce s-a modificat
        }
    }

    // Update Barograph plot based on user-defined interval
    baroGraphUpdate();

    // Update Weather Logger & Forecast analysis based on forecast interval
    weatherLoggerUpdate();

    // Verificare senzor proximitate si declansare bip test difuzor
    proximityUpdate();

    // Verificare periodica alarme acustice (independent de ecranul afisat)
    guiCheckAlarms();

    // Refresh ecran secundar Diagnostic GNSS (la fiecare 250 ms doar pe Pagina 1, exclusiv zone dirty)
    if (curPage == 1 && (millis() - lastUiUpdate >= 250)) {
        lastUiUpdate = millis();
        renderUiGpsDiag(false); // Smart Refresh: doar ce s-a modificat
    }
}
