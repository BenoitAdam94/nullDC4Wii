#pragma once

#include "types.h"

// Lifecycle
void rtc_Init();
void rtc_Reset(bool Manual);
void rtc_Term();

// Call at 64 Hz from the emulator main loop to advance the clock
void rtc_Tick();
