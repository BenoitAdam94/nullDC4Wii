/*
    asic.cpp - Dreamcast Holly ASIC interrupt controller
    Bridges hardware interrupts from Holly peripherals to the SH4 CPU.
*/

#include "types.h"
#include "asic.h"
#include "dc/sh4/intc.h"
#include "dc/mem/sb.h"
#include "dc/maple/maple_if.h"

// Re-evaluate and forward all three IRL lines to the SH4.
// Called after any change to IST or IML registers.
static inline void asic_UpdatePending()
{
    // RL6 -> IRL9
    InterruptPend(sh4_IRL_9,
        ((SB_ISTNRM & SB_IML6NRM) | (SB_ISTERR & SB_IML6ERR) | (SB_ISTEXT & SB_IML6EXT)) != 0);

    // RL4 -> IRL11
    InterruptPend(sh4_IRL_11,
        ((SB_ISTNRM & SB_IML4NRM) | (SB_ISTERR & SB_IML4ERR) | (SB_ISTEXT & SB_IML4EXT)) != 0);

    // RL2 -> IRL13
    InterruptPend(sh4_IRL_13,
        ((SB_ISTNRM & SB_IML2NRM) | (SB_ISTERR & SB_IML2ERR) | (SB_ISTEXT & SB_IML2EXT)) != 0);
}

static void RaiseAsicNormal(HollyInterruptID inter)
{
    if (inter == holly_SCANINT2)
        maple_vblank();

    SB_ISTNRM |= 1u << (u8)inter;
    asic_UpdatePending();
}

static void RaiseAsicExt(HollyInterruptID inter)
{
    SB_ISTEXT |= 1u << (u8)inter;
    asic_UpdatePending();
}

static void RaiseAsicErr(HollyInterruptID inter)
{
    SB_ISTERR |= 1u << (u8)inter;
    asic_UpdatePending();
}

void fastcall asic_RaiseInterrupt(HollyInterruptID inter)
{
    switch (inter >> 8)
    {
    case 0: RaiseAsicNormal(inter); break;
    case 1: RaiseAsicExt(inter);    break;
    case 2: RaiseAsicErr(inter);    break;
    }
}

// BUG FIX: original always cleared SB_ISTEXT regardless of interrupt type.
void fastcall asic_CancelInterrupt(HollyInterruptID inter)
{
    u32 bit = ~(1u << (u8)inter);
    switch (inter >> 8)
    {
    case 0: SB_ISTNRM &= bit; break;
    case 1: SB_ISTEXT &= bit; break;  // EXT cancel may be ignored by hardware anyway
    case 2: SB_ISTERR &= bit; break;
    }
    asic_UpdatePending();
}

// --- Register read/write handlers ---

u32 Read_SB_ISTNRM()
{
    // Bits 29:0 = normal interrupts; bit 30 = EXT summary; bit 31 = ERR summary
    u32 tmp = SB_ISTNRM & 0x3FFFFFFFu;
    if (SB_ISTEXT) tmp |= 0x40000000u;
    if (SB_ISTERR) tmp |= 0x80000000u;
    return tmp;
}

void Write_SB_ISTNRM(u32 data)
{
    SB_ISTNRM &= ~data;  // Writing a 1 clears the bit (acknowledge)
    asic_UpdatePending();
}

void Write_SB_ISTEXT(u32 data)
{
    // EXT interrupts must be cancelled at the source — writes are intentionally ignored
    (void)data;
}

void Write_SB_ISTERR(u32 data)
{
    SB_ISTERR &= ~data;
    asic_UpdatePending();
}

// IML mask register writes — update the relevant priority line only
void Write_SB_SB_IML6NRM(u32 data) { SB_IML6NRM = data; InterruptPend(sh4_IRL_9,  ((SB_ISTNRM & SB_IML6NRM) | (SB_ISTERR & SB_IML6ERR) | (SB_ISTEXT & SB_IML6EXT)) != 0); }
void Write_SB_SB_IML4NRM(u32 data) { SB_IML4NRM = data; InterruptPend(sh4_IRL_11, ((SB_ISTNRM & SB_IML4NRM) | (SB_ISTERR & SB_IML4ERR) | (SB_ISTEXT & SB_IML4EXT)) != 0); }
void Write_SB_SB_IML2NRM(u32 data) { SB_IML2NRM = data; InterruptPend(sh4_IRL_13, ((SB_ISTNRM & SB_IML2NRM) | (SB_ISTERR & SB_IML2ERR) | (SB_ISTEXT & SB_IML2EXT)) != 0); }

void Write_SB_SB_IML6EXT(u32 data) { SB_IML6EXT = data; InterruptPend(sh4_IRL_9,  ((SB_ISTNRM & SB_IML6NRM) | (SB_ISTERR & SB_IML6ERR) | (SB_ISTEXT & SB_IML6EXT)) != 0); }
void Write_SB_SB_IML4EXT(u32 data) { SB_IML4EXT = data; InterruptPend(sh4_IRL_11, ((SB_ISTNRM & SB_IML4NRM) | (SB_ISTERR & SB_IML4ERR) | (SB_ISTEXT & SB_IML4EXT)) != 0); }
void Write_SB_SB_IML2EXT(u32 data) { SB_IML2EXT = data; InterruptPend(sh4_IRL_13, ((SB_ISTNRM & SB_IML2NRM) | (SB_ISTERR & SB_IML2ERR) | (SB_ISTEXT & SB_IML2EXT)) != 0); }

