#pragma once

#include "types.h"

// Timer Unit (TMU) - SH4 hardware countdown timers
// Three independent channels (0, 1, 2) used for OS scheduling,
// audio sync, and general game timing on the Sega Dreamcast.

void UpdateTMU(u32 Cycles);
void tmu_Init();
void tmu_Reset(bool Manual);
void tmu_Term();
