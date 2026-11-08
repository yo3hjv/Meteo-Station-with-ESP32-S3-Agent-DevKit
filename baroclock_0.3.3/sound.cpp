#include "sound.h"
#include "setup.h"
#include "wifi_mgr.h"
#include <Wire.h>
#include <driver/i2s.h>
#include <math.h>

#define I2S_SAMPLE_RATE 16000
static bool i2sInstalled = false;
static bool isBeeping = false;
static uint8_t currentVolume = BEEP_VOLUME_PERCENT;

static int8_t isPaActive = -1;

// Rutina pentru streaming de liniste (zeroes) prin I2S catre DAC-ul ES8311
static void playSilenceI2s(uint16_t durationMs) {
    if (!i2sInstalled || durationMs == 0) return;
    const int sampleRate = I2S_SAMPLE_RATE;
    int totalSamples = (sampleRate * durationMs) / 1000;
    const int chunkSize = 128;
    int16_t buffer[chunkSize * 2];
    memset(buffer, 0, sizeof(buffer));

    while (totalSamples > 0) {
        int toWrite = (totalSamples > chunkSize) ? chunkSize : totalSamples;
        size_t bytesWritten = 0;
        i2s_write(I2S_NUM_0, buffer, toWrite * 2 * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
        totalSamples -= toWrite;
    }
}

// AW9523 helper pentru controlul amplificatorului audio (PA_CTRL pe P1_5)
// Citeste mai intai starea curenta a portului pentru a nu altera LCD_RST sau Backlight-ul (identic cu display_gui.h)
void soundSetPa(bool enable) {
    if ((enable ? 1 : 0) == isPaActive) return;
    isPaActive = enable ? 1 : 0;

    Wire.beginTransmission(AW9523_ADDR);
    Wire.write(0x03); // P1_PORT_OUTPUT_STATE
    if (Wire.endTransmission(false) != 0) return;

    uint8_t out = LCD_RST_BIT;
    if (Wire.requestFrom((uint8_t)AW9523_ADDR, (uint8_t)1) >= 1) {
        out = Wire.read();
    }
    // Asigura ca LCD_RST_BIT (P1_6) ramane intotdeauna 1 (HIGH) pentru a nu reseta LCD-ul
    out |= LCD_RST_BIT;

    if (enable) {
        out |= AW9523_PA_CTRL_BIT;  // P1_5 HIGH -> pornire amplificator
    } else {
        out &= ~AW9523_PA_CTRL_BIT; // P1_5 LOW  -> oprire amplificator
    }

    Wire.beginTransmission(AW9523_ADDR);
    Wire.write(0x03);
    Wire.write(out);
    Wire.endTransmission();

    // Daca tocmai am pornit PA, transmitem liniste digitala activa timp de AUDIO_PA_SETTLING_MS
    // pentru stabilizarea polarizarii interne a etajului final inainte de generarea sunetului
    if (enable) {
        playSilenceI2s(AUDIO_PA_SETTLING_MS);
    }
}

static volatile bool alarmClockRunning = false;
static volatile bool stormAlertRunning = false;

// Rutina de baza pentru generare ton sinusoidal sincron cu rampe anti-pop (fade-in / fade-out)
static void playToneRaw(uint16_t freq, uint16_t durationMs, uint8_t volumePercent = 100) {
    if (!i2sInstalled || freq == 0 || durationMs == 0) return;
    soundSetPa(true);

    const int sampleRate = I2S_SAMPLE_RATE;
    const int totalSamples = (sampleRate * durationMs) / 1000;
    const int attackSamples = (sampleRate * AUDIO_RAMP_ATTACK_MS) / 1000;
    const int decaySamples = (sampleRate * AUDIO_RAMP_DECAY_MS) / 1000;
    const int chunkSize = 128;
    int16_t buffer[chunkSize * 2];

    float phase = 0.0f;
    float phaseInc = (2.0f * (float)M_PI * (float)freq) / (float)sampleRate;
    const int16_t maxAmplitude = (volumePercent >= 100) ? 32760 : (int16_t)((32760L * volumePercent) / 100);

    int sampleIndex = 0;
    while (sampleIndex < totalSamples) {
        int toWrite = (totalSamples - sampleIndex > chunkSize) ? chunkSize : (totalSamples - sampleIndex);
        for (int i = 0; i < toWrite; i++) {
            int currentIdx = sampleIndex + i;
            float gain = 1.0f;
            if (currentIdx < attackSamples && attackSamples > 0) {
                gain = (float)currentIdx / (float)attackSamples; // Rampa de atac fade-in
            } else if (currentIdx >= (totalSamples - decaySamples) && decaySamples > 0) {
                gain = (float)(totalSamples - 1 - currentIdx) / (float)decaySamples; // Rampa de stingere fade-out
            }
            if (gain < 0.0f) gain = 0.0f;
            if (gain > 1.0f) gain = 1.0f;

            int16_t sample = (int16_t)(sinf(phase) * (float)maxAmplitude * gain);
            phase += phaseInc;
            if (phase >= 2.0f * (float)M_PI) phase -= 2.0f * (float)M_PI;
            buffer[i * 2]     = sample;
            buffer[i * 2 + 1] = sample;
        }
        size_t bytesWritten = 0;
        i2s_write(I2S_NUM_0, buffer, toWrite * 2 * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
        sampleIndex += toWrite;
    }
}

static void playSilence(uint16_t pauseMs) {
    if (pauseMs == 0) return;
    playSilenceI2s(pauseMs);
}


// Scriere registru ES8311 pe magistrala I2C interna
static bool es8311WriteReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(ES8311_I2C_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return (Wire.endTransmission() == 0);
}

static uint8_t es8311ReadReg(uint8_t reg) {
    Wire.beginTransmission(ES8311_I2C_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)ES8311_I2C_ADDR, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0;
}

// Initializare codec audio ES8311 pentru redare DAC (secventa identica din Reveil-CYDGOLD / codec_es8311.h)
static bool es8311InitCodec() {
    // 1. Verificare prezenta cip
    Wire.beginTransmission(ES8311_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("[SOUND] ES8311 codec nu raspunde pe I2C (0x18)!");
        return false;
    }

    // 2. Secventa de initializare registri pentru mod DAC 16-bit
    es8311WriteReg(0x00, 0x1F);
    delay(20);
    es8311WriteReg(0x00, 0x00);
    es8311WriteReg(0x00, 0x80);
    es8311WriteReg(0x01, 0x3F);
    es8311WriteReg(0x02, 0x08);
    es8311WriteReg(0x03, 0x10);
    es8311WriteReg(0x04, 0x10);
    es8311WriteReg(0x05, 0x00);
    es8311WriteReg(0x06, 0x03);
    es8311WriteReg(0x07, 0x00);
    es8311WriteReg(0x08, 0xFF);

    uint8_t v = es8311ReadReg(0x00);
    es8311WriteReg(0x00, v & 0xBF);

    es8311WriteReg(0x09, 0x0C);
    es8311WriteReg(0x0A, 0x0C);
    es8311WriteReg(0x0D, 0x01);
    es8311WriteReg(0x0E, 0x02);
    es8311WriteReg(0x12, 0x00);
    es8311WriteReg(0x13, 0x10);
    es8311WriteReg(0x1C, 0x6A);
    es8311WriteReg(0x37, 0x08);
    es8311WriteReg(0x17, 0xA8);
    es8311WriteReg(0x14, 0x1A);

    // Setare volum hardware ES8311 (registrul 0x32: 0..255 exact ca in Reveil-CYDGOLD / codec_es8311.h)
    currentVolume = settingsGetAudioVolume();
    uint8_t volReg = (uint8_t)((currentVolume * 255) / 100);
    es8311WriteReg(0x32, volReg);

#if (DEBUG_SOUND || DEBUG_PONSTEST)
    Serial.printf("[SOUND] ES8311 codec initializat cu succes (Volum: %u%% / reg 0x32: 0x%02X).\n", currentVolume, volReg);
#endif
    return true;
}

void soundSetVolume(uint8_t volumePercent) {
    if (volumePercent > 100) volumePercent = 100;
    currentVolume = volumePercent;
    // Mapare identica cu Reveil-CYDGOLD (codec_es8311.h): 0..100% -> 0..255 (0xFF = max digital gain +32dB)
    uint8_t volReg = (uint8_t)((currentVolume * 255) / 100);
    es8311WriteReg(0x32, volReg);
#if DEBUG_SOUND
    Serial.printf("[SOUND] Volum hardware ES8311 actualizat: %u%% (reg 0x32 = 0x%02X)\n", currentVolume, volReg);
#endif
}

uint8_t soundGetVolume() {
    return currentVolume;
}

// Initializare interfata I2S pentru transmisie audio catre ES8311
static void initI2S() {
    if (i2sInstalled) return;

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true
    };

    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_MCLK_PIN,
        .bck_io_num = I2S_BCLK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_DOUT_PIN,
        .data_in_num = -1
    };

    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err == ESP_OK) {
        i2s_set_pin(I2S_NUM_0, &pin_config);
        i2sInstalled = true;
#if (DEBUG_SOUND || DEBUG_PONSTEST)
        Serial.println("[SOUND] Driver I2S instalat cu succes pe I2S0.");
#endif
    } else {
        Serial.printf("[SOUND] EROARE la instalare driver I2S: 0x%X\n", err);
    }
}

