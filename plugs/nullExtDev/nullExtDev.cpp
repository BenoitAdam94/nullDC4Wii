// ExtDev isn't really emulated
//

#include "plugins/plugin_header.h"

//006* , on area0
u32 FASTCALL libExtDevice_ReadMem_A0_006(u32 addr,u32 size)
{
	return 0;	
}
void FASTCALL libExtDevice_WriteMem_A0_006(u32 addr,u32 data,u32 size)
{

}
//010* , on area0
u32 FASTCALL libExtDevice_ReadMem_A0_010(u32 addr,u32 size)
{
	return 0;
}
void FASTCALL libExtDevice_WriteMem_A0_010(u32 addr,u32 data,u32 size)
{

}
//Area 5
u32 FASTCALL libExtDevice_ReadMem_A5(u32 addr,u32 size)
{
	return 0;
}
void FASTCALL libExtDevice_WriteMem_A5(u32 addr,u32 data,u32 size)
{
}

void FASTCALL libExtDevice_Update(u32 cycles)
{
}

s32 FASTCALL libExtDevice_Load()
{
	return rv_ok;
}

void FASTCALL libExtDevice_Unload()
{

}

void FASTCALL libExtDevice_Reset(bool Manual)
{

}

s32 FASTCALL libExtDevice_Init(ext_device_init_params* p)
{
	return rv_ok;
}


void FASTCALL libExtDevice_Term()
{

}
