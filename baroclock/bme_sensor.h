#pragma once
#include <Arduino.h>

struct BmeData {
    bool ready;
    uint8_t chipId;            // 0x60 = BME280, 0x58 = BMP280
    float tempRawC;            // Raw sensor temperature in Celsius
    float tempC;               // Calibrated temperature in Celsius
    float tempF;               // Calibrated temperature in Fahrenheit
    float pressureRawMb;       // Raw sensor pressure in hPa/mb
    float pressureCalibratedMb;  // Calibrated with QNH offset
};

enum BaroTimeSpan : uint8_t {
    BARO_SPAN_10M = 0,
    BARO_SPAN_30M,
    BARO_SPAN_1H,
    BARO_SPAN_3H,
    BARO_SPAN_6H,
    BARO_SPAN_12H,
    BARO_SPAN_24H,
    BARO_SPAN_3D,
    BARO_SPAN_7D,
    BARO_SPAN_14D,
    BARO_SPAN_COUNT
};

struct BaroSample {
    uint32_t timestamp;  // Epoch secunde
    float    pressure;   // hPa calibrat
};

void bmeSensorInit();
void bmeSensorUpdate();
const BmeData& bmeGetData();

// Barograph functions
void baroGraphInit();
void baroGraphUpdate();
void baroGraphRedrawBox();
void baroGraphResetBaseline();
void baroGraphResetTimer();
void baroGraphDrawTimeAxis();
const float* baroGetPressHistory();

// Gesturi si control scala timp barograf
void baroGraphZoomIn();
void baroGraphZoomOut();
void baroGraphSetTimeSpan(uint8_t spanIdx);
uint8_t baroGraphGetTimeSpan();
uint32_t baroGraphGetSpanSeconds(uint8_t spanIdx);
const char* baroGraphGetSpanName(uint8_t spanIdx);
void baroGraphFlushToFlash();