void soundInit() {
#if (DEBUG_SOUND || DEBUG_PONSTEST)
    Serial.println("[SOUND] Initializare sistem audio & difuzor...");
#endif
    // 1. Amplificatorul PA initial oprit (fara zgomot de fond la boot)
    soundSetPa(false);

    // 2. Initializare codec ES8311
    es8311InitCodec();

    // 3. Initializare interfata I2S pe ESP32-S3
    initI2S();
}

struct BeepParams {
    uint16_t freq;
    uint16_t duration;
    uint8_t volume;
};

static void beepTask(void *param) {
    BeepParams *p = (BeepParams*)param;
    uint16_t freq = p->freq;
    uint16_t durationMs = p->duration;
    uint8_t vol = p->volume;
    delete p;

    if (i2sInstalled) {
        soundSetVolume(vol);
        soundSetPa(true);
        playToneRaw(freq, durationMs);
        playSilence(AUDIO_POST_TONE_SILENCE_MS);
        soundSetPa(false);
    }

    isBeeping = false;
    vTaskDelete(NULL);
}

void soundPlayBeep(uint16_t freqHz, uint16_t durationMs) {
    if (isBeeping || alarmClockRunning || stormAlertRunning) return;
    isBeeping = true;

    uint8_t vol = settingsGetAudioVolume();
    BeepParams *p = new BeepParams{freqHz, durationMs, vol};
    xTaskCreate(beepTask, "beepTask", 3072, p, 1, NULL);
}

