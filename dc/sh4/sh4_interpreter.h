#pragma once
#include "types.h"
#include "sh4_if.h"
#include "intc.h"

#undef sh4op
#define sh4op(str) void FASTCALL str(u32 op)
typedef void (FASTCALL OpCallFP)(u32 op);

// -------------------------------------------------------------------------
// Opcode classification flags
// -------------------------------------------------------------------------
enum OpcodeType
{
	Normal       = 0,    // No special handling required
	ReadsPC      = 1,    // PC must be valid/set before calling
	WritesPC     = 2,    // Opcode writes PC (branch)
	Delayslot    = 4,    // Has a delay-slot opcode (only valid with WritesPC)

	WritesSR     = 8,    // Writes SR  -> UpdateSR() must be called after
	WritesFPSCR  = 16,   // Writes FPSCR -> UpdateFPSCR() must be called after
	Invalid      = 128,  // Undefined/illegal opcode

	// Composite helpers
	ReadWritePC       = ReadsPC | WritesPC,
	WritesSRRWPC      = WritesSR | ReadsPC | WritesPC,

	// Branch variants (no delay slot)
	Branch_dir        = ReadWritePC,  // Direct   e.g. PC = Rn
	Branch_rel        = ReadWritePC,  // Relative e.g. PC += disp

	// Branch variants (with delay slot)
	Branch_dir_d      = Delayslot | Branch_dir,
	Branch_rel_d      = Delayslot | Branch_rel,
};

// -------------------------------------------------------------------------
// Interpreter interface
// -------------------------------------------------------------------------
void Sh4_int_Run();
void Sh4_int_Stop();
void Sh4_int_Step();
void Sh4_int_Skip();
void Sh4_int_Reset(bool Manual);
void Sh4_int_Init();
void Sh4_int_Term();
bool Sh4_int_IsCpuRunning();

// Exception helpers (note: legacy misspelling kept as an alias for ABI compat)
void FASTCALL sh4_int_RaiseException(u32 ExceptionCode, u32 VectorAddr);
static inline void FASTCALL sh4_int_RaiseExeption(u32 code, u32 vec)
{
	sh4_int_RaiseException(code, vec);
}

u32  Sh4_int_GetRegister(Sh4RegType reg);
void Sh4_int_SetRegister(Sh4RegType reg, u32 regdata);

// Delay-slot helpers (used by opcode implementations)
void ExecuteDelayslot();
void ExecuteDelayslot_RTE();


int FASTCALL UpdateSystem();


// -------------------------------------------------------------------------
// Miscellaneous
// -------------------------------------------------------------------------

// Coarse timer that ticks at ~13,216 kHz.
// Used to time block freeing; block promotion is no longer implemented.
extern u32 gcp_timer;

// SH4 execution constants (also useful to external code that schedules work)
static const s32 SH4_TIMESLICE_CYCLES = 448;
static const s32 SH4_CPU_RATIO        = 8;   // host cycles per SH4 cycle
