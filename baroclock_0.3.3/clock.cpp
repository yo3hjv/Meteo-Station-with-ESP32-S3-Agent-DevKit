#include "clock.h"
#include "setup.h"
#include "gps_handler.h"
#include "wifi_mgr.h"
#include <sys/time.h>

static bool isInitialized = false;
static bool isSynchronized = false;
static uint64_t baseUtcEpochMs = 0;
static unsigned long baseMillis = 0;

static bool isSlewing = false;
static int32_t slewTotalDeltaMs = 0;
static unsigned long slewStartMillis = 0;
static unsigned long slewDurationMs = CLOCK_SLEW_DURATION_SEC * 1000UL;

static String lastSyncSource = "None";
static String lastSyncTimestampStr = "None (pending)";
static float lastOffsetMs = 0.0f;
static int lastTriggerHour = -1;

void clockInit() {
    isInitialized = true;
    isSynchronized = false;
    isSlewing = false;
    baseUtcEpochMs = 0;
    baseMillis = millis();
}

static uint64_t getNowUtcEpochMs() {
    if (!isSynchronized) {
        time_t sTime;
        time(&sTime);
        if (sTime > 1672531199) {
            return (uint64_t)sTime * 1000ULL + (millis() % 1000);
        }
        return 0;
    }

    unsigned long curM = millis();
    unsigned long elapsed = curM - baseMillis;
    uint64_t curMs = baseUtcEpochMs + elapsed;

    if (isSlewing) {
        unsigned long slewElapsed = curM - slewStartMillis;
        if (slewElapsed >= slewDurationMs) {
            baseUtcEpochMs += slewTotalDeltaMs;
            isSlewing = false;
        } else {
            float progress = (float)slewElapsed / (float)slewDurationMs;
            int32_t currentAdj = (int32_t)roundf((float)slewTotalDeltaMs * progress);
            curMs += currentAdj;
        }
    }

    return curMs;
}

static bool getReferenceTime(uint64_t &outRefMs, const char* &outSource) {
    const GpsData &g = gpsGetData();
    if (g.present && g.fix && g.year >= 2023) {
        struct tm t;
        memset(&t, 0, sizeof(t));
        t.tm_year = g.year - 1900;
        t.tm_mon  = g.month - 1;
        t.tm_mday = g.day;
        t.tm_hour = g.hour;
        t.tm_min  = g.minute;
        t.tm_sec  = g.second;
        t.tm_isdst = -1;
        time_t utcSec = mktime(&t);
        if (utcSec > 1672531199) {
            outRefMs = (uint64_t)utcSec * 1000ULL;
            outSource = "GPS";
            return true;
        }
    }

    time_t nowSec;
    time(&nowSec);
    if (nowSec > 1672531199) {
        outRefMs = (uint64_t)nowSec * 1000ULL;
        outSource = "NTP";
        return true;
    }

    return false;
}

bool clockTriggerSync(bool forceStep) {
    uint64_t refMs = 0;
    const char *src = "None";
    if (!getReferenceTime(refMs, src)) {
        return false;
    }

    if (!isSynchronized || forceStep) {
        // Cold start / Step: ancorare directa
        baseUtcEpochMs = refMs;
        baseMillis = millis();
        isSynchronized = true;
        isSlewing = false;
        lastOffsetMs = 0.0f;
        lastSyncSource = src;

        struct timeval tv;
        tv.tv_sec = (time_t)(refMs / 1000ULL);
        tv.tv_usec = (suseconds_t)((refMs % 1000ULL) * 1000ULL);
        settimeofday(&tv, NULL);

        int32_t tzOffsetSec = (settingsGetTimezoneMin() + (settingsGetDst() ? 60 : 0)) * 60;
        time_t localSec = tv.tv_sec + tzOffsetSec;
        struct tm localTm;
        localtime_r(&localSec, &localTm);

        char buf[48];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d (%s, Boot)", localTm.tm_hour, localTm.tm_min, localTm.tm_sec, src);
        lastSyncTimestampStr = String(buf);
        return true;
    }

    // Sincronizare periodica cu Slew pe durata a 3 minute
    uint64_t currentEngineMs = getNowUtcEpochMs();
    int64_t deltaMs = (int64_t)refMs - (int64_t)currentEngineMs;

    // Daca diferenta este anormal de mare (> 30s), facem pas direct (Step)
    if (abs(deltaMs) > 30000LL) {
        return clockTriggerSync(true);
    }

    slewTotalDeltaMs = (int32_t)deltaMs;
    slewStartMillis = millis();
    slewDurationMs = CLOCK_SLEW_DURATION_SEC * 1000UL;
    isSlewing = true;
    lastOffsetMs = (float)deltaMs;
    lastSyncSource = src;

    time_t curSec = (time_t)(currentEngineMs / 1000ULL);
    int32_t tzOffsetSec = (settingsGetTimezoneMin() + (settingsGetDst() ? 60 : 0)) * 60;
    time_t localSec = curSec + tzOffsetSec;
    struct tm localTm;
    localtime_r(&localSec, &localTm);

    char buf[48];
    snprintf(buf, sizeof(buf), "%02d:%02d (%s, Slew %+.0fms)", 
             localTm.tm_hour, localTm.tm_min, src, (float)deltaMs);
    lastSyncTimestampStr = String(buf);

    return true;
}