void soundPlayFeedbackClick() {
    if (!settingsGetFeedbackEnabled()) return;
    if (isBeeping || alarmClockRunning || stormAlertRunning) return;
    isBeeping = true;

    // Feedback tactil: frecventa, durata si volum configurabile din setup.h si Web UI
    uint16_t freq = settingsGetFeedbackFreq();
    uint16_t dur = settingsGetFeedbackDuration();
    uint8_t vol = settingsGetFeedbackVolume();
    BeepParams *p = new BeepParams{freq, dur, vol};
    xTaskCreate(beepTask, "clickTask", 3072, p, 1, NULL);
}

struct ChimeParams {
    uint16_t freq;
    uint16_t toneDuration;
    uint16_t pauseDuration;
    uint8_t volume;
};

static void chimeTask(void *param) {
    ChimeParams *p = (ChimeParams*)param;
    uint16_t freq = p->freq;
    uint16_t toneDur = p->toneDuration;
    uint16_t pauseDur = p->pauseDuration;
    uint8_t vol = p->volume;
    delete p;

    if (i2sInstalled) {
        soundSetVolume(vol);
        soundSetPa(true);
        // Semnal orar dublu tip ceas electronic de mana (Pip-Pip)
        playToneRaw(freq, toneDur);
        playSilence(pauseDur);
        playToneRaw(freq, toneDur);
        playSilence(AUDIO_POST_TONE_SILENCE_MS);
        soundSetPa(false);
    }

    isBeeping = false;
    vTaskDelete(NULL);
}

void soundPlayChime() {
    if (!settingsGetChimeEnabled() || isBeeping || alarmClockRunning || stormAlertRunning) return;
    isBeeping = true;

    uint16_t f = settingsGetChimeFreq();
    uint16_t d = settingsGetChimeDuration();
    uint16_t pause = settingsGetChimePause();
    uint8_t v = settingsGetChimeVolume();
    ChimeParams *p = new ChimeParams{f, d, pause, v};
    xTaskCreate(chimeTask, "chimeTask", 3072, p, 1, NULL);
}

