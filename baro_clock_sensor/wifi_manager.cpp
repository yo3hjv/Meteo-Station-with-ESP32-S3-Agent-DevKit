#include "wifi_manager.h"
#include "setup.h"
#include "sensor_ds18b20.h"
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define EEPROM_SIZE   512
#define EEPROM_MAGIC  0x4D530102 // Semnatura date "MS02"

struct WifiSlotData {
    char ssid[33];
    char pass[65];
};

struct SensorStorageData {
    uint32_t magic;
    WifiSlotData slots[MAX_WIFI_SLOTS];
};

static SensorStorageData storage;
static ESP8266WebServer server(80);
static bool isAPMode = false;
static String deviceMacID = "";

static void storageLoad() {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(0, storage);
    if (storage.magic != EEPROM_MAGIC) {
        memset(&storage, 0, sizeof(storage));
        storage.magic = EEPROM_MAGIC;
        EEPROM.put(0, storage);
        EEPROM.commit();
#if DEBUG_SENSOR_WIFI
        Serial.println("[EEPROM] Memorie EEPROM initializata cu valori default.");
#endif
    }
}

static void storageSave() {
    storage.magic = EEPROM_MAGIC;
    EEPROM.put(0, storage);
    EEPROM.commit();
#if DEBUG_SENSOR_WIFI
    Serial.println("[EEPROM] Date salvate cu succes in memoria EEPROM.");
#endif
}

static void generateMacID() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    deviceMacID = String(buf);
}

void wifiMgrInit() {
    storageLoad();
    generateMacID();
#if DEBUG_SENSOR_WIFI
    Serial.printf("[WIFI] Initializat Wi-Fi Manager (EEPROM). Device ID (MAC): %s\n", deviceMacID.c_str());
#endif
}

void wifiMgrGetSlot(int slot, String &ssid, String &pass) {
    if (slot >= 1 && slot <= MAX_WIFI_SLOTS) {
        ssid = String(storage.slots[slot - 1].ssid);
        pass = String(storage.slots[slot - 1].pass);
    } else {
        ssid = "";
        pass = "";
    }
}

void wifiMgrSaveSlot(int slot, const String &ssid, const String &pass) {
    if (slot >= 1 && slot <= MAX_WIFI_SLOTS) {
        memset(storage.slots[slot - 1].ssid, 0, sizeof(storage.slots[slot - 1].ssid));
        memset(storage.slots[slot - 1].pass, 0, sizeof(storage.slots[slot - 1].pass));
        strncpy(storage.slots[slot - 1].ssid, ssid.c_str(), sizeof(storage.slots[slot - 1].ssid) - 1);
        strncpy(storage.slots[slot - 1].pass, pass.c_str(), sizeof(storage.slots[slot - 1].pass) - 1);
        storageSave();
    }
}

void wifiMgrResetCredentials() {
    memset(&storage, 0, sizeof(storage));
    storage.magic = EEPROM_MAGIC;
    EEPROM.put(0, storage);
    EEPROM.commit();
#if DEBUG_SENSOR_WIFI
    Serial.println("[WIFI] Credentialele Wi-Fi au fost resetate din EEPROM.");
#endif
}

// ----------------------------------------------------------------------------
// PORTAL CAPTIV WEB PENTRU CONFIGURARE IN MOD SOFT-AP
// ----------------------------------------------------------------------------
static void handleRoot() {
    String s1, p1, s2, p2, s3, p3;
    wifiMgrGetSlot(1, s1, p1);
    wifiMgrGetSlot(2, s2, p2);
    wifiMgrGetSlot(3, s3, p3);

    float currentTemp = -999.0f;
    bool tempOk = sensorDs18b20Read(currentTemp);

    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>BaroClock Sensor - Setup</title>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;background:#1a1a1a;color:#eee;margin:0;padding:15px;}";
    html += ".card{background:#262626;padding:15px;border-radius:8px;margin-bottom:15px;max-width:480px;margin-left:auto;margin-right:auto;box-shadow:0 2px 5px rgba(0,0,0,0.5);}";
    html += "h2,h3{color:#00e5ff;margin-top:0;}";
    html += "label{display:block;margin:10px 0 3px;}";
    html += "input[type='text'],input[type='password']{width:100%;padding:8px;border-radius:4px;border:1px solid #555;background:#333;color:#fff;box-sizing:border-box;}";
    html += "input[type='submit'],.btn{background:#00bcd4;color:#000;border:none;padding:10px 15px;font-weight:bold;border-radius:4px;cursor:pointer;margin-top:12px;width:100%;box-sizing:border-box;}";
    html += ".badge{display:inline-block;padding:3px 8px;border-radius:4px;font-weight:bold;}";
    html += ".bg-green{background:#4caf50;color:#000;} .bg-red{background:#f44336;color:#fff;}";
    html += "</style></head><body>";

    html += "<div class='card'>";
    html += "<h2>BaroClock - External Sensor</h2>";
    html += "<p>Device ID (MAC): <b style='color:#ffeb3b;font-size:1.1em;'>" + deviceMacID + "</b></p>";
    html += "<p>DS18B20 ROM: <b>" + String(sensorDs18b20GetRomAddress()) + "</b></p>";
    html += "<p>Current Reading: ";
    if (tempOk) {
        html += "<span class='badge bg-green'>" + String(currentTemp, 2) + " &deg;C</span>";
    } else {
        html += "<span class='badge bg-red'>SENSOR ERROR / DISCONNECTED</span>";
    }
    html += "</p>";
    html += "</div>";

    html += "<div class='card'>";
    html += "<h3>Wi-Fi Networks Configuration</h3>";
    html += "<form action='/save' method='POST'>";

    html += "<p><b>Primary Network (Slot 1):</b></p>";
    html += "<label>SSID:</label><input type='text' name='s1' value='" + s1 + "' placeholder='Network name'>";
    html += "<label>Password:</label><input type='password' name='p1' value='" + p1 + "' placeholder='Password'>";

    html += "<hr style='border:0;border-top:1px solid #444;margin:15px 0;'>";
    html += "<p><b>Backup Network (Slot 2):</b></p>";
    html += "<label>SSID:</label><input type='text' name='s2' value='" + s2 + "' placeholder='Optional'>";
    html += "<label>Password:</label><input type='password' name='p2' value='" + p2 + "' placeholder='Optional'>";

    html += "<hr style='border:0;border-top:1px solid #444;margin:15px 0;'>";
    html += "<p><b>Backup Network (Slot 3):</b></p>";
    html += "<label>SSID:</label><input type='text' name='s3' value='" + s3 + "' placeholder='Optional'>";
    html += "<label>Password:</label><input type='password' name='p3' value='" + p3 + "' placeholder='Optional'>";

    html += "<input type='submit' value='Save & Reboot Sensor'>";
    html += "</form>";
    html += "</div>";

    html += "</body></html>";
    server.send(200, "text/html", html);
}

