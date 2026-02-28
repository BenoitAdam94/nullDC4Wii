#pragma once

#include "types.h"

/**
 * SCI - Serial Communication Interface (SH4)
 *
 * NOTE: The SCI peripheral was not used by the Dreamcast.
 * These stubs exist to satisfy the emulator's standard
 * Init/Reset/Term lifecycle without performing any action.
 */

void sci_Init();
void sci_Reset(bool Manual);
void sci_Term();
