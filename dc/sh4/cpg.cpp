#include "types.h"
#include "dc/mem/sh4_internal_reg.h"
#include "cpg.h"

u16 CPG_FRQCR;
u8  CPG_STBCR;
u16 CPG_WTCNT;
u16 CPG_WTCSR;
u8  CPG_STBCR2;

// Helper: index into the CPG register array from a full address
#define CPG_REG(addr) CPG[((addr) & 0xFF) >> 2]

// Helper: set up a simple data-backed register with no custom read/write handlers
static inline void cpg_reg_init_16(u32 addr, u16* data)
{
    CPG_REG(addr).flags         = REG_16BIT_READWRITE | REG_READ_DATA | REG_WRITE_DATA;
    CPG_REG(addr).readFunction  = 0;
    CPG_REG(addr).writeFunction = 0;
    CPG_REG(addr).data16        = data;
}

static inline void cpg_reg_init_8(u32 addr, u8* data)
{
    CPG_REG(addr).flags         = REG_8BIT_READWRITE | REG_READ_DATA | REG_WRITE_DATA;
    CPG_REG(addr).readFunction  = 0;
    CPG_REG(addr).writeFunction = 0;
    CPG_REG(addr).data8         = data;
}

void cpg_Init()
{
    // FRQCR  H'FFC00000 - 16-bit, Clock Frequency Control
    cpg_reg_init_16(CPG_FRQCR_addr,  &CPG_FRQCR);

    // STBCR  H'FFC00004 - 8-bit,  Standby Control (module clock gating)
    cpg_reg_init_8 (CPG_STBCR_addr,  &CPG_STBCR);

    // WTCNT  H'FFC00008 - 16-bit, Watchdog Timer Counter
    cpg_reg_init_16(CPG_WTCNT_addr,  &CPG_WTCNT);

    // WTCSR  H'FFC0000C - 16-bit, Watchdog Timer Control/Status
    cpg_reg_init_16(CPG_WTCSR_addr,  &CPG_WTCSR);

    // STBCR2 H'FFC00010 - 8-bit,  Standby Control 2
    cpg_reg_init_8 (CPG_STBCR2_addr, &CPG_STBCR2);
}

void cpg_Reset(bool Manual)
{
    // SH-4 manual: on power-on reset FRQCR reflects the hardware pin state.
    // The Dreamcast boots at a fixed ratio; 0x0E0A is the typical value seen.
    // "Held" registers retain their value across manual resets.
    if (!Manual)
    {
        CPG_FRQCR  = 0x0E0A; // Power-on default (hardware pin-determined)
        CPG_STBCR  = 0x00;
        CPG_STBCR2 = 0x00;
        CPG_WTCNT  = 0x0000;
        CPG_WTCSR  = 0x0000;
    }
    // Manual reset: all CPG registers are "Held" — no changes needed
}

void cpg_Term()
{
    // Nothing to clean up; registers are statically allocated
}