#pragma once

#include "types.h"

//16 mb for naomi.. so be carefull not to hardcode it anywhere


#define Is_64_Bit(addr) ((addr &0x1000000)==0)
 
//vram_block, vramLockCBFP on plugin headers


u32 vramlock_ConvAddrtoOffset64(u32 Address);
u32 vramlock_ConvOffset32toOffset64(u32 offset32);