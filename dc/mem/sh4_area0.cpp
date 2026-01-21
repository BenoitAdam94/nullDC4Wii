#include "types.h"
#include "sh4_mem.h"
#include "sb.h"
#include "dc/pvr/pvr_if.h"
#include "dc/gdrom/gdrom_if.h"
#include "dc/aica/aica_if.h"

#include "plugins/plugin_manager.h"

#pragma warning( disable : 4127 /*4244*/)
//Area 0 mem map
//0x00000000- 0x001FFFFF	:MPX	System/Boot ROM
//0x00200000- 0x0021FFFF	:Flash Memory
//0x00400000- 0x005F67FF	:Unassigned
//0x005F6800- 0x005F69FF	:System Control Reg.
//0x005F6C00- 0x005F6CFF	:Maple i/f Control Reg.
//0x005F7000- 0x005F70FF	:GD-ROM / NAOMI BD Reg.
//0x005F7400- 0x005F74FF	:G1 i/f Control Reg.
//0x005F7800- 0x005F78FF	:G2 i/f Control Reg.
//0x005F7C00- 0x005F7CFF	:PVR i/f Control Reg.
//0x005F8000- 0x005F9FFF	:TA / PVR Core Reg.
//0x00600000- 0x006007FF	:MODEM
//0x00600800- 0x006FFFFF	:G2 (Reserved)
//0x00700000- 0x00707FFF	:AICA- Sound Cntr. Reg.
//0x00710000- 0x0071000B	:AICA- RTC Cntr. Reg.
//0x00800000- 0x00FFFFFF	:AICA- Wave Memory
//0x01000000- 0x01FFFFFF	:Ext. Device
//0x02000000- 0x03FFFFFF*	:Image Area*	2MB




// (very) tmp (slow) swaps for test
//
/*
inline static u16 bswap16(u16 v)
{
	u32 rv = ((v&255)<<8) | ((v>>8)&255) ;
	return rv;
}

inline static u32 bswap32(u32 v)
{
	u32 rv=0;
	((u8*)&rv)[0] = ((u8*)&v)[3];
        ((u8*)&rv)[1] = ((u8*)&v)[2];
        ((u8*)&rv)[2] = ((u8*)&v)[1];
        ((u8*)&rv)[3] = ((u8*)&v)[0];
	return rv;
}
*/





