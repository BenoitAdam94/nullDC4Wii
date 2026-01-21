/*
			--nullDC plugin managment code--
	Does plugin enumeration and handles dynamic lib loading.
	Also has code for plugin init/reset/term calls :).
*/
#include "types.h"
#include "plugin_manager.h"
#include "dc/pvr/pvr_if.h"
#include "dc/pvr/pvrLock.h"
#include "dc/aica/aica_if.h"
#include "dc/asic/asic.h"
#include "dc/gdrom/gdrom_if.h"
#include "dc/maple/maple_cfg.h"
#include "config/config.h"

#include <string.h>
#include "dc/mem/sb.h"

//to avoid including windows.h
#define EXCEPTION_EXECUTE_HANDLER       1
#define EXCEPTION_CONTINUE_SEARCH       0
#define EXCEPTION_CONTINUE_EXECUTION    -1

sh4_if						sh4_cpu;

struct MapleState
{
	bool Created;
	bool Inited;
};
extern MapleState MapleDevices_dd[4][6];


void maple_cfg_name(int i,int j,wchar * out)
{
	sprintf(out,"Current_maple%d_%d",i,j);
}

void maple_cfg_plug(int i,int j,wchar * out)
{
	wchar temp[512];
	maple_cfg_name(i,j,temp);
	cfgLoadStr("nullDC_plugins",temp,out,"NULL");
}

//Internal function for load/unload w/ error checking :)
s32 plugins_Load_()
{
	libPvr_Load();
	libGDR_Load();
	libAICA_Load();
	libExtDevice_Load();
	mcfg_CreateDevices();
	
#if 0
	libMaple_Load();

	wchar plug_name[512];
	for (int port=0;port<4;port++)
	{
		maple_cfg_plug(port,5,plug_name);
		if (strcmp(plug_name,_T("NULL"))!=0)
		{
			MapleDevices_dd[port][5].Created=true;
			s32 rv=libMaple_CreateDevice(&MapleDevices[port],plug_name);
			if (rv<0)
			{
				return rv;
			}

			u32 flags=rv;

			for (int subport=0;subport<5;subport++)
			{
				if (!(flags & (1<<subport)))
				{
					MapleDevices_dd[port][subport].Created=false;
					continue;
				}

				maple_cfg_plug(port,subport,plug_name);
				if (strcmp(plug_name,_T("NULL"))!=0)
				{
					MapleDevices_dd[port][subport].Created=true;
					
					//Create it
					s32 rv=libMaple_CreateSubDevice(&MapleDevices[port].subdevices[subport],plug_name);
					if (rv!=rv_ok)
					{
						return rv;
					}

					MapleDevices_dd[port][subport].Created=true;
				}
			}
		}
		else
		{
			MapleDevices_dd[port][5].Created=false;
		}
	}
#endif
	return rv_ok;
}

//Loads plugins , if allready loaded does nothing :)
bool plugins_Load()
{
	if (s32 rv=plugins_Load_())
	{
		if (rv==rv_error)
		{
			msgboxf("Unable to load plugins",MBX_ICONEXCLAMATION);
		}

		plugins_Unload();
		return false;
	}
	else
		return true;
}

//Unloads plugins , if allready unloaded does nothing
void plugins_Unload()
{
	mcfg_DestroyDevices();
#if 0
	for (int port=3;port>=0;port--)
	{
		if (MapleDevices_dd[port][5].Created)
		{
			for (int subport=4;subport>=0;subport--)
			{
				if (MapleDevices_dd[port][subport].Created)
				{
					MapleDevices_dd[port][subport].Created=false;

					s32 rv=DestroyMapleSubDevice(port,subport);
					if (rv!=rv_ok)
					{
						printf("DestroyMapleSubDevice(port,subport) failed: %d\n",rv);
					}
				}
			}
			MapleDevices_dd[port][5].Created=false;
			s32 rv=DestroyMapleDevice(port);
			if (rv!=rv_ok)
			{
				printf("DestroyMapleDevice(port) failed: %d\n",rv);
			}
		}
	}
#endif

	//libMaple_Unload
	libExtDevice_Unload();
	libAICA_Unload();
	libGDR_Unload();
	libPvr_Unload();
}

bool plugins_inited=false;