void Write_SB_SB_IML6ERR(u32 data) { SB_IML6ERR = data; InterruptPend(sh4_IRL_9,  ((SB_ISTNRM & SB_IML6NRM) | (SB_ISTERR & SB_IML6ERR) | (SB_ISTEXT & SB_IML6EXT)) != 0); }
void Write_SB_SB_IML4ERR(u32 data) { SB_IML4ERR = data; InterruptPend(sh4_IRL_11, ((SB_ISTNRM & SB_IML4NRM) | (SB_ISTERR & SB_IML4ERR) | (SB_ISTEXT & SB_IML4EXT)) != 0); }
void Write_SB_SB_IML2ERR(u32 data) { SB_IML2ERR = data; InterruptPend(sh4_IRL_13, ((SB_ISTNRM & SB_IML2NRM) | (SB_ISTERR & SB_IML2ERR) | (SB_ISTEXT & SB_IML2EXT)) != 0); }

// --- Lifecycle ---

void asic_reg_Init()
{
    sb_regs[((SB_ISTNRM_addr-SB_BASE)>>2)].flags         = REG_32BIT_READWRITE;
    sb_regs[((SB_ISTNRM_addr-SB_BASE)>>2)].readFunction  = Read_SB_ISTNRM;
    sb_regs[((SB_ISTNRM_addr-SB_BASE)>>2)].writeFunction = Write_SB_ISTNRM;

    sb_regs[((SB_ISTEXT_addr-SB_BASE)>>2)].flags         = REG_32BIT_READWRITE | REG_READ_DATA;
    sb_regs[((SB_ISTEXT_addr-SB_BASE)>>2)].writeFunction = Write_SB_ISTEXT;

    sb_regs[((SB_ISTERR_addr-SB_BASE)>>2)].flags         = REG_32BIT_READWRITE | REG_READ_DATA;
    sb_regs[((SB_ISTERR_addr-SB_BASE)>>2)].writeFunction = Write_SB_ISTERR;

    // NRM masks
    sb_regs[((SB_IML6NRM_addr-SB_BASE)>>2)].flags = REG_32BIT_READWRITE | REG_READ_DATA;
    sb_regs[((SB_IML6NRM_addr-SB_BASE)>>2)].writeFunction = Write_SB_SB_IML6NRM;
    sb_regs[((SB_IML4NRM_addr-SB_BASE)>>2)].flags = REG_32BIT_READWRITE | REG_READ_DATA;
    sb_regs[((SB_IML4NRM_addr-SB_BASE)>>2)].writeFunction = Write_SB_SB_IML4NRM;
    sb_regs[((SB_IML2NRM_addr-SB_BASE)>>2)].flags = REG_32BIT_READWRITE | REG_READ_DATA;
    sb_regs[((SB_IML2NRM_addr-SB_BASE)>>2)].writeFunction = Write_SB_SB_IML2NRM;

    // EXT masks
    sb_regs[((SB_IML6EXT_addr-SB_BASE)>>2)].flags = REG_32BIT_READWRITE | REG_READ_DATA;
    sb_regs[((SB_IML6EXT_addr-SB_BASE)>>2)].writeFunction = Write_SB_SB_IML6EXT;
    sb_regs[((SB_IML4EXT_addr-SB_BASE)>>2)].flags = REG_32BIT_READWRITE | REG_READ_DATA;
    sb_regs[((SB_IML4EXT_addr-SB_BASE)>>2)].writeFunction = Write_SB_SB_IML4EXT;
    sb_regs[((SB_IML2EXT_addr-SB_BASE)>>2)].flags = REG_32BIT_READWRITE | REG_READ_DATA;
    sb_regs[((SB_IML2EXT_addr-SB_BASE)>>2)].writeFunction = Write_SB_SB_IML2EXT;

    // ERR masks
    sb_regs[((SB_IML6ERR_addr-SB_BASE)>>2)].flags = REG_32BIT_READWRITE | REG_READ_DATA;
    sb_regs[((SB_IML6ERR_addr-SB_BASE)>>2)].writeFunction = Write_SB_SB_IML6ERR;
    sb_regs[((SB_IML4ERR_addr-SB_BASE)>>2)].flags = REG_32BIT_READWRITE | REG_READ_DATA;
    sb_regs[((SB_IML4ERR_addr-SB_BASE)>>2)].writeFunction = Write_SB_SB_IML4ERR;
    sb_regs[((SB_IML2ERR_addr-SB_BASE)>>2)].flags = REG_32BIT_READWRITE | REG_READ_DATA;
    sb_regs[((SB_IML2ERR_addr-SB_BASE)>>2)].writeFunction = Write_SB_SB_IML2ERR;
}

void asic_reg_Term()  {}
void asic_reg_Reset(bool Manual) {}