#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>

void wifiMgrInit();
bool wifiMgrConnect();
void wifiMgrStartAP();
void wifiMgrHandleClient();
bool wifiMgrIsAP();
bool wifiMgrIsConnected();

String wifiMgrGetIP();
IPAddress wifiMgrGetBroadcastIP();
String wifiMgrGetMacID();

void wifiMgrGetSlot(int slot, String &ssid, String &pass);
void wifiMgrSaveSlot(int slot, const String &ssid, const String &pass);
void wifiMgrResetCredentials();

