/*
	Some WIP optimisation stuff and maby helper functions for shil
*/

#include "types.h"
#include "shil.h"
#include "decoder.h"

u32 RegisterWrite[sh4_reg_count];
u32 RegisterRead[sh4_reg_count];

void RegReadInfo(shil_param p,size_t ord)
{
	if (p.is_reg())
	{
		for (int i=0;i<p.count();i++)
			RegisterRead[p._reg+i]=ord;
	}
}
void RegWriteInfo(shil_opcode* ops, shil_param p,size_t ord)
{
	if (p.is_reg())
	{
		for (int i=0;i<p.count();i++)
		{
			if (RegisterWrite[p._reg+i]>=RegisterRead[p._reg+i] && RegisterWrite[p._reg+i]!=0xFFFFFFFF)	//if last read was before last write, and there was a last write
			{
				printf("DEAD OPCODE %d %d!\n",RegisterWrite[p._reg+i],ord);
				ops[RegisterWrite[p._reg+i]].Flow=1;						//the last write was unused
			}
			RegisterWrite[p._reg+i]=ord;
		}
	}
}
u32 fallback_blocks;
u32 total_blocks;
u32 REMOVED_OPS;

//Simplistic Write after Write without read pass to remove (a few) dead opcodes
//Seems to be working
void AnalyseBlock(DecodedBlock* blk)
{
	return;	//disbled to be on the safe side ..
	memset(RegisterWrite,-1,sizeof(RegisterWrite));
	memset(RegisterRead,-1,sizeof(RegisterRead));

	total_blocks++;
	for (size_t i=0;i<blk->oplist.size();i++)
	{
		shil_opcode* op=&blk->oplist[i];
		op->Flow=0;
		if (op->op==shop_ifb)
		{
			fallback_blocks++;
			return;
		}

		RegReadInfo(op->rs1,i);
		RegReadInfo(op->rs2,i);
		RegReadInfo(op->rs3,i);

		RegWriteInfo(&blk->oplist[0],op->rd,i);
		RegWriteInfo(&blk->oplist[0],op->rd2,i);
	}

	for (size_t i=0;i<blk->oplist.size();i++)
	{
		if (blk->oplist[i].Flow)
		{
			blk->oplist.erase(blk->oplist.begin()+i);
			REMOVED_OPS++;
			i--;
		}
	}

	int affregs=0;
	for (int i=0;i<16;i++)
	{
		if (RegisterWrite[i]!=0)
		{
			affregs++;
			//printf("r%02d:%02d ",i,RegisterWrite[i]);
		}
	}
	//printf("<> %d\n",affregs);

	printf("%d FB, %d native, %.2f%% || %d removed ops!\n",fallback_blocks,total_blocks-fallback_blocks,fallback_blocks*100.f/total_blocks,REMOVED_OPS);
	//printf("\nBlock: %d affecter regs %d c\n",affregs,blk->cycles);
}

void FASTCALL do_sqw_mmu(u32 dst);
void FASTCALL do_sqw_nommu(u32 dst);
void UpdateFPSCR();
bool UpdateSR();
#include "dc/sh4/ccn.h"
#include "ngen.h"
#include "dc/sh4/sh4_registers.h"


#define SHIL_MODE 1
#include "shil_canonical.h"

//#define SHIL_MODE 2
//#include "shil_canonical.h"

#define SHIL_MODE 3
#include "shil_canonical.h"

