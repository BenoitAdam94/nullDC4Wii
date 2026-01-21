/*
	Highly inefficient and boring interpreter. Nothing special here
*/

#include "types.h"

#include "sh4_interpreter.h"
#include "sh4_opcode_list.h"
#include "sh4_registers.h"
#include "sh4_if.h"
#include "dc/pvr/pvr_if.h"
#include "dc/aica/aica_if.h"
#include "dmac.h"
#include "dc/gdrom/gdrom_if.h"
#include "dc/maple/maple_if.h"
#include "intc.h"
#include "tmu.h"
#include "dc/mem/sh4_mem.h"
#include "ccn.h"

#include <time.h>
#include <float.h>

#define SH4_TIMESLICE	(448)
#define CPU_RATIO		(8)

//uh uh
volatile bool  sh4_int_bCpuRun=false;

#define GetN(str) ((str>>8) & 0xf)
#define GetM(str) ((str>>4) & 0xf)

void Sh4_int_Run()
{
	sh4_int_bCpuRun=true;

	s32 l=SH4_TIMESLICE;

	do
	{
		do
		{
			u32 op=ReadMem16(next_pc);
			next_pc+=2;

			OpPtr[op](op);
			l-=CPU_RATIO;
		} while(l>0);
		l+=SH4_TIMESLICE;
		UpdateSystem();

	} while(sh4_int_bCpuRun);

	sh4_int_bCpuRun=false;
}

void Sh4_int_Stop()
{
	if (sh4_int_bCpuRun)
	{
		sh4_int_bCpuRun=false;
	}
}

void Sh4_int_Step()
{
	if (sh4_int_bCpuRun)
	{
		printf("Sh4 Is running , can't step\n");
	}
	else
	{
		u32 op=ReadMem16(next_pc);
		next_pc+=2;
		ExecuteOpcode(op);
	}
}

void Sh4_int_Skip()
{
	if (sh4_int_bCpuRun)
	{
		printf("Sh4 Is running , can't Skip\n");
	}
	else
	{
		next_pc+=2;
	}
}

void Sh4_int_Reset(bool Manual)
{
	if (sh4_int_bCpuRun)
	{
		printf("Sh4 Is running , can't Reset\n");
	}
	else
	{
		next_pc = 0xA0000000;

		memset(r,0,sizeof(r));
		memset(r_bank,0,sizeof(r_bank));

		gbr=ssr=spc=sgr=dbr=vbr=0;
		mach=macl=pr=fpul=0;

		sr.SetFull(0x700000F0);
		old_sr=sr;
		UpdateSR();

		fpscr.full = 0x0004001;
		old_fpscr=fpscr;
		UpdateFPSCR();

		//Any more registers have default value ?
		printf("Sh4 Reset\n");
	}
}

f32 sin_table[0x10000+0x4000];	//+0x4000 to avoid having to warp around twice on cos

void GenerateSinCos()
{
	printf("Generating sincos tables ...\n");
	wchar* path=GetEmuPath("data/fsca-table.bin");
	FILE* tbl=fopen(path,"rb");
	free(path);
	if (!tbl)
		die("fsca-table.bin is missing!");
	fread(sin_table,1,4*0x8000,tbl);
	fclose(tbl);

	for (int i=0x0000;i<0x8000;i++)
	{
		(u32&)sin_table[i]=host_to_le<4>((u32&)sin_table[i]);
	}

	for (int i=0x8000;i<0x10000;i++)
	{
		if (i==0x8000)
			sin_table[i]=0;
		else
			sin_table[i]=-sin_table[i-0x8000];
	}
	for (int i=0x10000;i<0x14000;i++)
	{
		sin_table[i]=sin_table[i&0xFFFF];//warp around for the last 0x4000 entries
	}
}
void Sh4_int_Init()
{
	BuildOpcodeTables();
	GenerateSinCos();
}

void Sh4_int_Term()
{
	Sh4_int_Stop();
	printf("Sh4 Term\n");
}

bool Sh4_int_IsCpuRunning()
{
	return sh4_int_bCpuRun;
}

//TODO : Check for valid delayslot instruction
void ExecuteDelayslot()
{
	u32 op=IReadMem16(next_pc);
	next_pc+=2;
	if (op!=0)
		ExecuteOpcode(op);
}

void ExecuteDelayslot_RTE()
{
	sr.SetFull(ssr);

	ExecuteDelayslot();
}

//General update
s32 rtc_cycles=0;
u32 update_cnt;
u32 gcp_timer=0;


//14336 Cycles
void FASTCALL VerySlowUpdate()
{
	//gpc_counter=0;
	gcp_timer++;
	rtc_cycles-=14336;
	if (rtc_cycles<=0)
	{
		rtc_cycles+=SH4_CLOCK;
		settings.dreamcast.RTC++;
	}
	maple_Update(14336);
}

//7168 Cycles
void FASTCALL SlowUpdate()
{
	UpdateGDRom();

	if (!(update_cnt&0x10))
		VerySlowUpdate();
}

//3584 Cycles
void FASTCALL MediumUpdate()
{
	UpdateAica(3584);
//	libExtDevice_Update(3584);
	UpdateDMA();
	if (!(update_cnt&0x8))
		SlowUpdate();
}

//448 Cycles (fixed)
int FASTCALL UpdateSystem()
{
	if (!(update_cnt&0x7))
		MediumUpdate();

	update_cnt++;

	UpdateTMU(448);
	UpdatePvr(448);
	return UpdateINTC();
}
void sh4_int_resetcache() { }
//Get an interface to sh4 interpreter
void Get_Sh4Interpreter(sh4_if* rv)
{
	rv->Run=Sh4_int_Run;
	rv->Stop=Sh4_int_Stop;
	rv->Step=Sh4_int_Step;
	rv->Skip=Sh4_int_Skip;
	rv->Reset=Sh4_int_Reset;
	rv->Init=Sh4_int_Init;
	rv->Term=Sh4_int_Term;
	rv->IsCpuRunning=Sh4_int_IsCpuRunning;

	rv->ResetCache=sh4_int_resetcache;
}

