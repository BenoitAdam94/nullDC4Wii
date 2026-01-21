/*
	This file was for vram memory locking.Its kind of not needed on ndce
*/
#include "plugins/plugin_manager.h"
#include "pvrLock.h"
 
using namespace std;

//vram 32-64b
VArray2 vram;
 
//So, the bytes 0xA4000000-0xA4000003 correspond to 0xA5000000-0xA5000003, 0xA4000004-0xA4000007 to 0xA5400000-0xA5400003,
//0xA4000008-0xA400000B to 0xA5000004-0xA5000007 and so on. 
//
//
//Convert Sh4 address to vram_32 offset
u32 vramlock_ConvAddrtoOffset32(u32 Address)
{
	if (Is_64_Bit(Address))
	{
		//64b wide bus is archevied by interleavingthe banks every 32 bits
		//so bank is Address>>3
		//so >>1 to get real uper offset
		u32 t=(Address>>1)& ((VRAM_MASK>>1)-3);	//upper bits
		u32 t2=Address&0x3;	//lower bits
		//u32 t3=t& 0x7FFFFC;		//clean upper bits
		t=((Address& (1<<2))<<20)|t|t2;//bank offset |clean upper bits|lower bits -> Ready :)!
 
		return t;
	}
	else
	{
		return  Address & VRAM_MASK;
	}
}
 
//Convert offset64 to offset32
u32 vramlock_ConvOffset64toOffset32(u32 offset64)
{
		//64b wide bus is archevied by interleavingthe banks every 32 bits
		//so bank is Address>>3
		//so >>1 to get real uper offset
		u32 t=(offset64>>1)& (VRAM_MASK-3);	//upper bits
		u32 t2=offset64&0x3;	//lower bits
		//u32 t3=t& 0x7FFFFC;		//clean upper bits
		t=((offset64& (1<<2))<<20)|t|t2;//bank offset |clean upper bits|lower bits -> Ready :)!
 
		return t;
}
//Convert Sh4 address to vram_64 offset
u32 vramlock_ConvAddrtoOffset64(u32 Address)
{
	if (Is_64_Bit(Address))
	{
		return  Address & VRAM_MASK;//64 bit offset
	}
	else
	{
		//64b wide bus is archevied by interleaving the banks every 32 bits
		//so bank is Address<<3
		//bits <4 are <<1 to create space for bank num
		//bank 0 is mapped at 400000 (32b offset) and after
		u32 bank=((Address>>22)&0x1)<<2;//bank will be used as uper offset too
		u32 lv=Address&0x3; //these will survive
		Address<<=1;
		//       |inbank offset    |       bank id        | lower 2 bits (not changed)
		u32 rv=  (Address&(VRAM_MASK-7))|bank                  | lv;
 
		return rv;
	}
}
//Convert offset32 to offset64
u32 vramlock_ConvOffset32toOffset642(u32 offset32)
{
		//64b wide bus is archevied by interleaving the banks every 32 bits
		//so bank is Address<<3
		//bits <4 are <<1 to create space for bank num
		//bank 1 is mapped at 400000 (32b offset) and after
		//u32 bank=((offset32>>22)&0x1)<<2;//bank will be used ass uper offset too
		offset32&=VRAM_MASK;
		u32 uv=offset32&0x800000;
		u32 mv=offset32&0x3FFFFC;	//in bank offest
		u32 bank=(offset32 & (0x400000)) ? 4:0;
		u32 lv=offset32&0x3; //these will survive
		
		//       upper_value | inbank offset| bank id | lower 2 bits (not changed)
		u32 rv=  uv			 |(mv<<1)		| bank    | lv;
 
		return rv;
}

u32 vramlock_ConvOffset32toOffset64(u32 offset32)
{
		//64b wide bus is archevied by interleaving the banks every 32 bits
		//so bank is Address<<3
		//bits <4 are <<1 to create space for bank num
		//bank 0 is mapped at 400000 (32b offset) and after
		u32 bank=((offset32>>22)&0x1)<<2;//bank will be used as uper offset too
		u32 lv=offset32&0x3; //these will survive
		offset32<<=1;
		//       |inbank offset    |       bank id        | lower 2 bits (not changed)
		u32 rv=  (offset32&(VRAM_MASK-7))|bank                  | lv;
 
		return rv;
}