// ----------------------------------------------------------------------------
// Alarma Ceas Desteptator (5 bipuri rapide in succesiune + pauza ciclica)
// ----------------------------------------------------------------------------
static void alarmClockTask(void *param) {
    uint16_t f = settingsGetAlarmFreq();
    uint8_t v = settingsGetAlarmVolume();
    unsigned long startTime = millis();

    soundSetVolume(v);
#if DEBUG_SOUND
    Serial.printf("[ALARM] Pornita alarma ceas desteptator (Volum hardware ES8311: %u%%)!\n", v);
#endif

    while (alarmClockRunning && (millis() - startTime < (uint32_t)ALARM_AUTO_STOP_SEC * 1000)) {
        // Salva de 5 bipuri rapide
        for (int i = 0; i < 5; i++) {
            if (!alarmClockRunning) break;
            playToneRaw(f, ALARM_BEEP_DURATION_MS);
            playSilence(ALARM_BEEP_PAUSE_MS);
        }
        // Pauza mai lunga intre cicluri
        if (alarmClockRunning) {
            playSilence(ALARM_CYCLE_PAUSE_MS);
        }
    }

    playSilence(AUDIO_POST_TONE_SILENCE_MS);
    soundSetPa(false);
    alarmClockRunning = false;
#if DEBUG_SOUND
    Serial.println("[ALARM] Alarma ceas oprita.");
#endif
    vTaskDelete(NULL);
}

void soundStartAlarmClock() {
    if (alarmClockRunning) return;
    alarmClockRunning = true;
    xTaskCreate(alarmClockTask, "almClkTask", 3072, NULL, 1, NULL);
}

void soundStopAlarmClock() {
    alarmClockRunning = false;
    soundSetPa(false);
}

bool soundIsAlarmClockRinging() {
    return alarmClockRunning;
}

// ----------------------------------------------------------------------------
// Alarma Deteriorare Vreme (3 bipuri emise o singura data)
// ----------------------------------------------------------------------------
static void wtrDetTask(void *param) {
    uint16_t f = settingsGetWtrDetFreq();
    uint8_t v = settingsGetWtrDetVolume();

    soundSetVolume(v);

    for (int i = 0; i < 3; i++) {
        playToneRaw(f, WTR_DET_BEEP_DURATION_MS);
        playSilence(80);
    }
    playSilence(AUDIO_POST_TONE_SILENCE_MS);
    soundSetPa(false);
    vTaskDelete(NULL);
}

void soundPlayWeatherDeterioration() {
    if (!settingsGetWtrDetAlertEnabled() || alarmClockRunning || stormAlertRunning) return;
    xTaskCreate(wtrDetTask, "wtrDetTask", 3072, NULL, 1, NULL);
}

// ----------------------------------------------------------------------------
// Alarma Furtuna (Cod Morse S.O.S: ... --- ..., timp de 1 minut sau pana la anulare)
// ----------------------------------------------------------------------------
static void stormAlertTask(void *param) {
    uint16_t f = settingsGetStormFreq();
    uint8_t v = settingsGetStormVolume();
    unsigned long startTime = millis();

    soundSetVolume(v);
#if DEBUG_SOUND
    Serial.printf("[ALARM] Pornita alerta sonora furtuna (S.O.S Morse, Volum hardware ES8311: %u%%)!\n", v);
#endif

    while (stormAlertRunning && (millis() - startTime < (uint32_t)STORM_TIMEOUT_SEC * 1000)) {
        // S: 3 puncte (...)
        for (int i = 0; i < 3 && stormAlertRunning; i++) {
            playToneRaw(f, STORM_DOT_DURATION_MS);
            playSilence(STORM_DOT_DURATION_MS);
        }
        playSilence(STORM_DASH_DURATION_MS); // pauza litera

        // O: 3 linii (---)
        for (int i = 0; i < 3 && stormAlertRunning; i++) {
            playToneRaw(f, STORM_DASH_DURATION_MS);
            playSilence(STORM_DOT_DURATION_MS);
        }
        playSilence(STORM_DASH_DURATION_MS); // pauza litera

        // S: 3 puncte (...)
        for (int i = 0; i < 3 && stormAlertRunning; i++) {
            playToneRaw(f, STORM_DOT_DURATION_MS);
            playSilence(STORM_DOT_DURATION_MS);
        }
        playSilence(800); // pauza intre cuvinte
    }

    playSilence(AUDIO_POST_TONE_SILENCE_MS);
    soundSetPa(false);
    stormAlertRunning = false;
#if DEBUG_SOUND
    Serial.println("[ALARM] Alerta furtuna oprita.");
#endif
    vTaskDelete(NULL);
}

void soundStartStormAlert() {
    if (!settingsGetStormAlertEnabled() || stormAlertRunning) return;
    stormAlertRunning = true;
    xTaskCreate(stormAlertTask, "stormTask", 3072, NULL, 1, NULL);
}

void soundStopStormAlert() {
    stormAlertRunning = false;
    soundSetPa(false);
}

bool soundIsStormAlertRinging() {
    return stormAlertRunning;
}

void soundStopAllAlarms() {
    soundStopAlarmClock();
    soundStopStormAlert();
}