void clockUpdate() {
    if (!isInitialized) return;

    // 1. Daca ceasul nu este inca sincronizat, incercam continuu sa prindem prima sincronizare
    if (!isSynchronized) {
        static unsigned long lastInitTry = 0;
        if (millis() - lastInitTry >= 1000) {
            lastInitTry = millis();
            clockTriggerSync(true);
        }
        return;
    }

    // 2. Mentinere motor de Slew activ
    if (isSlewing) {
        unsigned long slewElapsed = millis() - slewStartMillis;
        if (slewElapsed >= slewDurationMs) {
            baseUtcEpochMs += slewTotalDeltaMs;
            isSlewing = false;
        }
    }

    // 3. Verificare tranzitie cand GPS obtine 3D FIX valid (sursa primara peste NTP)
    static bool lastGpsFixState = false;
    const GpsData &g = gpsGetData();
    bool curGpsHasFix = (g.present && g.fix && g.year >= 2023);
    if (curGpsHasFix && (!lastGpsFixState || lastSyncSource != "GPS")) {
        lastGpsFixState = true;
        clockTriggerSync(false); // Sincronizare lina din GPS si actualizare sursa la "GPS"
    } else if (!curGpsHasFix) {
        lastGpsFixState = false;
    }

    // 4. Verificare orar sincronizare periodica (la 12 ore, incepand cu ora 03:00)
    struct tm utcTm, localTm;
    if (clockGetTime(utcTm, localTm)) {
        int h = localTm.tm_hour;
        bool isScheduledHour = ((h % CLOCK_SYNC_INTERVAL_HOURS) == (CLOCK_SYNC_START_HOUR % CLOCK_SYNC_INTERVAL_HOURS));

        if (isScheduledHour && localTm.tm_min == 0) {
            if (lastTriggerHour != h) {
                lastTriggerHour = h;
                clockTriggerSync(false); // Slew 3 minute!
            }
        } else if (localTm.tm_min > 5) {
            lastTriggerHour = -1; // Re-armare pentru urmatorul interval
        }
    }
}

bool clockGetTime(struct tm &utcTm, struct tm &localTm) {
    uint64_t epochMs = getNowUtcEpochMs();
    if (epochMs == 0) return false;

    time_t utcSec = (time_t)(epochMs / 1000ULL);
    gmtime_r(&utcSec, &utcTm);

    int32_t tzOffsetSec = (settingsGetTimezoneMin() + (settingsGetDst() ? 60 : 0)) * 60;
    time_t localSec = utcSec + tzOffsetSec;
    localtime_r(&localSec, &localTm);

    return true;
}

time_t clockGetUtcEpoch() {
    uint64_t ms = getNowUtcEpochMs();
    if (ms == 0) return 0;
    return (time_t)(ms / 1000ULL);
}

bool clockIsSynchronized() {
    return isSynchronized;
}

bool clockIsSlewing() {
    return isSlewing;
}

uint32_t clockGetSlewRemainingSec() {
    if (!isSlewing) return 0;
    unsigned long elapsed = millis() - slewStartMillis;
    if (elapsed >= slewDurationMs) return 0;
    return (uint32_t)((slewDurationMs - elapsed) / 1000UL);
}

float clockGetLastOffsetMs() {
    return lastOffsetMs;
}

const char* clockGetSyncSourceStr() {
    return lastSyncSource.c_str();
}

String clockGetLastSyncStatus() {
    if (!isSynchronized) return "Not Synchronized (Awaiting GPS/NTP)";
    if (isSlewing) {
        uint32_t rem = clockGetSlewRemainingSec();
        char buf[64];
        snprintf(buf, sizeof(buf), "Slew in progress (%us rem, %+.0fms)", rem, lastOffsetMs);
        return String(buf);
    }
    return lastSyncTimestampStr;
}