/************************************/
/*******Plugin Init/Term/Reset*******/
/************************************/
s32 plugins_Init_()
{
	if (plugins_inited)
		return rv_ok;
	plugins_inited=true;

	//pvr
	pvr_init_params pvr_info;
	pvr_info.RaiseInterrupt=asic_RaiseInterrupt;
	pvr_info.vram=&vram[0];

	if (s32 rv = libPvr_Init(&pvr_info))
		return rv;


	gdr_init_params gdr_info;

	if (s32 rv = libGDR_Init(&gdr_info))
		return rv;


	aica_init_params aica_info;
	aica_info.RaiseInterrupt=asic_RaiseInterrupt;
	aica_info.SB_ISTEXT=&SB_ISTEXT;
	aica_info.CancelInterrupt=asic_CancelInterrupt;
	aica_info.aica_ram=aica_ram.data;

	if (s32 rv = libAICA_Init(&aica_info))
		return rv;

	ext_device_init_params ext_device_info;
	ext_device_info.RaiseInterrupt=asic_RaiseInterrupt;
	ext_device_info.SB_ISTEXT=&SB_ISTEXT;
	ext_device_info.CancelInterrupt=asic_CancelInterrupt;

	if (s32 rv = libExtDevice_Init(&ext_device_info))
		return rv;


#if 0
	//Init Created maple devices
	for ( int i=0;i<4;i++)
	{
		if (MapleDevices_dd[i][5].Created)
		{
			lcp_name=_T("Made Device");
			verify(MapleDevices_dd[i][5].mdd!=0);
			//Init
			nullDC_Maple_plugin *nmp=FindMaplePlugin(MapleDevices_dd[i][5].mdd);
			lcp_name=MapleDevices_dd[i][5].mdd->Name;
			if (s32 rv=nmp->InitMain(&MapleDevices[i],MapleDevices_dd[i][5].mdd->id,&mip))
				return rv;

			MapleDevices_dd[i][5].Inited=true;
MapleDevices[i][5]
			for (int j=0;j<5;j++)
			{
				if (MapleDevices_dd[i][j].Created)
				{
					lcp_name=_T("Made SubDevice");
					verify(MapleDevices_dd[i][j].mdd!=0);
					//Init
					nullDC_Maple_plugin *nmp=FindMaplePlugin(MapleDevices_dd[i][j].mdd);
					lcp_name=MapleDevices_dd[i][j].mdd->Name;
					if (s32 rv=nmp->InitSub(&MapleDevices[i].subdevices[j],MapleDevices_dd[i][j].mdd->id,&mip))
						return rv;
				}
			}
		}
		else
			MapleDevices_dd[i][5].Inited=false;
	}
#endif
	return rv_ok;
}

bool plugins_Init()
{
	if (s32 rv = plugins_Init_())
	{
		printf("Plugins INIT FAIL\n");
		if (rv==rv_error)
		{
			msgboxf("Failed to init",MBX_ICONERROR);
		}
		plugins_Term();
		return false;
	}
	return true;
}

void plugins_Term()
{
	if (!plugins_inited)
		return;
	plugins_inited=false;

#if 0
	//Term inited maple devices
	for ( int i=3;i>=0;i--)
	{
		if (MapleDevices_dd[i][5].Inited)
		{
			for (int j=4;j>=0;j--)
			{
				if (MapleDevices_dd[i][j].Inited)
				{
					MapleDevices_dd[i][j].Inited=false;

					verify(MapleDevices_dd[i][j].mdd!=0);
					//term
					nullDC_Maple_plugin *nmp=FindMaplePlugin(MapleDevices_dd[i][j].mdd);
					nmp->TermSub(&MapleDevices[i].subdevices[j],MapleDevices_dd[i][j].mdd->id);
				}
			}

			MapleDevices_dd[i][5].Inited=false;

			verify(MapleDevices_dd[i][5].mdd!=0);
			//term
			nullDC_Maple_plugin *nmp=FindMaplePlugin(MapleDevices_dd[i][5].mdd);
			nmp->TermMain(&MapleDevices[i],MapleDevices_dd[i][5].mdd->id);
		}
	}
#endif

	//term all plugins
	libExtDevice_Term();
	libAICA_Term();
	libGDR_Term();
	libPvr_Term();
}

void plugins_Reset(bool Manual)
{
	libPvr_Reset(Manual);
	libGDR_Reset(Manual);
	libAICA_Reset(Manual);
	libExtDevice_Reset(Manual);
}