static void handleSave() {
    if (server.hasArg("s1")) wifiMgrSaveSlot(1, server.arg("s1"), server.arg("p1"));
    if (server.hasArg("s2")) wifiMgrSaveSlot(2, server.arg("s2"), server.arg("p2"));
    if (server.hasArg("s3")) wifiMgrSaveSlot(3, server.arg("s3"), server.arg("p3"));

#if DEBUG_SENSOR_WIFI
    Serial.println("[WIFI] Setari Wi-Fi salvate din Web Portal. Repornire dispozitiv in 1 secunda...");
#endif

    String resp = "<html><body style='background:#181818;color:#eee;font-family:sans-serif;padding:20px;text-align:center;'>";
    resp += "<h2>Settings Saved Successfully!</h2>";
    resp += "<p>Rebooting sensor and connecting to Wi-Fi...</p>";
    resp += "</body></html>";
    server.send(200, "text/html", resp);

    delay(1000);
    ESP.restart();
}

void wifiMgrStartAP() {
    isAPMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_IP_DEFAULT, AP_IP_DEFAULT, IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID_DEFAULT, AP_PASS_DEFAULT);

#if DEBUG_SENSOR_WIFI
    Serial.println("\n[WIFI] ========================================");
    Serial.printf("[WIFI] Mod SoftAP ACTIVAT: '%s'\n", AP_SSID_DEFAULT);
    Serial.printf("[WIFI] Parola AP: '%s'\n", AP_PASS_DEFAULT);
    Serial.printf("[WIFI] IP Portal: http://%s\n", WiFi.softAPIP().toString().c_str());
    Serial.println("[WIFI] ========================================\n");
#endif

    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.begin();
}

bool wifiMgrConnect() {
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);

    for (int slot = 1; slot <= MAX_WIFI_SLOTS; slot++) {
        String ssid, pass;
        wifiMgrGetSlot(slot, ssid, pass);

        if (ssid.length() == 0) continue;

#if DEBUG_SENSOR_WIFI
        Serial.printf("[WIFI] Incercare conectare la Slot %d: '%s'...\n", slot, ssid.c_str());
#endif
        WiFi.begin(ssid.c_str(), pass.c_str());

        unsigned long startMs = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startMs < (WIFI_CONNECT_TIMEOUT_SEC * 1000UL))) {
            delay(250);
#if DEBUG_SENSOR_WIFI
            Serial.print(".");
#endif
        }
#if DEBUG_SENSOR_WIFI
        Serial.println();
#endif

        if (WiFi.status() == WL_CONNECTED) {
            isAPMode = false;
#if DEBUG_SENSOR_WIFI
            Serial.printf("[WIFI] Conectat cu succes la '%s'!\n", ssid.c_str());
            Serial.printf("[WIFI] IP Atribuit: %s | Masca: %s | Broadcast: %s\n",
                          WiFi.localIP().toString().c_str(),
                          WiFi.subnetMask().toString().c_str(),
                          wifiMgrGetBroadcastIP().toString().c_str());
#endif
            return true;
        } else {
#if DEBUG_SENSOR_WIFI
            Serial.printf("[WIFI] Timeout conectare la Slot %d ('%s').\n", slot, ssid.c_str());
#endif
        }
    }

#if DEBUG_SENSOR_WIFI
    Serial.println("[WIFI] Nu s-a putut realiza conexiunea la niciun slot Wi-Fi configurat.");
#endif
    return false;
}

void wifiMgrHandleClient() {
    if (isAPMode) {
        server.handleClient();
    }
}

bool wifiMgrIsAP() {
    return isAPMode;
}

bool wifiMgrIsConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

String wifiMgrGetIP() {
    if (isAPMode) return WiFi.softAPIP().toString();
    return WiFi.localIP().toString();
}

IPAddress wifiMgrGetBroadcastIP() {
    if (!wifiMgrIsConnected()) {
        return IPAddress(255, 255, 255, 255);
    }
    IPAddress ip = WiFi.localIP();
    IPAddress subnet = WiFi.subnetMask();
    return IPAddress((uint32_t)ip | ~(uint32_t)subnet);
}

String wifiMgrGetMacID() {
    return deviceMacID;
}
