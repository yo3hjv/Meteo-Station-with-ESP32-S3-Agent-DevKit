#include "sensor_ds18b20.h"
#include "setup.h"
#include <OneWire.h>
#include <DallasTemperature.h>

static OneWire oneWireBus(PIN_ONEWIRE);
static DallasTemperature dallasSensor(&oneWireBus);

static DeviceAddress sensorDeviceAddress;
static bool sensorDetected = false;
static char romAddressStr[17] = "0000000000000000";

static bool scanAndConfigure() {
    pinMode(PIN_ONEWIRE, INPUT_PULLUP);
    oneWireBus.reset_search();
    dallasSensor.begin();
    dallasSensor.setWaitForConversion(true);

    if (dallasSensor.getAddress(sensorDeviceAddress, 0)) {
        sensorDetected = true;
        dallasSensor.setResolution(sensorDeviceAddress, DS18B20_RESOLUTION);

        snprintf(romAddressStr, sizeof(romAddressStr),
                 "%02X%02X%02X%02X%02X%02X%02X%02X",
                 sensorDeviceAddress[0], sensorDeviceAddress[1], sensorDeviceAddress[2], sensorDeviceAddress[3],
                 sensorDeviceAddress[4], sensorDeviceAddress[5], sensorDeviceAddress[6], sensorDeviceAddress[7]);
#if DEBUG_SENSOR
        Serial.printf("[DS18B20] Senzor identificat pe 1-Wire! ROM: %s | Rezolutie: %d biti\n",
                      romAddressStr, DS18B20_RESOLUTION);
#endif
        return true;
    }

    sensorDetected = false;
    return false;
}

void sensorDs18b20Init() {
#if DEBUG_SENSOR
    Serial.printf("[DS18B20] Initializare 1-Wire pe GPIO %d...\n", PIN_ONEWIRE);
#endif
    if (!scanAndConfigure()) {
#if DEBUG_SENSOR
        Serial.println("[DS18B20] AVERTISMENT: Niciun senzor DS18B20 nu a raspuns la initializare!");
#endif
    }
}

bool sensorDs18b20Read(float &outTemp) {
    // Mecanism robust cu pana la 3 incercari impotriva zgomotului sau jitter-ului Wi-Fi
    for (int attempt = 1; attempt <= 3; attempt++) {
        if (!sensorDetected) {
            if (!scanAndConfigure()) {
                delay(100);
                continue;
            }
        }

        // Cerere conversie temperatura
        dallasSensor.requestTemperatures();
        float t = dallasSensor.getTempC(sensorDeviceAddress);

        // Fallback daca citirea prin adresa precisa a returnat -127
        if (t == DEVICE_DISCONNECTED_C || isnan(t) || isinf(t)) {
            t = dallasSensor.getTempCByIndex(0);
        }

        // Verificare daca valoarea este plauzibila si valida
        if (t != DEVICE_DISCONNECTED_C && !isnan(t) && !isinf(t) &&
            t >= TEMP_MIN_VALID && t <= TEMP_MAX_VALID) {
            outTemp = t;
#if DEBUG_SENSOR
            if (attempt > 1) {
                Serial.printf("[DS18B20] Masuratoare reusita la incercarea %d: %.2f C\n", attempt, outTemp);
            } else {
                Serial.printf("[DS18B20] Masuratoare valida: %.2f C\n", outTemp);
            }
#endif
            return true;
        }

#if DEBUG_SENSOR
        Serial.printf("[DS18B20] Avertisment: Citire invalida (%.2f C) la incercarea %d/3. Reset magistrala...\n", t, attempt);
#endif
        oneWireBus.reset();
        delay(120);
    }

    // Daca toate cele 3 incercari au esuat
    sensorDetected = false;
#if DEBUG_SENSOR
    Serial.println("[DS18B20] Eroare: Senzorul DS18B20 nu a raspuns valid dupa 3 incercari.");
#endif
    return false;
}

bool sensorDs18b20IsConnected() {
    return sensorDetected;
}

const char* sensorDs18b20GetRomAddress() {
    return romAddressStr;
}
