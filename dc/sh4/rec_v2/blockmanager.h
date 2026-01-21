/*
	In case you wonder, the extern "C" stuff are for the assembly code on beagleboard/pandora
*/
#include "types.h"
#pragma once
#if HOST_OS==OS_LINUX
extern "C" {
#endif

#define BM_BLOCKLIST_COUNT (16384)
#define BM_BLOCKLIST_MASK (BM_BLOCKLIST_COUNT-1)
#define BM_BLOCKLIST_SHIFT 2

typedef void DynarecCodeEntry();

DynarecCodeEntry* FASTCALL bm_GetCode(u32 addr);
void bm_AddCode(u32 addr,DynarecCodeEntry* code);
void bm_Reset();

struct DynarecBlock
{
	DynarecCodeEntry* code;
	u32 addr;
	u32 lookups;
	//u32 pad;
};
extern DynarecBlock* cache[BM_BLOCKLIST_COUNT];

#if HOST_OS==OS_LINUX
}
#endif
