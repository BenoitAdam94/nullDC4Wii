#pragma once
#include "types.h"
#include "config.h"

#define LoadSettings LSPVR
#define SaveSettings SSPVR
#define settings pvrsetts

#include <stdlib.h>
#include <stdio.h>

#include <string.h>

int msgboxf(const wchar* text,unsigned int type,...);


#if DO_VERIFY==OP_ON
#define verifyf(x) verify(x)
#define verifyc(x) verify(FAILED(x))
#else
#undef verify
#define verify(x)
#define verifyf(x) (x)
#define verifyc(x) (x)
#endif

#define fverify verify

#include "ta_structs.h"

extern s32 render_end_pending_cycles;

extern pvr_init_params PVRPARAMS;

void LoadSettings();
void SaveSettings();

#if REND_API == REND_PSP
	#define REND_NAME "PSPgu DHA"
#elif REND_API == REND_GLES2
	#define REND_NAME "Opengl|ES 2.0 HAL"
#elif REND_API == REND_SOFT
	#define REND_NAME "Software"
#elif REND_API == REND_WII
	#define REND_NAME "WIIgx DHA"
#elif REND_API == REND_PS2
	#define REND_NAME "PS2gs DHA"
#else
	#error invalid config.REND_API must be set with one of REND_D3D/REND_OGL/REND_SW/REND_D3D_V2
#endif

struct _settings_type
{
	struct
	{
		u32 Enabled;
		u32 Res_X;
		u32 Res_Y;
		u32 Refresh_Rate;
	} Fullscreen;
	struct
	{
		u32 MultiSampleCount;
		u32 MultiSampleQuality;
		u32 AspectRatioMode;
	} Enhancements;

	struct
	{
		u32 PaletteMode;
		u32 AlphaSortMode;
		u32 ModVolMode;
		u32 ZBufferMode;
	} Emulation;
	struct
	{
		u32 ShowFPS;
		u32 ShowStats;
	} OSD;
};

extern _settings_type settings;

