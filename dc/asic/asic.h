#pragma once
#include "types.h"
#include "dc/sh4/intc.h"

//returns true if any RL6 Interrupts are pending
//bool asic_RL6Pending();
//return true if any RL4 iterupts are pending
//bool asic_RL4Pending();
//return true if any RL2 iterupts are pending
//bool asic_RL2Pending();
//Return interrupt priority level
//u32 asic_GetRL6Priority();
//Return interrupt priority level
//u32 asic_GetRL4Priority();
//Return interrupt priority level
//u32 asic_GetRL2Priority();
void fastcall asic_RaiseInterrupt(HollyInterruptID inter);
void fastcall asic_CancelInterrupt(HollyInterruptID inter);
//void RaiseAsicNormal(InterruptID inter);
//void RaiseAsicErr(InterruptID inter);
//void RaiseAsicExt(InterruptID inter);

//Init/Res/Term for regs
void asic_reg_Init();
void asic_reg_Term();
void asic_reg_Reset(bool Manual);

