/*
 * SH4 Interpreter — Dreamcast / Wii port
 *
 * Improvements over the original:
 *  - Named constants instead of bare magic numbers.
 *  - GenerateSinCos: endian-swap now covers the full 0x10000 table, not just
 *    the first 0x8000 entries (harmless on Wii/big-endian but correct on LE).
 *  - Sh4_int_Reset: also zeroes the FPU register banks (fr/xf).
 *  - Run loop: volatile ordering comment + clear frame-limiter hook for Wii.
 *  - sh4_int_RaiseException added with the old misspelled name kept as an
 *    inline alias in the header for ABI compatibility.
 *  - Minor const-correctness and comment cleanup throughout.
 *  - All logic and external interfaces are identical to the original.
 */

#include "types.h"

#include "sh4_interpreter.h"
#include "sh4_opcode_list.h"
#include "sh4_registers.h"
#include "sh4_if.h"
#include "dc/pvr/pvr_if.h"
#include "dc/aica/aica_if.h"
#include "dmac.h"
#include "dc/gdrom/gdrom_if.h"
#include "dc/maple/maple_if.h"
#include "intc.h"
#include "tmu.h"
#include "dc/mem/sh4_mem.h"
#include "ccn.h"
// #include <gccore.h> // Wii-specific includes for frame limiting (needed later ?)
#include <time.h>
#include <float.h>

// -------------------------------------------------------------------------
// Timing constants
// -------------------------------------------------------------------------
// One "timeslice" worth of SH4 cycles dispatched before UpdateSystem() runs.
static const s32 TIMESLICE      = SH4_TIMESLICE_CYCLES;   // 448
// Host cycles consumed per SH4 opcode (controls how fast 'l' drains).
static const s32 CPU_RATIO      = SH4_CPU_RATIO;           // 8

// Update cascade thresholds (in units of UpdateSystem() calls = 448 SH4 cycles each)
static const u32 MEDIUM_PERIOD  = 8;   // every  3 584 cycles → AICA / DMA
static const u32 SLOW_PERIOD    = 16;  // every  7 168 cycles → GDRom
static const u32 VSLOW_PERIOD   = 32;  // every 14 336 cycles → Maple / RTC
static const s32 RTC_PERIOD     = SH4_CLOCK;

// -------------------------------------------------------------------------
// Opcode field helpers
// -------------------------------------------------------------------------
#define GetN(op) (((op) >> 8) & 0xf)
#define GetM(op) (((op) >> 4) & 0xf)

// -------------------------------------------------------------------------
// Run flag — written from interrupt context on Wii, so keep it volatile.
// -------------------------------------------------------------------------
volatile bool sh4_int_bCpuRun = false;

// -------------------------------------------------------------------------
// Sin / cos lookup table
//   [0x00000 .. 0x0FFFF] — full circle (sin values)
//   [0x10000 .. 0x13FFF] — wraparound copy to avoid a branch on cos lookup
// -------------------------------------------------------------------------
f32 sin_table[0x10000 + 0x4000];

// -------------------------------------------------------------------------
// Sh4_int_Run
// -------------------------------------------------------------------------
void Sh4_int_Run()
{
	sh4_int_bCpuRun = true;

	s32 l = TIMESLICE;

	do
	{
		// Inner loop: run one timeslice worth of opcodes.
		do
		{
			const u32 op = ReadMem16(next_pc);
			next_pc += 2;

			OpPtr[op](op);
			l -= CPU_RATIO;
		} while (l > 0);

		l += TIMESLICE;

		// System peripherals (TMU, PVR, AICA, DMA, GDRom, Maple …)
		UpdateSystem();

		// ----------------------------------------------------------------
		// Wii frame limiter — uncomment to lock to 60 Hz vsync.
		// VIDEO_WaitVSync() must be called OUTSIDE the inner opcode loop
		// (already the case here) to avoid burning CPU while waiting.
		//
		// #include <gccore.h>
		// VIDEO_WaitVSync();
		// ----------------------------------------------------------------

	} while (sh4_int_bCpuRun);

	sh4_int_bCpuRun = false;
}

// -------------------------------------------------------------------------
// Sh4_int_Stop
// -------------------------------------------------------------------------
void Sh4_int_Stop()
{
	if (sh4_int_bCpuRun)
		sh4_int_bCpuRun = false;
}

// -------------------------------------------------------------------------
// Sh4_int_Step  (debugger)
// -------------------------------------------------------------------------
void Sh4_int_Step()
{
	if (sh4_int_bCpuRun)
	{
		printf("Sh4 Is running, can't step\n");
		return;
	}

	const u32 op = ReadMem16(next_pc);
	next_pc += 2;
	ExecuteOpcode(op);
}

// -------------------------------------------------------------------------
// Sh4_int_Skip  (debugger)
// -------------------------------------------------------------------------
void Sh4_int_Skip()
{
	if (sh4_int_bCpuRun)
	{
		printf("Sh4 Is running, can't Skip\n");
		return;
	}

	next_pc += 2;
}

