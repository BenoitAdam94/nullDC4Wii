/*
	Emulation Interface for very high level stuff, like starting/pausing emulation
	This is not modified (much) from baseline, and thats why it looks so ugly.
	Needs to be rewriten for ndce as it doesn't use multithreading
*/

#include "mem/sh4_mem.h"
#include "mem/memutil.h"
#include "sh4/sh4_opcode_list.h"
#include "pvr/pvr_if.h"
#include "mem/sh4_internal_reg.h"
#include "aica/aica_if.h"
#include "maple/maple_if.h"
#include "dc.h"
#include "config/config.h"
#include <string.h>

bool dc_inited=false;
bool dc_reseted=false;
bool dc_Ignore_init=false;
bool dc_running=false;

void _Reset_DC(bool Manual);

enum emu_thread_state_t
{
	EMU_IDLE,
	EMU_CPU_START,
	EMU_SOFTRESET,
	EMU_NOP,
	EMU_QUIT,
	EMU_INIT,
	EMU_TERM,
	EMU_RESET,
	EMU_RESET_MANUAL,
};
enum emu_thread_rv_t
{
	RV_OK = 1,
	RV_ERROR=2,

	RV_EXEPTION=-2,
	RV_WAIT =-1,
};


volatile emu_thread_state_t emu_thread_state=EMU_IDLE;
volatile emu_thread_rv_t emu_thread_rv=RV_WAIT;

emu_thread_rv_t emu_rtc(emu_thread_state_t cmd)
{
	emu_thread_state=cmd;
	while(emu_thread_state!=EMU_QUIT)
	{
		switch(emu_thread_state)
		{
		case EMU_IDLE:
			return emu_thread_rv;
			break;

		case EMU_NOP:
			emu_thread_state=EMU_IDLE;

			emu_thread_rv=RV_OK;
			break;

		case EMU_CPU_START:
			emu_thread_state=EMU_IDLE;
			emu_thread_rv=RV_OK;
			sh4_cpu.Run();
			break;
		case EMU_SOFTRESET:
			emu_thread_state=EMU_CPU_START;
			_Reset_DC(true);
			break;

		case EMU_INIT:
			emu_thread_state=EMU_IDLE;


			if (!plugins_Init())
			{ 
				printf("Emulation thread : Plugin init failed\n"); 	
				plugins_Term();
				emu_thread_rv=RV_ERROR;
				break;
			}
			sh4_cpu.Init();


			mem_Init();
			pvr_Init();
			aica_Init();
			mem_map_defualt();

			emu_thread_rv=RV_OK;
			break;

		case EMU_TERM:
			emu_thread_state=EMU_IDLE;

			aica_Term();
			pvr_Term();
			mem_Term();
			sh4_cpu.Term();
			plugins_Term();


			emu_thread_rv=RV_OK;
			break;

		case EMU_RESET:
			emu_thread_state=EMU_IDLE;

			_Reset_DC(false);

			emu_thread_rv=RV_OK;
			break;

		case EMU_RESET_MANUAL:
			emu_thread_state=EMU_IDLE;

			_Reset_DC(true);

			//when we boot from ip.bin , it's nice to have it seted up

			emu_thread_rv=RV_OK;
			break;
		case EMU_QUIT:
			emu_thread_rv=RV_OK;
			break;

		}
	}
	return emu_thread_rv;
}


//Init mainly means allocate
//Reset is called before first run
//Init is called olny once
//When Init is called , cpu interface and all plugins configurations myst be finished
//Plugins/Cpu core must not change after this call is made.
bool Init_DC()
{
	if (dc_inited)
		return true;

	if (!plugins_Load())
		return false;

	if (emu_rtc(EMU_INIT)!=RV_OK)
		return false;

	dc_inited=true;
	return true;
}
void _Reset_DC(bool Manual)
{
	plugins_Reset(Manual);
	sh4_cpu.Reset(Manual);
	mem_Reset(Manual);
	pvr_Reset(Manual);
	aica_Reset(Manual);
}
bool SoftReset_DC()
{
	if (sh4_cpu.IsCpuRunning())
	{
		sh4_cpu.Stop();
		emu_rtc(EMU_SOFTRESET);
		return true;
	}
	else
		return false;
}
bool Reset_DC(bool Manual)
{
	if (!dc_inited || sh4_cpu.IsCpuRunning())
		return false;

	if (Manual)
		emu_rtc(EMU_RESET_MANUAL);
	else
		emu_rtc(EMU_RESET);

	dc_reseted=true;
	return true;
}

void Term_DC()
{
	if (dc_inited)
	{
		Stop_DC();
		emu_rtc(EMU_TERM);
		emu_rtc(EMU_QUIT);
		dc_inited=false;
	}
}

void LoadBiosFiles()
{
	wchar* temp_path=GetEmuPath("data/");
	u32 pl=(u32)strlen(temp_path);

	strcat(temp_path,"dc_boot.bin");

	if (!LoadFileToSh4Bootrom(temp_path))
	{

	}

	temp_path[pl]=0;
	//try to load saved flash
	strcat(temp_path,"dc_flash_wb.bin");
	if (!LoadFileToSh4Flashrom(temp_path))
	{
		//not found , load default :)
		temp_path[pl]=0;
		strcat(temp_path,"dc_flash.bin");
		LoadFileToSh4Flashrom(temp_path);
	}
	

	temp_path[pl]=0;
	strcat(temp_path,"syscalls.bin");
	LoadFileToSh4Mem(0x00000, temp_path);

	temp_path[pl]=0;
	strcat(temp_path,"IP.bin");
	LoadFileToSh4Mem(0x08000, temp_path);
	temp_path[pl]=0;


	free(temp_path);
}

void Start_DC()
{
	if (!sh4_cpu.IsCpuRunning())
	{
		if (!dc_inited)
		{
			if (!Init_DC())
				return;
		}
		if (!dc_reseted)
		{
			Reset_DC(false);//hard reset kthx
		}
		
		verify(emu_rtc(EMU_CPU_START)==RV_OK);

		//sh4_cpu.Run();
	}
}
void Stop_DC()
{
	if (dc_inited)//sh4_cpu may not be inited ;)
	{
		if (sh4_cpu.IsCpuRunning())
		{
			sh4_cpu.Stop();
			emu_rtc(EMU_NOP);
		}
	}
}

bool IsDCInited()
{
	return dc_inited;
}