//use unified size handler for registers
//it realy makes no sense to use different size handlers on em -> especialy when we can use templates :p
template<u32 sz, class T, u32 b_start,u32 b_end>
T FASTCALL ReadMem_area0(u32 addr)
{
	addr &= 0x01FFFFFF;//to get rid of non needed bits
	const u32 base_start=addr>>16;//b_start & 0x01FF;
	const u32 base_end=addr>>16;//b_end & 0x01FF;
	//map 0x0000 to 0x01FF to Default handler
	//mirror 0x0200 to 0x03FF , from 0x0000 to 0x03FFF
	//map 0x0000 to 0x001F
	if (/*(base_start>=0x0000) && */(base_end<= 0x001F))//	:MPX	System/Boot ROM
	{
		switch (sz)
		{
		case 1:
			return (T)bios_b[addr];
		case 2:
			return (T)HOST_TO_LE16(*(u16*)&bios_b[addr]);
		case 4:
			return (T)HOST_TO_LE32(*(u32*)&bios_b[addr]);
		}

		//ReadMemArrRet(bios_b,addr,sz);
		EMUERROR3("Read from [MPX	System/Boot ROM], addr=%x , sz=%d",addr,sz);
	}
	//map 0x0020 to 0x0021
	else if ((base_start>= 0x0020) && (base_end<= 0x0021))		//	:Flash Memory
	{
		//ReadMemFromPtrRet(flashrom,adr-0x00200000,sz);
		addr-=0x00200000;
		//ReadMemArrRet(flash_b,addr-0x00200000,sz);
        switch (sz)
        {
        case 1:
                return (T)flash_b[addr];
        case 2:
                return (T)HOST_TO_LE16(*(u16*)&flash_b[addr]);
        case 4:
                return (T)HOST_TO_LE32(*(u32*)&flash_b[addr]);
        }

		EMUERROR3("Read from [Flash Memory], addr=%x , sz=%d",addr,sz);
	}
	//map 0x005F to 0x005F
	else if ((base_start >=0x005F) && (base_end <=0x005F) /*&& (addr>= 0x00400000)*/ && (addr<= 0x005F67FF))		//	:Unassigned
	{
		EMUERROR2("Read from area0_32 not implemented [Unassigned], addr=%x",addr);
	}
	/*
	else if ((base_start >=0x005F) && (base_end <=0x005F) && (addr>= 0x005F6800) && (addr<= 0x005F69FF))		//	:System Control Reg.
	{
		return (T)sb_ReadMem(addr,sz);
	}
	else if ((base_start >=0x005F) && (base_end <=0x005F) && (addr>= 0x005F6C00) && (addr<= 0x005F6CFF)) //	:Maple i/f Control Reg.
	{
		return (T)sb_ReadMem(addr,sz);
	}
	else if ((base_start >=0x005F) && (base_end <=0x005F) && (addr>= 0x005F7400) && (addr<=0x005F74FF)) //	:G1 i/f Control Reg.
	{
		return (T)sb_ReadMem(addr,sz);
	}
	else if ((base_start >=0x005F) && (base_end <=0x005F) && (addr>= 0x005F7800) && (addr<=0x005F78FF)) //	:G2 i/f Control Reg.
	{
		return (T)sb_ReadMem(addr,sz);
	}*/
	else if ((base_start >=0x005F) && (base_end <=0x005F) && (addr>= 0x005F7000) && (addr<= 0x005F70FF)) //	:GD-ROM
	{
		//EMUERROR3("Read from area0_32 not implemented [GD-ROM], addr=%x,size=%d",addr,sz);
		return (T)ReadMem_gdrom(addr,sz);
	}
	else if ((base_start >=0x005F) && (base_end <=0x005F) && (addr>= 0x005F6800) && (addr<=0x005F7CFF)) //	/*:PVR i/f Control Reg.*/ -> ALL SB registers now
	{
		//EMUERROR2("Read from area0_32 not implemented [PVR i/f Control Reg], addr=%x",addr);
		return (T)sb_ReadMem(addr,sz);
	}
	else if ((base_start >=0x005F) && (base_end <=0x005F) && (addr>= 0x005F8000) && (addr<=0x005F9FFF)) //	:TA / PVR Core Reg.
	{
		//EMUERROR2("Read from area0_32 not implemented [TA / PVR Core Reg], addr=%x",addr);
		return (T)pvr_readreg_TA(addr,sz);
	}
	//map 0x0060 to 0x0060
	else if ((base_start >=0x0060) && (base_end <=0x0060) /*&& (addr>= 0x00600000)*/ && (addr<= 0x006007FF)) //	:MODEM
	{
		return (T)libExtDevice_ReadMem_A0_006(addr,sz);
		//EMUERROR2("Read from area0_32 not implemented [MODEM], addr=%x",addr);
	}
	//map 0x0060 to 0x006F
	else if ((base_start >=0x0060) && (base_end <=0x006F) && (addr>= 0x00600800) && (addr<= 0x006FFFFF)) //	:G2 (Reserved)
	{
		EMUERROR2("Read from area0_32 not implemented [G2 (Reserved)], addr=%x",addr);
	}
	//map 0x0070 to 0x0070
	else if ((base_start >=0x0070) && (base_end <=0x0070) /*&& (addr>= 0x00700000)*/ && (addr<=0x00707FFF)) //	:AICA- Sound Cntr. Reg.
	{
		//EMUERROR2("Read from area0_32 not implemented [AICA- Sound Cntr. Reg], addr=%x",addr);
		return (T) ReadMem_aica_reg(addr,sz);//libAICA_ReadMem_aica_reg(addr,sz);
	}
	//map 0x0071 to 0x0071
	else if ((base_start >=0x0071) && (base_end <=0x0071) /*&& (addr>= 0x00710000)*/ && (addr<= 0x0071000B)) //	:AICA- RTC Cntr. Reg.
	{
		//EMUERROR2("Read from area0_32 not implemented [AICA- RTC Cntr. Reg], addr=%x",addr);
		return (T)ReadMem_aica_rtc(addr,sz);
	}
	//map 0x0080 to 0x00FF
	else if ((base_start >=0x0080) && (base_end <=0x00FF) /*&& (addr>= 0x00800000) && (addr<=0x00FFFFFF)*/) //	:AICA- Wave Memory
	{
		//EMUERROR2("Read from area0_32 not implemented [AICA- Wave Memory], addr=%x",addr);
		return (T)libAICA_ReadMem_aica_ram(addr,sz);
	}
	//map 0x0100 to 0x01FF
	else if ((base_start >=0x0100) && (base_end <=0x01FF) /*&& (addr>= 0x01000000) && (addr<= 0x01FFFFFF)*/) //	:Ext. Device
	{
	//	EMUERROR2("Read from area0_32 not implemented [Ext. Device], addr=%x",addr);
		return (T)libExtDevice_ReadMem_A0_010(addr,sz);
	}
	//rest of it ;P
	/*else 
	{
		EMUERROR2("Read from area0_32 not mapped!!! , addr=%x",addr);
	}*/
	return 0;
}