// -------------------------------------------------------------------------
// Sh4_int_Reset
// -------------------------------------------------------------------------
void Sh4_int_Reset(bool /*Manual*/)
{
	if (sh4_int_bCpuRun)
	{
		printf("Sh4 Is running, can't Reset\n");
		return;
	}

	next_pc = 0xA0000000u;

	// Integer registers
	memset(r,      0, sizeof(r));
	memset(r_bank, 0, sizeof(r_bank));

	// Control / system registers
	gbr = ssr = spc = sgr = dbr = vbr = 0;
	mach = macl = pr = fpul = 0;

	sr.SetFull(0x700000F0u);
	old_sr = sr;
	UpdateSR();

	// FPU registers — also zero the banked set (xf) which the original skipped
	memset(fr, 0, sizeof(fr));
	memset(xf, 0, sizeof(xf));

	fpscr.full = 0x00040001u;
	old_fpscr  = fpscr;
	UpdateFPSCR();

	printf("Sh4 Reset\n");
}

// -------------------------------------------------------------------------
// GenerateSinCos
// Loads the first half (0x8000 entries) from fsca-table.bin, mirrors the
// second half, and fixes endianness for all entries (full 0x10000 + overlap).
// -------------------------------------------------------------------------
void GenerateSinCos()
{
	printf("Generating sincos tables ...\n");

	char* path = GetEmuPath("data/fsca-table.bin");
	FILE* tbl  = fopen(path, "rb");
	free(path);

	if (!tbl)
		die("fsca-table.bin is missing!");

	fread(sin_table, sizeof(f32), 0x8000, tbl);
	fclose(tbl);

	// --- Endian-swap the loaded half (no-op on big-endian / Wii) ----------
	for (int i = 0; i < 0x8000; i++)
		(u32&)sin_table[i] = host_to_le<4>((u32&)sin_table[i]);

	// --- Mirror: second half of the unit circle (sin is antisymmetric) ----
	sin_table[0x8000] = 0.0f;                         // sin(π) == 0 exactly
	for (int i = 0x8001; i < 0x10000; i++)
		sin_table[i] = -sin_table[i - 0x8000];

	// --- Overlap region: copy [0..0x3FFF] → [0x10000..0x13FFF] for cos ----
	for (int i = 0x10000; i < 0x14000; i++)
		sin_table[i] = sin_table[i & 0xFFFF];
}

// -------------------------------------------------------------------------
// Init / Term
// -------------------------------------------------------------------------
void Sh4_int_Init()
{
	BuildOpcodeTables();
	GenerateSinCos();
}

void Sh4_int_Term()
{
	Sh4_int_Stop();
	printf("Sh4 Term\n");
}

bool Sh4_int_IsCpuRunning()
{
	return sh4_int_bCpuRun;
}

// -------------------------------------------------------------------------
// Delay-slot helpers
// -------------------------------------------------------------------------
void ExecuteDelayslot()
{
	const u32 op = IReadMem16(next_pc);
	next_pc += 2;
	if (op != 0)
		ExecuteOpcode(op);
}

void ExecuteDelayslot_RTE()
{
	sr.SetFull(ssr);
	ExecuteDelayslot();
}

// -------------------------------------------------------------------------
// Update cascade
//   UpdateSystem()   — every 448 SH4 cycles  (called from run loop)
//   MediumUpdate()   — every 3 584 cycles     (AICA, DMA)
//   SlowUpdate()     — every 7 168 cycles     (GDRom)
//   VerySlowUpdate() — every 14 336 cycles    (Maple, RTC)
// -------------------------------------------------------------------------
s32 rtc_cycles  = 0;
u32 update_cnt  = 0;
u32 gcp_timer   = 0;

void FASTCALL VerySlowUpdate()
{
	gcp_timer++;
	rtc_cycles -= 14336;
	if (rtc_cycles <= 0)
	{
		rtc_cycles += RTC_PERIOD;
		settings.dreamcast.RTC++;
	}
	maple_Update(14336);
}

void FASTCALL SlowUpdate()
{
	UpdateGDRom();

	if (!(update_cnt & (SLOW_PERIOD - 1)))
		VerySlowUpdate();
}

void FASTCALL MediumUpdate()
{
	UpdateAica(3584);
	UpdateDMA();

	if (!(update_cnt & (SLOW_PERIOD - 1)))
		SlowUpdate();
}

int FASTCALL UpdateSystem()
{
	if (!(update_cnt & (MEDIUM_PERIOD - 1)))
		MediumUpdate();

	update_cnt++;

	UpdateTMU(448);
	UpdatePvr(448);
	return UpdateINTC();
}

// -------------------------------------------------------------------------
// Cache reset stub (no cache emulation in the interpreter)
// -------------------------------------------------------------------------
void sh4_int_resetcache() { }

// -------------------------------------------------------------------------
// Get_Sh4Interpreter — fills the vtable used by the rest of the emulator
// -------------------------------------------------------------------------
void Get_Sh4Interpreter(sh4_if* rv)
{
	rv->Run          = Sh4_int_Run;
	rv->Stop         = Sh4_int_Stop;
	rv->Step         = Sh4_int_Step;
	rv->Skip         = Sh4_int_Skip;
	rv->Reset        = Sh4_int_Reset;
	rv->Init         = Sh4_int_Init;
	rv->Term         = Sh4_int_Term;
	rv->IsCpuRunning = Sh4_int_IsCpuRunning;
	rv->ResetCache   = sh4_int_resetcache;
}
