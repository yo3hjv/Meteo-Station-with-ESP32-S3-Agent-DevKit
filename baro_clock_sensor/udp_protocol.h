#pragma once
#include <Arduino.h>

void udpProtocolInit();
bool udpProtocolSendAnnounce();
bool udpProtocolSendData(float temperatureC);
bool udpProtocolSendError(const char *errorMsg);

