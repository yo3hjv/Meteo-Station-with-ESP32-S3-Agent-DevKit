#pragma once
#include <Arduino.h>

// ============================================================================
// CONFIGURARE HARDWARE SI PARAMETRI - BAROCLOCK EXTERNAL SENSOR (SONOFF BASIC)
// ============================================================================

// 1. PINOUT HARDWARE SONOFF BASIC V1.0 (ESP8266)
#define PIN_ONEWIRE                  2   // GPIO 2: Magistrala 1-Wire pentru DS18B20 (necesita rezistor pull-up 4.7k la 3V3)
#define PIN_BUTTON                   0   // GPIO 0: Butonul integrat Sonoff (activ LOW)
#define PIN_LED                     13   // GPIO 13: LED-ul verde integrat Sonoff (activ LOW)
#define PIN_RELAY                   12   // GPIO 12: Releu integrat Sonoff (0 = Decuplat)

// 2. PARAMETRI SENZOR TEMPERATURA DS18B20
#define DS18B20_RESOLUTION          11   // Rezolutie convertor: 9..12 biti (11 biti = ~375ms conversie, precizie 0.125 C)
#define TEMP_MIN_VALID             -45.0f // Limita minima plauzibila pentru exterior (C)
#define TEMP_MAX_VALID              70.0f // Limita maxima plauzibila pentru exterior (C)

// 3. PARAMETRI RETEA WI-FI & PORTAL CAPTIV AP
#define AP_SSID_DEFAULT             "METEO_SETUP"
#define AP_PASS_DEFAULT             "12345678"
#define AP_IP_DEFAULT               IPAddress(192, 168, 4, 1)
#define WIFI_CONNECT_TIMEOUT_SEC    15   // Secunde maxime pentru incercare conectare la un slot Wi-Fi
#define MAX_WIFI_SLOTS              3    // 3 retele Wi-Fi salvate persistent (Slot 1, Slot 2, Slot 3)

// 4. PROTOCOL UDP BROADCAST (PORT 5005)
#define UDP_PORT                    5005 // Port UDP specificat in FSD
#define SENSOR_TX_INTERVAL_SEC      10  // Interval normal de masurare si transmisie (secunde)
#define UDP_TX_REPETITIONS          2    // Numar de transmisii per cadru (redundanta impotriva pierderilor UDP)
#define UDP_TX_REPETITION_DELAY_MS  50   // Pauza intre retransmisiile din acelasi cadru (ms)
#define ANNOUNCE_EVERY_N_DATA       5    // 1 mesaj ANNOUNCE la fiecare N mesaje DATA (plus 1 la boot)
#define PROTOCOL_NAME               "METEO_V1"

// 5. GESTIUNE BUTON & TIMPI OPERATIONALI
#define BUTTON_LONG_PRESS_MS        5000 // Apasare continua 5 secunde pe butonul GPIO 0 pentru intrare in mod AP

// 6. MACRO-URI DEBUG SERIAL (GRUPATE PE CATEGORII CONFORM FSD)
#define DEBUG_SERIAL                1    // 1 = Activare loguri seriale, 0 = Silent
#define DEBUG_SENSOR_WIFI           1    // 1 = Debug detaliat Wi-Fi (AP, STA, IP, Scanare)
#define DEBUG_SENSOR                1    // 1 = Debug senzor DS18B20 (adresa ROM, temperatura, erori)
#define DEBUG_UDP                   1    // 1 = Debug mesaje UDP (payload JSON, broadcast IP, stare trimitere)

