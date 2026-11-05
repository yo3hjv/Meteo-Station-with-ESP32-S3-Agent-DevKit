#pragma once
#include <Arduino.h>

void sensorDs18b20Init();
bool sensorDs18b20Read(float &outTemp);
bool sensorDs18b20IsConnected();
const char* sensorDs18b20GetRomAddress();

