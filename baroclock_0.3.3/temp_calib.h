#pragma once
#include <Arduino.h>
#include "setup.h"

struct TempBandInfo {
    float minT;
    float maxT;
    float nodeT;
    const char* label;
};

extern const TempBandInfo TEMP_BANDS[TEMP_CALIB_BANDS_COUNT];

void tempCalibInit();

// Corectii multi-punct interpolate
float tempCalibrateBme(float rawT);
float tempCalibrateExt1(float rawT);

// Obtinere offset interpolat curent pentru o temperatura data
float tempCalibGetInterpolatedOffsetBme(float rawT);
float tempCalibGetInterpolatedOffsetExt1(float rawT);

// Citire si scriere tabele de offset-uri
void tempCalibGetBmeOffsets(float outOffsets[TEMP_CALIB_BANDS_COUNT]);
void tempCalibSetBmeOffsets(const float newOffsets[TEMP_CALIB_BANDS_COUNT]);

void tempCalibGetExt1Offsets(float outOffsets[TEMP_CALIB_BANDS_COUNT]);
void tempCalibSetExt1Offsets(const float newOffsets[TEMP_CALIB_BANDS_COUNT]);

