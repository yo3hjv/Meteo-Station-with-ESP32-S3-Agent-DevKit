/*
 * BaroClock Sensor
 * On ESP8266 Sonoff Basic HW R.1.0
 * by Adrian Florescu, yo3hjv
 * In Arduino IDE select: 
 * Generic ESP8266 Module sau Sonoff Basic
 */
#define FW_VERSION "0.0.1"   // Se modifica doar de USER; sursa unica de adevar pentru versiunile de firmware
// De aici incepe codul

#include <Arduino.h>
#include "setup.h"
#include "sensor_ds18b20.h"
#include "wifi_manager.h"
#include "udp_protocol.h"

static unsigned long lastTxMillis = 0;
static uint32_t dataCycleCount = 0;
static unsigned long buttonPressStart = 0;
static bool buttonIsPressed = false;
static unsigned long lastWifiReconnectTry = 0;

// Semnalizare vizuala LED (GPIO 13 este activ LOW pe Sonoff Basic)
static void ledSet(bool on) {
    digitalWrite(PIN_LED, on ? LOW : HIGH);
}

static void ledFlash(uint16_t durationMs) {
    ledSet(true);
    delay(durationMs);
    ledSet(false);
}

void setup() {
    Serial.begin(115200);
    delay(200);

#if DEBUG_SERIAL
    Serial.println("\n\n========================================");
    Serial.printf("BaroClock External Sensor v%s\n", FW_VERSION);
    Serial.println("Hardware: SONOFF BASIC V1.0 (ESP8266)");
    Serial.println("========================================");
#endif

    // Configurare pini hardware
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_RELAY, OUTPUT);
    ledSet(false);
    digitalWrite(PIN_RELAY, LOW); // Releul ramane decuplat

    // 1. Initializare memorie si identificator unic MAC
    wifiMgrInit();

    // 2. Verificare buton GPIO 0 apasat la boot (Hold 5s pentru AP Fortat)
    if (digitalRead(PIN_BUTTON) == LOW) {
#if DEBUG_SERIAL
        Serial.println("[BOOT] Butonul GPIO 0 a fost detectat apasat la pornire. Verificare 5 secunde...");
#endif
        unsigned long pressStart = millis();
        bool holdSuccess = true;
        while (millis() - pressStart < BUTTON_LONG_PRESS_MS) {
            ledFlash(100);
            delay(100);
            if (digitalRead(PIN_BUTTON) != LOW) {
                holdSuccess = false;
                break;
            }
        }

        if (holdSuccess) {
#if DEBUG_SERIAL
            Serial.println("[BOOT] Buton tinut apasat 5 secunde -> Pornire directa in Mod SoftAP!");
#endif
            wifiMgrStartAP();
            return;
        }
    }

    // 3. Initializare senzor DS18B20 pe GPIO 2
    sensorDs18b20Init();

    // 4. Conectare Wi-Fi la sloturile memorate in NVS
    bool wifiOk = wifiMgrConnect();

    if (wifiOk) {
        // Conexiune reusita: dublu flash LED de confirmare
        ledFlash(100);
        delay(80);
        ledFlash(100);

        // Transmitere mesaje initiale la pornire
        udpProtocolInit();
        udpProtocolSendAnnounce();

        float t = -999.0f;
        if (sensorDs18b20Read(t)) {
            udpProtocolSendData(t);
            dataCycleCount++;
        } else {
            udpProtocolSendError("sensor_disconnected");
        }
        lastTxMillis = millis();
    } else {
        // Daca nu se poate conecta la nicio retea, pornim SoftAP pentru configurare
#if DEBUG_SENSOR_WIFI
        Serial.println("[WIFI] Nicio retea disponibila -> Activare automata portal de configurare AP.");
#endif
        wifiMgrStartAP();
    }
}

void loop() {
    // 1. Monitorizare buton fizic GPIO 0 pentru intrare in mod AP (Apasare 5 secunde)
    if (digitalRead(PIN_BUTTON) == LOW) {
        if (!buttonIsPressed) {
            buttonIsPressed = true;
            buttonPressStart = millis();
        } else if (millis() - buttonPressStart >= BUTTON_LONG_PRESS_MS) {
            if (!wifiMgrIsAP()) {
#if DEBUG_SERIAL
                Serial.println("[BUTTON] Apasare de 5 secunde detectata in loop -> Trecere in Mod SoftAP!");
#endif
                ledFlash(500);
                wifiMgrStartAP();
            }
            buttonIsPressed = false;
        }
    } else {
        buttonIsPressed = false;
    }

    // 2. Comportament in Mod SoftAP (Portal Captiv Configurare)
    if (wifiMgrIsAP()) {
        wifiMgrHandleClient();

        // Clipire rapida LED (250ms) semnalizand modul configurare AP
        static unsigned long lastApBlink = 0;
        if (millis() - lastApBlink >= 250) {
            lastApBlink = millis();
            static bool apLedState = false;
            apLedState = !apLedState;
            ledSet(apLedState);
        }
        return;
    }

    // 3. Comportament in Mod Station (STA)
    if (!wifiMgrIsConnected()) {
        // Clipire lenta de avertizare conexiune pierduta
        static unsigned long lastDiscBlink = 0;
        if (millis() - lastDiscBlink >= 1500) {
            lastDiscBlink = millis();
            ledFlash(80);
        }

        // Incercare periodica de reconectare Wi-Fi la fiecare 30 secunde
        if (millis() - lastWifiReconnectTry >= 30000UL) {
            lastWifiReconnectTry = millis();
#if DEBUG_SENSOR_WIFI
            Serial.println("[WIFI] Reincercare asociere Wi-Fi...");
#endif
            if (wifiMgrConnect()) {
                udpProtocolSendAnnounce();
            }
        }
        return;
    }

    // 4. Ciclu periodic de masurare si transmisie UDP (Implicit la fiecare 180 secunde)
    if (millis() - lastTxMillis >= (SENSOR_TX_INTERVAL_SEC * 1000UL)) {
        lastTxMillis = millis();

        // 1 Announce la fiecare N Data conform FSD SWR-009
        if (dataCycleCount % ANNOUNCE_EVERY_N_DATA == 0) {
            udpProtocolSendAnnounce();
            delay(50);
        }

        // Masurare temperatura DS18B20
        float t = -999.0f;
        bool readOk = sensorDs18b20Read(t);

        if (readOk) {
            ledFlash(50); // Flash scurt semnalizare transmisie reusita
            udpProtocolSendData(t);
            dataCycleCount++;
        } else {
            // Semnalizare eroare: 3 clipiri scurte
            ledFlash(50); delay(50);
            ledFlash(50); delay(50);
            ledFlash(50);
            udpProtocolSendError("sensor_disconnected");
        }
    }
}
