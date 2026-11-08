#include "temp_calib.h"
#include <Preferences.h>

const TempBandInfo TEMP_BANDS[TEMP_CALIB_BANDS_COUNT] = {
    { -20.0f, -10.0f, -15.0f, "-20 to -10 C" },
    { -10.0f,  -5.0f,  -7.5f, "-10 to -5 C"  },
    {  -5.0f,   5.0f,   0.0f, "-5 to +5 C"   },
    {   5.0f,  10.0f,   7.5f, "5 to 10 C"    },
    {  10.0f,  15.0f,  12.5f, "10 to 15 C"   },
    {  15.0f,  20.0f,  17.5f, "15 to 20 C"   },
    {  20.0f,  25.0f,  22.5f, "20 to 25 C"   },
    {  25.0f,  30.0f,  27.5f, "25 to 30 C"   },
    {  30.0f,  35.0f,  32.5f, "30 to 35 C"   },
    {  35.0f,  40.0f,  37.5f, "35 to 40 C"   },
    {  40.0f,  45.0f,  42.5f, "40 to 45 C"   }
};

static float bmeOffsets[TEMP_CALIB_BANDS_COUNT]  = BME_DEFAULT_CALIB_OFFSETS;
static float ext1Offsets[TEMP_CALIB_BANDS_COUNT] = EXT_1_DEFAULT_CALIB_OFFSETS;

static Preferences prefs;

void tempCalibInit() {
    prefs.begin("t_calib", false);
    size_t sz = sizeof(bmeOffsets);
    if (prefs.isKey("bme_offs")) {
        prefs.getBytes("bme_offs", bmeOffsets, sz);
    }
    if (prefs.isKey("ext1_offs")) {
        prefs.getBytes("ext1_offs", ext1Offsets, sz);
    }
    prefs.end();
}

static float calcInterpolatedOffset(float rawT, const float offsets[TEMP_CALIB_BANDS_COUNT]) {
    if (rawT <= TEMP_BANDS[0].nodeT) {
        return offsets[0];
    }
    if (rawT >= TEMP_BANDS[TEMP_CALIB_BANDS_COUNT - 1].nodeT) {
        return offsets[TEMP_CALIB_BANDS_COUNT - 1];
    }

    for (int i = 0; i < TEMP_CALIB_BANDS_COUNT - 1; i++) {
        if (rawT >= TEMP_BANDS[i].nodeT && rawT <= TEMP_BANDS[i + 1].nodeT) {
            float span = TEMP_BANDS[i + 1].nodeT - TEMP_BANDS[i].nodeT;
            if (span <= 0.0001f) return offsets[i];
            float frac = (rawT - TEMP_BANDS[i].nodeT) / span;
            return offsets[i] + frac * (offsets[i + 1] - offsets[i]);
        }
    }
    return 0.0f;
}

float tempCalibrateBme(float rawT) {
    return rawT + calcInterpolatedOffset(rawT, bmeOffsets);
}

float tempCalibrateExt1(float rawT) {
    return rawT + calcInterpolatedOffset(rawT, ext1Offsets);
}

float tempCalibGetInterpolatedOffsetBme(float rawT) {
    return calcInterpolatedOffset(rawT, bmeOffsets);
}

float tempCalibGetInterpolatedOffsetExt1(float rawT) {
    return calcInterpolatedOffset(rawT, ext1Offsets);
}

void tempCalibGetBmeOffsets(float outOffsets[TEMP_CALIB_BANDS_COUNT]) {
    memcpy(outOffsets, bmeOffsets, sizeof(bmeOffsets));
}

void tempCalibSetBmeOffsets(const float newOffsets[TEMP_CALIB_BANDS_COUNT]) {
    memcpy(bmeOffsets, newOffsets, sizeof(bmeOffsets));
    prefs.begin("t_calib", false);
    prefs.putBytes("bme_offs", bmeOffsets, sizeof(bmeOffsets));
    prefs.end();
}

void tempCalibGetExt1Offsets(float outOffsets[TEMP_CALIB_BANDS_COUNT]) {
    memcpy(outOffsets, ext1Offsets, sizeof(ext1Offsets));
}

void tempCalibSetExt1Offsets(const float newOffsets[TEMP_CALIB_BANDS_COUNT]) {
    memcpy(ext1Offsets, newOffsets, sizeof(ext1Offsets));
    prefs.begin("t_calib", false);
    prefs.putBytes("ext1_offs", ext1Offsets, sizeof(ext1Offsets));
    prefs.end();
}

