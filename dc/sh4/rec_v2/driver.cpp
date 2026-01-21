#include "types.h"

#if HOST_OS==OS_WINDOWS
#include <windows.h>
#elif HOST_OS==OS_LINUX
#include <unistd.h>
#include <sys/mman.h>
#endif

#include "../sh4_interpreter.h"
#include "../sh4_opcode_list.h"
#include "../sh4_registers.h"
#include "../sh4_if.h"
#include "../dmac.h"
#include "../intc.h"
#include "../tmu.h"

#include "dc/mem/sh4_mem.h"
#include "dc/pvr/pvr_if.h"
#include "dc/aica/aica_if.h"
#include "dc/gdrom/gdrom_if.h"

#include <time.h>
#include <float.h>

#include "blockmanager.h"
#include "ngen.h"
#include "decoder.h"

#ifndef HOST_NO_REC
//uh uh
extern volatile bool  sh4_int_bCpuRun;


#if HOST_OS != OS_LINUX
u8 CodeCache[CODE_SIZE];
#elif HOST_OS == OS_LINUX
u8 pMemBuff[CODE_SIZE+4095];
u8 *CodeCache;
#endif


u32 LastAddr;
u32 LastAddr_min;
u32* emit_ptr=0;
void* emit_GetCCPtr() { return emit_ptr==0?(void*)&CodeCache[LastAddr]:(void*)emit_ptr; }
void emit_SetBaseAddr() { LastAddr_min = LastAddr; }
void emit_WriteCodeCache()
{
	wchar path[512];
	sprintf(path,"code_cache_%08X.bin",CodeCache);
	wchar* pt2=GetEmuPath(path);
	printf("Writing code cache to %s\n",pt2);
	FILE*f=fopen(pt2,"wb");
	if (f)
	{
		free(pt2);
		fwrite(CodeCache,LastAddr,1,f);
		fclose(f);
		printf("Writen!\n");
	}
}
void recSh4_ClearCache()
{
	LastAddr=LastAddr_min;
	bm_Reset();

	printf("recSh4:Dynarec Cache clear at %08X\n",curr_pc);
}

void recSh4_Run()
{
	sh4_int_bCpuRun=true;

	ngen_mainloop();

	sh4_int_bCpuRun=false;
}

void emit_Write8(u8 data)
{
	verify(!emit_ptr);
	CodeCache[LastAddr]=data;
	LastAddr+=1;
}
void emit_Write16(u16 data)
{
	verify(!emit_ptr);

	*(u16*)&CodeCache[LastAddr]=data;
	LastAddr+=2;
}

void emit_Write32(u32 data)
{
	if (emit_ptr)
	{
		*emit_ptr=data;
		emit_ptr++;
	}
	else
	{
		*(u32*)&CodeCache[LastAddr]=data;
		LastAddr+=4;
	}
}
void emit_Skip(u32 sz)
{
	LastAddr+=sz;
}
u32 emit_FreeSpace()
{
	return CODE_SIZE-LastAddr;
}


bool DoCheck(u32 pc)
{
	if (IsOnRam(pc))
	{
		pc&=0xFFFFFF;
		switch(pc)
		{
			case 0x3DAFC6:
			case 0x3C83F8:
				return true;

			default:
				return false;
		}
	}
	return false;
}

void AnalyseBlock(DecodedBlock* blk);
DynarecCodeEntry* rdv_CompileBlock(u32 bpc)
{
	DecodedBlock* blk=dec_DecodeBlock(bpc,fpscr,(SH4_TIMESLICE/2));
	AnalyseBlock(blk);
	DynarecCodeEntry* rv=ngen_Compile(blk,DoCheck(blk->start));
	dec_Cleanup();
	return rv;
}
u32 rdv_FailedToFindBlock_pc;

DynarecCodeEntry* rdv_CompilePC()
{
	u32 pc=next_pc;
	DynarecCodeEntry* rv;

	rv=rdv_CompileBlock(pc);
	
	if (rv==0)
	{
		recSh4_ClearCache();
		return rdv_CompilePC();
	}
	bm_AddCode(pc,rv);

	//check if the game or bootloader boots, and if so reset the dynarec cache
	//also make sure to never cache the boot addresses
	if ((pc&0xFFFFFF)==0x08300 || (pc&0xFFFFFF)==0x10000)
	{
		//**NOTE** the block has to still be valid after this
		//or it'l crash
		recSh4_ClearCache();
	}

	return rv;
}

DynarecCodeEntry* rdv_FailedToFindBlock()
{
	next_pc=rdv_FailedToFindBlock_pc;

	printf("rdv_FailedToFindBlock ~ %08X\n",next_pc);

	return rdv_CompilePC();
}

DynarecCodeEntry* FASTCALL rdv_BlockCheckFail(u32 pc)
{
	next_pc=pc;
	recSh4_ClearCache();
	return rdv_CompilePC();
}

DynarecCodeEntry* rdv_FindCode()
{
	DynarecCodeEntry* rv=bm_GetCode(next_pc);
	if (rv==ngen_FailedToFindBlock)
		return 0;
	
	return rv;
}

DynarecCodeEntry* rdv_FindOrCompile()
{
	DynarecCodeEntry* rv=bm_GetCode(next_pc);
	if (rv==ngen_FailedToFindBlock)
		rv=rdv_CompilePC();
	
	return rv;
}
void recSh4_Stop()
{
	Sh4_int_Stop();
}

void recSh4_Step()
{
	Sh4_int_Step();
}

void recSh4_Skip()
{
	Sh4_int_Skip();
}

void recSh4_Reset(bool Manual)
{
	Sh4_int_Reset(Manual);
}

void recSh4_Init()
{
	printf("recSh4 Init\n");
	Sh4_int_Init();
	bm_Reset();

#if HOST_OS == OS_WINDOWS
	DWORD old;
	VirtualProtect(CodeCache,CODE_SIZE,PAGE_EXECUTE_READWRITE,&old);
#elif HOST_OS == OS_LINUX

	//some overcomplicated code
    CodeCache = (u8*)(((unat)pMemBuff+4095)& ~4095);

    printf("\n\t CodeCache addr: %p | from: %p | addr here: %p\n", CodeCache, pMemBuff, recSh4_Init);

    if (mprotect(CodeCache, CODE_SIZE, PROT_EXEC|PROT_READ|PROT_WRITE)) {
        perror("\n\tError,Couldn’t mprotect CodeCache!");
        verify(false);
    }

    memset(CodeCache,0xCD,CODE_SIZE);

#endif
}

void recSh4_Term()
{
	printf("recSh4 Term\n");
	Sh4_int_Term();

#if HOST_OS == OS_LINUX
	//hum ?
#endif
}

bool recSh4_IsCpuRunning()
{
	return Sh4_int_IsCpuRunning();
}
#endif

void Get_Sh4Recompiler(sh4_if* rv)
{
    #ifdef HOST_NO_REC
    Get_Sh4Interpreter(rv);
    #else
	rv->Run=recSh4_Run;
	rv->Stop=recSh4_Stop;
	rv->Step=recSh4_Step;
	rv->Skip=recSh4_Skip;
	rv->Reset=recSh4_Reset;
	rv->Init=recSh4_Init;
	rv->Term=recSh4_Term;
	rv->IsCpuRunning=recSh4_IsCpuRunning;
	//rv->GetRegister=Sh4_int_GetRegister;
	//rv->SetRegister=Sh4_int_SetRegister;
	rv->ResetCache=recSh4_ClearCache;
	#endif
}

