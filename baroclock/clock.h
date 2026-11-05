#pragma once
#include <Arduino.h>
#include <time.h>
#include "setup.h"

// ============================================================================
// CLOCK ENGINE - MOTOR DE CEAS AUTONOM CU SINCRONIZARE SLEW (3 MINUTE)
// ============================================================================

void clockInit();
void clockUpdate();

// Obtinere timp atomic local si UTC din motorul intern
bool clockGetTime(struct tm &utcTm, struct tm &localTm);
time_t clockGetUtcEpoch();

// Stare sincronizare si Slew
bool clockIsSynchronized();
bool clockIsSlewing();
uint32_t clockGetSlewRemainingSec();
float clockGetLastOffsetMs();
const char* clockGetSyncSourceStr();
String clockGetLastSyncStatus();

// Fortare manuala de sincronizare (apelabila din Web UI sau comenzi)
bool clockTriggerSync(bool forceStep = false);

