/*
	Tiny cute block manager. Doesn't keep block graphs or anything fancy ...
	Its based on a simple hashed-lists idea
*/

#include "blockmanager.h"
#include "ngen.h"

#include "../sh4_interpreter.h"
#include "../sh4_opcode_list.h"
#include "../sh4_registers.h"
#include "../sh4_if.h"
#include "dc/pvr/pvr_if.h"
#include "dc/aica/aica_if.h"
#include "../dmac.h"
#include "dc/gdrom/gdrom_if.h"
#include "../intc.h"
#include "../tmu.h"
#include "dc/mem/sh4_mem.h"

#define bm_AddrHash(addr) (((addr)>>BM_BLOCKLIST_SHIFT)&BM_BLOCKLIST_MASK)

#ifndef HOST_NO_REC


typedef vector<DynarecBlock> bm_List;
bm_List blocks[BM_BLOCKLIST_COUNT];

DynarecBlock* cache[BM_BLOCKLIST_COUNT];
DynarecBlock pBM_BLOCK_EMPTY={0,0xFFFFFFFF,0};
#define BM_BLOCK_EMPTY (&pBM_BLOCK_EMPTY)
extern u32 rdv_FailedToFindBlock_pc;

DynarecCodeEntry* FASTCALL bm_GetCode(u32 addr)
{
	u32 idx=bm_AddrHash(addr);
	if (cache[idx]->addr==addr)
	{
		cache[idx]->lookups++;
		return cache[idx]->code;
	}

	bm_List& block=blocks[idx];

	for(u32 i=0;i<block.size();i++)
	{
		if (block[i].addr==addr)
		{
			block[i].lookups++;
			if (block[i].lookups>cache[idx]->lookups)
			{
				cache[idx]=&(block[i]);
			}
			return block[i].code;
		}
	}

	rdv_FailedToFindBlock_pc=addr;
	return ngen_FailedToFindBlock;
}

void bm_AddCode(u32 addr,DynarecCodeEntry* code)
{
	u32 idx=bm_AddrHash(addr);
	bm_List& block=blocks[idx];
	DynarecBlock temp={code,addr};

	block.push_back(temp);
	cache[idx]=BM_BLOCK_EMPTY;
}

void bm_Reset()
{
	ngen_ResetBlocks();
	for (u32 i=0;i<BM_BLOCKLIST_COUNT;i++)
	{
		cache[i]=BM_BLOCK_EMPTY;
		blocks[i].clear();
	}
}

#endif //#ifndef HOST_NO_REC

