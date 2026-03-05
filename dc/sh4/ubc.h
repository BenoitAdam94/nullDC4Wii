#pragma once

#include "types.h"

// Hitachi SH4 User Break Controller (UBC)
// The UBC is disabled on retail Dreamcast hardware, but is emulated here
// for software compatibility (e.g. kos-debug relies on these registers).
//
// Register map base: 0xFF200000 (P4) / 0x1F200000 (area 7)
// All registers are "Held" on all reset types — only BBRA, BBRB, BRCR
// have defined reset values (0x0000); the rest are Undefined.

// Offset index helper: converts a register address offset to a UBC[] array index
// Register addresses are defined in sh4_internal_reg.h (0x1F200000 base, area 7)
#define UBC_REG_IDX(addr)  (((addr) & 0xFF) >> 2)

// UBC register mirrors
// BARA  — Break Address Register A       (32-bit, reset: Undefined)
extern u32 UBC_BARA;
// BAMRA — Break Address Mask Register A  (8-bit,  reset: Undefined)
extern u8  UBC_BAMRA;
// BBRA  — Break Bus Cycle Register A     (16-bit, reset: 0x0000)
extern u16 UBC_BBRA;
// BARB  — Break Address Register B       (32-bit, reset: Undefined)
extern u32 UBC_BARB;
// BAMRB — Break Address Mask Register B  (8-bit,  reset: Undefined)
extern u8  UBC_BAMRB;
// BBRB  — Break Bus Cycle Register B     (16-bit, reset: 0x0000)
extern u16 UBC_BBRB;
// BDRB  — Break Data Register B          (32-bit, reset: Undefined)
extern u32 UBC_BDRB;
// BDMRB — Break Data Mask Register B     (32-bit, reset: Undefined)
extern u32 UBC_BDMRB;
// BRCR  — Break Control Register         (16-bit, reset: 0x0000)
extern u16 UBC_BRCR;

// Lifecycle
void ubc_Init();
void ubc_Reset(bool Manual);
void ubc_Term();
