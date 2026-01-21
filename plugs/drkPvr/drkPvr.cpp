// drkPvr.cpp : Defines the entry point for the DLL application.
//

#include "drkPvr.h"

#include "ta.h"
#include "spg.h"
#include "regs.h"
#include "Renderer_if.h"
#include "config/config.h"
#include <algorithm>

#define params PVRPARAMS
//RaiseInterruptFP* RaiseInterrupt;

//void* Hwnd;

//emu_info emu;
wchar emu_name[512];

pvr_init_params params;
_settings_type settings;

//u8*	params.vram;
//vramlock_Lock_32FP* lock32;
//vramlock_Lock_64FP* lock64;
//vramlock_Unlock_blockFP* unlock;


void FASTCALL libPvr_vramLockCB (vram_block* block,u32 addr)
{
	//rend_if_vram_locked_write(block,addr);
	//renderer->VramLockedWrite(block);
	//rend_text_invl(block);
}
#include <vector>
using std::vector;

u32 enable_FS_mid;
u32 AA_mid_menu;
u32 AA_mid_0;




//called when plugin is used by emu (you should do first time init here)
s32 FASTCALL libPvr_Load()
{
	LoadSettings();
	printf("drkpvr: Using %s\n",REND_NAME);
	return rv_ok;
}

//called when plugin is unloaded by emu , olny if dcInitPvr is called (eg , not called to enumerate plugins)
void FASTCALL libPvr_Unload()
{

}

//It's suposed to reset anything but vram (vram is set to 0 by emu)
void FASTCALL libPvr_Reset(bool Manual)
{
	Regs_Reset(Manual);
	spg_Reset(Manual);
	rend_reset(Manual);
}

//called when entering sh4 thread , from the new thread context (for any thread speciacific init)
s32 FASTCALL libPvr_Init(pvr_init_params* param)
{
	memcpy(&params,param,sizeof(params));


	if ((!Regs_Init()))
	{
		//failed
		return rv_error;
	}
	if (!spg_Init())
	{
		//failed
		return rv_error;
	}
	if (!rend_init())
	{
		//failed
		return rv_error;
	}
	//UpdateRRect();
	//olny the renderer cares about thread speciacific shit ..
	if (!rend_thread_start())
	{
		return rv_error;
	}

	return rv_ok;
}

//called when exiting from sh4 thread , from the new thread context (for any thread speciacific de init) :P
void FASTCALL libPvr_Term()
{
	rend_thread_end();

	rend_term();
	spg_Term();
	Regs_Term();
}

char* GetNullDCSoruceFileName(char* full);


int cfgGetInt(wchar* key,int def)
{
	return cfgLoadInt("drkpvr",key,def);
}
void cfgSetInt(wchar* key,int val)
{
	cfgSaveInt("drkpvr",key,val);
}

void LoadSettings()
{
	settings.Emulation.AlphaSortMode			=	cfgGetInt("Emulation.AlphaSortMode",1);
	settings.Emulation.PaletteMode				=	cfgGetInt("Emulation.PaletteMode",1);
	settings.Emulation.ModVolMode				= 	cfgGetInt("Emulation.ModVolMode",1);
	settings.Emulation.ZBufferMode				= 	cfgGetInt("Emulation.ZBufferMode",0);

	settings.OSD.ShowFPS						=	cfgGetInt("OSD.ShowFPS",0);
	settings.OSD.ShowStats						=	cfgGetInt("OSD.ShowStats",0);

	settings.Fullscreen.Enabled					=	cfgGetInt("Fullscreen.Enabled",0);
	//RECT rc;
	//GetWindowRect(GetDesktopWindow(),&rc);

	settings.Fullscreen.Res_X					=	cfgGetInt("Fullscreen.Res_X",-1);//rc.right
	settings.Fullscreen.Res_Y					=	cfgGetInt("Fullscreen.Res_Y",-1);//rc.bottom
	settings.Fullscreen.Refresh_Rate			=	cfgGetInt("Fullscreen.Refresh_Rate",-1);//60

	settings.Enhancements.MultiSampleCount		=	cfgGetInt("Enhancements.MultiSampleCount",0);
	settings.Enhancements.MultiSampleQuality	=	cfgGetInt("Enhancements.MultiSampleQuality",0);
	settings.Enhancements.AspectRatioMode		=	cfgGetInt("Enhancements.AspectRatioMode",1);
}


void SaveSettings()
{
	cfgSetInt("Emulation.AlphaSortMode",settings.Emulation.AlphaSortMode);
	cfgSetInt("Emulation.PaletteMode",settings.Emulation.PaletteMode);
	cfgSetInt("Emulation.ModVolMode",settings.Emulation.ModVolMode);
	cfgSetInt("Emulation.ZBufferMode",settings.Emulation.ZBufferMode);

	cfgSetInt("OSD.ShowFPS",settings.OSD.ShowFPS);
	cfgSetInt("OSD.ShowStats",settings.OSD.ShowStats);

	cfgSetInt("Fullscreen.Enabled",settings.Fullscreen.Enabled);
	cfgSetInt("Fullscreen.Res_X",settings.Fullscreen.Res_X);
	cfgSetInt("Fullscreen.Res_Y",settings.Fullscreen.Res_Y);
	cfgSetInt("Fullscreen.Refresh_Rate",settings.Fullscreen.Refresh_Rate);

	cfgSetInt("Enhancements.MultiSampleCount",settings.Enhancements.MultiSampleCount);
	cfgSetInt("Enhancements.MultiSampleQuality",settings.Enhancements.MultiSampleQuality);
	cfgSetInt("Enhancements.AspectRatioMode",settings.Enhancements.AspectRatioMode);
}

