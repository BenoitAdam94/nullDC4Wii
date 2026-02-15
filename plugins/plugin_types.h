#pragma once

#ifndef _PLUGIN_HEADER_
#error beef
#endif


/* There are for easy porting from ndc */
#define DC_PLATFORM_NORMAL		0
#define DC_PLATFORM DC_PLATFORM_NORMAL

#define FLASH_SIZE (128*1024)

#if (DC_PLATFORM==DC_PLATFORM_NORMAL)

	#define BUILD_DREAMCAST 1
	
	//DC : 16 mb ram, 8 mb vram, 2 mb aram, 2 mb bios, 128k flash
	#define RAM_SIZE (16*1024*1024) // 16MB RAM
	#define VRAM_SIZE (8*1024*1024) // 8MB VRAM
	#define ARAM_SIZE (2*1024*1024)
	#define BIOS_SIZE (2*1024*1024)

#else
	#error invalid build config
#endif

#define RAM_MASK	(RAM_SIZE-1)
#define VRAM_MASK	(VRAM_SIZE-1) // 0x7FFFFF
#define ARAM_MASK	(ARAM_SIZE-1)
#define BIOS_MASK	(BIOS_SIZE-1)
#define FLASH_MASK	(FLASH_SIZE-1)

#define SH4_CLOCK (200*1000*1000) // 200 Mhz

enum ndc_error_codes
{
	rv_ok = 0,		//no error

	rv_error=-2,	//error
	rv_serror=-1,	//silent error , it has been reported to the user
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////

//******************************************************
//*********************** PowerVR **********************
//******************************************************

struct pvr_init_params
{
	HollyRaiseInterruptFP*	RaiseInterrupt;

	//Vram is allocated by the emu.A pointer is given to the buffer here :)
	u8*					vram; 
};

//******************************************************
//************************ GDRom ***********************
//******************************************************
enum DiscType
{
	CdDA=0x00,
	CdRom=0x10,
	CdRom_XA=0x20,
	CdRom_Extra=0x30,
	CdRom_CDI=0x40,
	GdRom=0x80,		
	NoDisk=0x1,
	Open=0x2,			//tray is open :)
	Busy=0x3			//busy -> needs to be autmaticaly done by gdhost
};

enum DiskArea
{
	SingleDensity,
	DoubleDensity
};

enum DriveEvent
{
	DiskChange=1	//disk ejected/changed
};

//passed on GDRom init call
struct gdr_init_params
{
	wchar* source;
};
void NotifyEvent_gdrom(u32 info,void* param);

//******************************************************
//************************ AICA ************************
//******************************************************

//passed on AICA init call
struct aica_init_params
{
	HollyRaiseInterruptFP*	RaiseInterrupt;
	
	u8*				aica_ram;
	u32*			SB_ISTEXT;			//SB_ISTEXT register , so that aica can cancel interrupts =)
	HollyCancelInterruptFP* CancelInterrupt;
};

//******************************************************
//****************** Maple devices ******************
//******************************************************

//wait, what ?//

//******************************************************
//********************* Ext.Device *********************
//******************************************************


//passed on Ext.Device init call
struct ext_device_init_params
{
	HollyRaiseInterruptFP*	RaiseInterrupt;
	u32* SB_ISTEXT;
	HollyCancelInterruptFP* CancelInterrupt;
};