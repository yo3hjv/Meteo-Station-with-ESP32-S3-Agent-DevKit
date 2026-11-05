#include "udp_protocol.h"
#include "setup.h"
#include "wifi_manager.h"
#include <WiFiUdp.h>

static WiFiUDP udpClient;
static bool udpStarted = false;

void udpProtocolInit() {
    if (!udpStarted) {
        udpClient.begin(UDP_PORT);
        udpStarted = true;
#if DEBUG_UDP
        Serial.printf("[UDP] Socket UDP initializat pe portul local %d.\n", UDP_PORT);
#endif
    }
}

static bool sendPacketWithRepetitions(const String &jsonPayload) {
    if (!wifiMgrIsConnected()) {
#if DEBUG_UDP
        Serial.println("[UDP] Nu se poate trimite pachetul: Wi-Fi neconectat.");
#endif
        return false;
    }

    udpProtocolInit();

    IPAddress broadcastIp = wifiMgrGetBroadcastIP();
    bool allSuccess = true;

    for (int rep = 0; rep < UDP_TX_REPETITIONS; rep++) {
        udpClient.beginPacket(broadcastIp, UDP_PORT);
        size_t written = udpClient.write((const uint8_t*)jsonPayload.c_str(), jsonPayload.length());
        int res = udpClient.endPacket();

        if (written != jsonPayload.length() || res != 1) {
            allSuccess = false;
        }

        if (rep < UDP_TX_REPETITIONS - 1) {
            delay(UDP_TX_REPETITION_DELAY_MS);
        }
    }

#if DEBUG_UDP
    Serial.printf("[UDP TX -> %s:%d] (rep x%d): %s\n",
                  broadcastIp.toString().c_str(), UDP_PORT, UDP_TX_REPETITIONS, jsonPayload.c_str());
#endif

    return allSuccess;
}

bool udpProtocolSendAnnounce() {
    String id = wifiMgrGetMacID();
    String ip = wifiMgrGetIP();

    // Format JSON conform FSD SWR-009:
    // {"protocol":"METEO_V1","type":"announce","id":"...","ip":"...","capabilities":["temperature"]}
    String json = "{\"protocol\":\"";
    json += PROTOCOL_NAME;
    json += "\",\"type\":\"announce\",\"id\":\"";
    json += id;
    json += "\",\"ip\":\"";
    json += ip;
    json += "\",\"capabilities\":[\"temperature\"]}";

    return sendPacketWithRepetitions(json);
}

bool udpProtocolSendData(float temperatureC) {
    String id = wifiMgrGetMacID();
    String ip = wifiMgrGetIP();

    char tempBuf[16];
    snprintf(tempBuf, sizeof(tempBuf), "%.2f", temperatureC);

    // Format JSON conform FSD SWR-009:
    // {"protocol":"METEO_V1","type":"data","id":"...","ip":"...","temperature_c":18.75}
    String json = "{\"protocol\":\"";
    json += PROTOCOL_NAME;
    json += "\",\"type\":\"data\",\"id\":\"";
    json += id;
    json += "\",\"ip\":\"";
    json += ip;
    json += "\",\"temperature_c\":";
    json += tempBuf;
    json += "}";

    return sendPacketWithRepetitions(json);
}

bool udpProtocolSendError(const char *errorMsg) {
    String id = wifiMgrGetMacID();
    String ip = wifiMgrGetIP();

    String json = "{\"protocol\":\"";
    json += PROTOCOL_NAME;
    json += "\",\"type\":\"error\",\"id\":\"";
    json += id;
    json += "\",\"ip\":\"";
    json += ip;
    json += "\",\"error\":\"";
    json += errorMsg;
    json += "\"}";

    return sendPacketWithRepetitions(json);
}

