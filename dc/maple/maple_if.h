#pragma once
#include "types.h"
#include "plugins/plugin_manager.h"
#include "maple_devs.h"

extern maple_device* MapleDevices[4][6];

void maple_Init();
void maple_Reset(bool Manual);
void maple_Term();

void maple_vblank();
void maple_Update(u32 cycls);