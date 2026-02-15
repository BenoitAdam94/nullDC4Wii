// nullDC.cpp : Defines the entry point for the console application.
//

//initialse Emu
#include "types.h"
#include "dc/mem/_vmem.h"
#include "dc/sh4/sh4_opcode_list.h"
#include "stdclass.h"
#include "dc/dc.h"
#include "config/config.h"
#include "plugins/plugin_manager.h"
#include "cl/cl.h"


__settings settings;

//mainloop
int RunDC()
{

	if(settings.dynarec.Enable)
	{
		Get_Sh4Recompiler(&sh4_cpu);
		printf("Using Recompiler\n");
	}
	else
	{
		Get_Sh4Interpreter(&sh4_cpu);
		printf("Using Interpreter\n");
	}

	Start_DC();	//this call is blocking ...
	

#ifndef _ANDROID
	Term_DC();
#endif
	return 0;
}

//////////////////////////////////////

//command lineparsing & init
int main___(int argc,char* argv[])
{
	if(ParseCommandLine(argc,argv))
	{
		printf("\n\n(Exiting due to command line, without starting nullDC)\n");
		return 69;
	}

	if(!cfgOpen())
	{
		msgboxf(_T("Unable to open config file"),MBX_ICONERROR);
		return -4;
	}
	LoadSettings();

	int rv= 0;

	char* temp_path=GetEmuPath(_T("data/dc_boot.bin"));

	FILE* fr=fopen(temp_path,"r");
	if (!fr)
	{
		msgboxf(_T("Unable to find bios -- exiting\n%s"),MBX_ICONERROR,temp_path);
		rv=-3;
		goto cleanup;
	}
	free(temp_path);

	temp_path=GetEmuPath(_T("data/dc_flash.bin"));

	fr=fopen (temp_path,"r");
	if (!fr)
	{
		msgboxf(_T("Unable to find flash -- exiting\n%s"),MBX_ICONERROR,temp_path);
		rv=-6;
		goto cleanup;
	}

	free(temp_path);

	fclose(fr);

	printf("Loading plugins!\n");
	while (!plugins_Load())
	{
		//if (!plugins_Select())
		{
			msgboxf("Unable to load plugins -- exiting\n",MBX_ICONERROR);
			rv = -2;
			goto cleanup;
		}
	}

	rv = RunDC();

cleanup:

	printf("__Cleanup\n");

	SaveSettings();
	return rv;
}

//entry point, platform specific main() calls this when done with init/stuff
int EmuMain(int argc, char* argv[])
{
	printf(VER_FULLNAME " starting up ..");

	if (!_vmem_reserve())
	{
		msgboxf("Unable to reserve nullDC memory ...",MBX_OK | MBX_ICONERROR);
		return -5;
	}

	int rv=main___(argc,argv);

#ifndef _ANDROID
	_vmem_release();
#endif

	return rv;
}


void LoadSettings()
{
	printf("Loading settings\n");
	settings.dynarec.Enable=1|cfgLoadInt("nullDC","Dynarec.Enabled",1)!=0;
	settings.dynarec.CPpass=cfgLoadInt("nullDC","Dynarec.DoConstantPropagation",1)!=0;
	settings.dynarec.UnderclockFpu=cfgLoadInt("nullDC","Dynarec.UnderclockFpu",0)!=0;

	settings.dreamcast.cable=cfgLoadInt("nullDC","Dreamcast.Cable",3);
	settings.dreamcast.RTC=cfgLoadInt("nullDC","Dreamcast.RTC",GetRTC_now());

	settings.emulator.AutoStart=cfgLoadInt("nullDC","Emulator.AutoStart",0)!=0;
	settings.emulator.NoConsole=cfgLoadInt("nullDC","Emulator.NoConsole",0)!=0;
	printf("Loaded settings\n");
}
void SaveSettings()
{
	cfgSaveInt("nullDC","Dynarec.Enabled",settings.dynarec.Enable);
	cfgSaveInt("nullDC","Dynarec.DoConstantPropagation",settings.dynarec.CPpass);
	cfgSaveInt("nullDC","Dynarec.UnderclockFpu",settings.dynarec.UnderclockFpu);
	cfgSaveInt("nullDC","Dreamcast.Cable",settings.dreamcast.cable);
	cfgSaveInt("nullDC","Dreamcast.RTC",settings.dreamcast.RTC);
	cfgSaveInt("nullDC","Emulator.AutoStart",settings.emulator.AutoStart);
	cfgSaveInt("nullDC","Emulator.NoConsole",settings.emulator.NoConsole);
}
