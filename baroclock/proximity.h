#pragma once
#include <Arduino.h>
#include "setup.h"

void proximityInit();
void proximityUpdate();
uint16_t proximityGetRawValue();
bool proximityIsDetected();

