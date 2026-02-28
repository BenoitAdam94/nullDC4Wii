/**
 * sci.cpp - Serial Communication Interface (SH4)
 *
 * The SCI peripheral was not used on the Dreamcast. All functions
 * are intentional no-ops to satisfy the emulator lifecycle.
 *
 * SCI vs SCIF: The SH4 has two serial interfaces:
 *   - SCI  (Serial Communication Interface)     -- unused on DC
 *   - SCIF (Serial Communication Interface FIFO) -- used for debugging
 */

#include "types.h"
#include "dc/mem/sh4_internal_reg.h"
#include "sci.h"

// Suppress unused parameter warnings for intentional no-op stubs
#ifdef _MSC_VER
#  define SCI_UNUSED(x) (void)(x)
#else
#  define SCI_UNUSED(x) (void)(x)
#endif

void sci_Init()
{
	// SCI was not used on the Dreamcast -- nothing to initialize
}

void sci_Reset(bool Manual)
{
	// SCI was not used on the Dreamcast -- nothing to reset
	SCI_UNUSED(Manual);
}

void sci_Term()
{
	// SCI was not used on the Dreamcast -- nothing to clean up
}
