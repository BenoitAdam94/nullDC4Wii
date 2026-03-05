/*
    Hitachi SH4 User Break Controller (UBC) — emulation stub

    The UBC provides two hardware breakpoint channels (A and B) with address,
    address-mask, bus-cycle, data, and data-mask registers. On retail Dreamcast
    hardware the UBC is disabled and cannot trigger breaks, but the register
    range is still mapped and software (notably kos-debug) reads/writes it, so
    we emulate the register bank as a simple read/write pass-through.

    All registers are "Held" on every reset type (power-on, manual, watchdog,
    H-UDI). Only BBRA, BBRB and BRCR have defined reset values (0x0000); the
    remaining registers are Undefined after reset and are left untouched here.

    Register map base: 0xFF200000 (P4) / 0x1F200000 (area 7)
*/

#include "types.h"
#include "dc/mem/sh4_internal_reg.h"
#include "ubc.h"

// ---------------------------------------------------------------------------
// Register mirror storage
// ---------------------------------------------------------------------------

u32 UBC_BARA  = 0;          // Break Address Register A       (32-bit, undef)
u8  UBC_BAMRA = 0;          // Break Address Mask Register A  (8-bit,  undef)
u16 UBC_BBRA  = 0x0000;     // Break Bus Cycle Register A     (16-bit, 0x0000)
u32 UBC_BARB  = 0;          // Break Address Register B       (32-bit, undef)
u8  UBC_BAMRB = 0;          // Break Address Mask Register B  (8-bit,  undef)
u16 UBC_BBRB  = 0x0000;     // Break Bus Cycle Register B     (16-bit, 0x0000)
u32 UBC_BDRB  = 0;          // Break Data Register B          (32-bit, undef)
u32 UBC_BDMRB = 0;          // Break Data Mask Register B     (32-bit, undef)
u16 UBC_BRCR  = 0x0000;     // Break Control Register         (16-bit, 0x0000)

// ---------------------------------------------------------------------------
// Internal helper: wire one register entry into the UBC[] map
// ---------------------------------------------------------------------------

// Generic flags shared by every UBC register (no custom read/write handlers)
#define UBC_FLAGS_32  (REG_32BIT_READWRITE | REG_READ_DATA | REG_WRITE_DATA)
#define UBC_FLAGS_16  (REG_16BIT_READWRITE | REG_READ_DATA | REG_WRITE_DATA)
#define UBC_FLAGS_8   (REG_8BIT_READWRITE  | REG_READ_DATA | REG_WRITE_DATA)

static inline void ubc_reg32(u32 addr, u32 *storage)
{
    u32 idx = UBC_REG_IDX(addr);
    UBC[idx].flags         = UBC_FLAGS_32;
    UBC[idx].readFunction  = 0;
    UBC[idx].writeFunction = 0;
    UBC[idx].data32        = storage;
}

static inline void ubc_reg16(u32 addr, u16 *storage)
{
    u32 idx = UBC_REG_IDX(addr);
    UBC[idx].flags         = UBC_FLAGS_16;
    UBC[idx].readFunction  = 0;
    UBC[idx].writeFunction = 0;
    UBC[idx].data16        = storage;
}

static inline void ubc_reg8(u32 addr, u8 *storage)
{
    u32 idx = UBC_REG_IDX(addr);
    UBC[idx].flags         = UBC_FLAGS_8;
    UBC[idx].readFunction  = 0;
    UBC[idx].writeFunction = 0;
    UBC[idx].data8         = storage;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ubc_Init()
{
    ubc_reg32(UBC_BARA_addr,  &UBC_BARA);
    ubc_reg8 (UBC_BAMRA_addr, &UBC_BAMRA);
    ubc_reg16(UBC_BBRA_addr,  &UBC_BBRA);
    ubc_reg32(UBC_BARB_addr,  &UBC_BARB);
    ubc_reg8 (UBC_BAMRB_addr, &UBC_BAMRB);
    ubc_reg16(UBC_BBRB_addr,  &UBC_BBRB);
    ubc_reg32(UBC_BDRB_addr,  &UBC_BDRB);
    ubc_reg32(UBC_BDMRB_addr, &UBC_BDMRB);
    ubc_reg16(UBC_BRCR_addr,  &UBC_BRCR);
}

void ubc_Reset(bool Manual)
{
    // The Manual parameter mirrors the reset-type distinction in the SH4
    // manual, but all UBC registers behave identically across reset types
    // ("Held" for everything). Only the three registers with defined reset
    // values are touched; the rest remain Undefined as per the hardware spec.
    (void)Manual;

    UBC_BBRA = 0x0000;
    UBC_BBRB = 0x0000;
    UBC_BRCR = 0x0000;
}

void ubc_Term()
{
    // No dynamic resources to release — the UBC register bank is statically
    // allocated and the UBC[] map entries will be cleaned up by the caller.
}