template<u32 sz, class T, u32 b_start,u32 b_end>
void  FASTCALL WriteMem_area0(u32 addr,T data)
{
	addr &= 0x01FFFFFF;//to get rid of non needed bits

	const u32 base_start=addr>>16;//b_start & 0x01FF;
	const u32 base_end=addr>>16;//b_end & 0x01FF;

	//map 0x0000 to 0x001F
	if ((base_start >=0x0000) && (base_end <=0x001F) /*&& (addr<=0x001FFFFF)*/)//	:MPX	System/Boot ROM
	{
		//WriteMemFromPtrRet(bootfile,adr,sz);
		EMUERROR4("Write to  [MPX	System/Boot ROM] is not possible, addr=%x,data=%x,size=%d",addr,data,sz);
	}
	//map 0x0020 to 0x0021
	else if ((base_start >=0x0020) && (base_end <=0x0021) /*&& (addr>= 0x00200000) && (addr<= 0x0021FFFF)*/)		//	:Flash Memory
	{
		WriteMemArrRet(flash_b,addr-0x00200000,data,sz);
		EMUERROR4("Write to [Flash Memory] , sz?!, addr=%x,data=%x,size=%d",addr,data,sz);
	}
	//map 0x0040 to 0x005F -> actualy , i'l olny map 0x005F to 0x005F , b/c the rest of it is unpammed (left to defalt handler)
	//map 0x005F to 0x005F
	else if ((base_start >=0x005F) && (base_end <=0x005F) /*&& (addr>= 0x00400000) */&& (addr<= 0x005F67FF))		//	:Unassigned
	{
		EMUERROR4("Write to area0_32 not implemented [Unassigned], addr=%x,data=%x,size=%d",addr,data,sz);
	}/*
	else if ((base_start >=0x005F) && (base_end <=0x005F) && (addr>= 0x005F6800) && (addr<= 0x005F69FF))		//	:System Control Reg.
	{
		//EMUERROR4("Write to area0_32 not implemented [System Control Reg], addr=%x,data=%x,size=%d",addr,data,sz);
		sb_WriteMem(addr,data,sz);
		return;
	}
	else if ((base_start >=0x005F) && (base_end <=0x005F) && (addr>= 0x005F6C00) && (addr<= 0x005F6CFF)) //	:Maple i/f Control Reg.
	{
		//EMUERROR4("Write to area0_32 not implemented [Maple i/f Control Reg], addr=%x,data=%x,size=%d",addr,data,sz);
		sb_WriteMem(addr,data,sz);
	}
	else if ((base_start >=0x005F) && (base_end <=0x005F) && (addr>= 0x005F7400) && (addr<=0x005F74FF)) //	:G1 i/f Control Reg.
	{
		//EMUERROR4("Write to area0_32 not implemented [G1 i/f Control Reg], addr=%x,data=%x,size=%d",addr,data,sz);
		sb_WriteMem(addr,data,sz);
	}
	else if ((base_start >=0x005F) && (base_end <=0x005F) && (addr>= 0x005F7800) && (addr<=0x005F78FF)) //	:G2 i/f Control Reg.
	{
		//EMUERROR4("Write to area0_32 not implemented [G2 i/f Control Reg], addr=%x,data=%x,size=%d",addr,data,sz);
		sb_WriteMem(addr,data,sz);
	}*/
	else if ((base_start >=0x005F) && (base_end <=0x005F) && (addr>= 0x005F7000) && (addr<= 0x005F70FF)) //	:GD-ROM
	{
		//EMUERROR4("Write to area0_32 not implemented [GD-ROM], addr=%x,data=%x,size=%d",addr,data,sz);
		WriteMem_gdrom(addr,data,sz);
	}
	else if ((base_start >=0x005F) && (base_end <=0x005F) && (addr>= 0x005F6800) && (addr<=0x005F7CFF)) //	/*:PVR i/f Control Reg.*/ -> ALL SB registers
	{
		//EMUERROR4("Write to area0_32 not implemented [PVR i/f Control Reg], addr=%x,data=%x,size=%d",addr,data,sz);
		sb_WriteMem(addr,data,sz);
	}
	else if ((base_start >=0x005F) && (base_end <=0x005F) && (addr>= 0x005F8000) && (addr<=0x005F9FFF)) //	:TA / PVR Core Reg.
	{
		//EMUERROR4("Write to area0_32 not implemented [TA / PVR Core Reg], addr=%x,data=%x,size=%d",addr,data,sz);
		pvr_writereg_TA(addr,data,sz);
		return;
	}
	//map 0x0060 to 0x0060
	else if ((base_start >=0x0060) && (base_end <=0x0060) /*&& (addr>= 0x00600000)*/ && (addr<= 0x006007FF)) //	:MODEM
	{
		//EMUERROR4("Write to area0_32 not implemented [MODEM], addr=%x,data=%x,size=%d",addr,data,sz);
		libExtDevice_WriteMem_A0_006(addr,data,sz);
	}
	//map 0x0060 to 0x006F
	else if ((base_start >=0x0060) && (base_end <=0x006F) && (addr>= 0x00600800) && (addr<= 0x006FFFFF)) //	:G2 (Reserved)
	{
		EMUERROR4("Write to area0_32 not implemented [G2 (Reserved)], addr=%x,data=%x,size=%d",addr,data,sz);
	}
	//map 0x0070 to 0x0070
	else if ((base_start >=0x0070) && (base_end <=0x0070) /*&& (addr>= 0x00700000)*/ && (addr<=0x00707FFF)) //	:AICA- Sound Cntr. Reg.
	{
		//EMUERROR4("Write to area0_32 not implemented [AICA- Sound Cntr. Reg], addr=%x,data=%x,size=%d",addr,data,sz);
		//aica_writereg(addr,data,sz);
		//libAICA_WriteMem_aica_reg(addr,data,sz);
		WriteMem_aica_reg(addr,data,sz);
		return;
	}
	//map 0x0071 to 0x0071
	else if ((base_start >=0x0071) && (base_end <=0x0071) /*&& (addr>= 0x00710000)*/ && (addr<= 0x0071000B)) //	:AICA- RTC Cntr. Reg.
	{
		//EMUERROR4("Write to area0_32 not implemented [AICA- RTC Cntr. Reg], addr=%x,data=%x,size=%d",addr,data,sz);
		WriteMem_aica_rtc(addr,data,sz);
		return;
	}
	//map 0x0080 to 0x00FF
	else if ((base_start >=0x0080) && (base_end <=0x00FF) /*&& (addr>= 0x00800000) && (addr<=0x00FFFFFF)*/) //	:AICA- Wave Memory
	{
		//EMUERROR4("Write to area0_32 not implemented [AICA- Wave Memory], addr=%x,data=%x,size=%d",addr,data,sz);
		//aica_writeram(addr,data,sz);
		libAICA_WriteMem_aica_ram(addr,data,sz);
		return;
	}
	//map 0x0100 to 0x01FF
	else if ((base_start >=0x0100) && (base_end <=0x01FF) /*&& (addr>= 0x01000000) && (addr<= 0x01FFFFFF)*/) //	:Ext. Device
	{
		//EMUERROR4("Write to area0_32 not implemented [Ext. Device], addr=%x,data=%x,size=%d",addr,data,sz);
		libExtDevice_WriteMem_A0_010(addr,data,sz);
	}
	/*else
	{
		EMUERROR4("Write to area0_32 not mapped!!! , addr=%x,data=%x,size=%d",addr,data,sz);
	}*/
	return;
}

