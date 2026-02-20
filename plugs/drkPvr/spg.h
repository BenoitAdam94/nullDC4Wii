#pragma once
#include "drkPvr.h"

bool spg_Init();
void spg_Term();
void spg_Reset(bool Manual);
void CalculateSync();
void FASTCALL libPvr_UpdatePvr(u32 cycles);