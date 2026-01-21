#pragma once
#include "types.h"
#include "plugins/plugin_manager.h"

//regs
u32 pvr_readreg_TA(u32 addr,u32 sz);
void pvr_writereg_TA(u32 addr,u32 data,u32 sz);

//vram 32-64b
extern VArray2 vram;
//read
u8 FASTCALL pvr_read_area1_8(u32 addr);
u16 FASTCALL pvr_read_area1_16(u32 addr);
u32 FASTCALL pvr_read_area1_32(u32 addr);
//write
void FASTCALL pvr_write_area1_8(u32 addr,u8 data);
void FASTCALL pvr_write_area1_16(u32 addr,u16 data);
void FASTCALL pvr_write_area1_32(u32 addr,u32 data);

void pvr_Update(u32 cycles);

//Init/Term , global
void pvr_Init();
void pvr_Term();
//Reset -> Reset - Initialise
void pvr_Reset(bool Manual);

void FASTCALL TAWrite(u32 address,u32* data,u32 count);
void FASTCALL TAWriteSQ(u32 address,u32* data);

//
#define UpdatePvr(clc) libPvr_UpdatePvr(clc)


//registers 
#define PVR_BASE 0x005F8000