//Init/Res/Term
void sh4_area0_Init()
{
	sb_Init();
}

void sh4_area0_Reset(bool Manual)
{
	sb_Reset(Manual);
}

void sh4_area0_Term()
{
	sb_Term();
}


//AREA 0
_vmem_handler area0_handler_0000_00FF;
_vmem_handler area0_handler_0100_01FF;


//Different mem mapping regions for area0
//0x0000-0x001f
//0x0020-0x0021
//0x005F-0x005F
//0x0060-0x0060
//0x0061-0x006F
//0x0070-0x0070
//0x0071-0x0071
//0x0080-0x00FF
//0x0100-0x01FF
//Count : 9

void map_area0_init()
{
	//area0_handler =	_vmem_register_handler(ReadMem8_area0,ReadMem16_area0,ReadMem32_area0,
	//									  WriteMem8_area0,WriteMem16_area0,WriteMem32_area0);
	
	area0_handler_0000_00FF = _vmem_register_handler_Template2(ReadMem_area0,WriteMem_area0,0x0000,0x00FF);
	/*
	area0_handler_20_21 = _vmem_register_handler_Template2(ReadMem_area0,WriteMem_area0,0x0020,0x0021);
	area0_handler_5F_5F = _vmem_register_handler_Template2(ReadMem_area0,WriteMem_area0,0x005F,0x005F);
	area0_handler_60_60 = _vmem_register_handler_Template2(ReadMem_area0,WriteMem_area0,0x0060,0x0060);
	area0_handler_61_6F = _vmem_register_handler_Template2(ReadMem_area0,WriteMem_area0,0x0061,0x006F);
	area0_handler_70_70 = _vmem_register_handler_Template2(ReadMem_area0,WriteMem_area0,0x0070,0x0070);
	area0_handler_71_71 = _vmem_register_handler_Template2(ReadMem_area0,WriteMem_area0,0x0071,0x0071);
	//area0_handler_80_FF = _vmem_register_handler_Template2(ReadMem_area0,WriteMem_area0,0x0080,0x00FF);
	*/
	area0_handler_0100_01FF = _vmem_register_handler_Template2(ReadMem_area0,WriteMem_area0,0x0100,0x01FF);
}
void map_area0(u32 base)
{
	verify(base<0xE000);

	//Map 0x0000 to 0x01FF
	//u32 start=0x0000 | base;
	//u32 end  =start+0x01FF;
	
	//_vmem_map_handler(area0_handler,start,end);
	//0x0000-0x001f
	_vmem_map_handler(area0_handler_0000_00FF,0x00|base,0x00|base);
	/*
	//0x0020-0x0021
	_vmem_map_handler(area0_handler_20_21,0x0020|base,0x0021|base);
	//0x005F-0x005F
	_vmem_map_handler(area0_handler_5F_5F,0x005F|base,0x005F|base);
	//0x0060-0x0060
	_vmem_map_handler(area0_handler_60_60,0x0060|base,0x0060|base);
	//0x0061-0x006F
	_vmem_map_handler(area0_handler_61_6F,0x0061|base,0x006F|base);
	//0x0070-0x0070
	_vmem_map_handler(area0_handler_70_70,0x0070|base,0x0070|base);
	//0x0071-0x0071
	_vmem_map_handler(area0_handler_71_71,0x0071|base,0x0071|base);
	//0x0080-0x00FF
	//_vmem_map_handler(area0_handler_80_FF,0x0080|base,0x00FF|base);
	
	_vmem_map_block_mirror(aica_ram.data,0x0080|base,0x00FF|base,ARAM_SIZE);
	*/

	//0x0100-0x01FF
	_vmem_map_handler(area0_handler_0100_01FF,0x01|base,0x01|base);

	//0x0240 to 0x03FF mirrors 0x0040 to 0x01FF (no flashrom or bios)
	//0x0200 to 0x023F are unused
	_vmem_mirror_mapping(0x02|base,0x00|base,0x02);
}

