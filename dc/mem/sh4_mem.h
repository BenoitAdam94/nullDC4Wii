#pragma once
#include "types.h"

//main system mem
extern VArray2 mem_b;

//bios rom
extern Array<u8> bios_b;

//flash rom
extern Array<u8> flash_b;

#define MEMCALL FASTCALL

#include "_vmem.h"

#define ReadMem8 _vmem_ReadMem8
#define ReadMem16 _vmem_ReadMem16
#define IReadMem16 ReadMem16
#define ReadMem32 _vmem_ReadMem32
#define ReadMem64 _vmem_ReadMem64

#define WriteMem8 _vmem_WriteMem8
#define WriteMem16 _vmem_WriteMem16
#define WriteMem32 _vmem_WriteMem32
#define WriteMem64 _vmem_WriteMem64

#define ReadMem8_nommu _vmem_ReadMem8
#define ReadMem16_nommu _vmem_ReadMem16
#define IReadMem16_nommu _vmem_IReadMem16
#define ReadMem32_nommu _vmem_ReadMem32


#define WriteMem8_nommu _vmem_WriteMem8
#define WriteMem16_nommu _vmem_WriteMem16
#define WriteMem32_nommu _vmem_WriteMem32

void MEMCALL WriteMemBlock_ptr(u32 dst,u32* src,u32 size);
void MEMCALL WriteMemBlock_nommu_ptr(u32 dst,u32* src,u32 size);
void MEMCALL WriteMemBlock_nommu_dma(u32 dst,u32 src,u32 size);
//Init/Res/Term
void mem_Init();
void mem_Term();
void mem_Reset(bool Manual);
void mem_map_defualt();

//Generic read/write functions for debugger
bool ReadMem_DB(u32 addr,u32& data,u32 size );
bool WriteMem_DB(u32 addr,u32 data,u32 size );

//Get pointer to ram area , 0 if error
//For debugger(gdb) - dynarec
u8* GetMemPtr(u32 Addr,u32 size);

//Get infomation about an area , eg ram /size /anything
//For dynarec - needs to be done
struct MemInfo
{
	//MemType:
	//Direct ptr   , just read/write to the ptr
	//Direct call  , just call for read , ecx=data on write (no address)
	//Generic call , ecx=addr , call for read , edx=data for write
	u32 MemType;		
	
	//todo
	u32 Flags;

	void* read_ptr;
	void* write_ptr;
};

void GetMemInfo(u32 addr,u32 size,MemInfo* meminfo);

bool IsOnRam(u32 addr);


u32 FASTCALL GetRamPageFromAddress(u32 RamAddress);

#define 	ReadMemArrRet(arr,addr,sz)				\
			{if (sz==1)								\
				return arr[addr];					\
			else if (sz==2)							\
				return HOST_TO_LE16(*(u16*)&arr[addr]);			\
			else if (sz==4)							\
				return HOST_TO_LE32(*(u32*)&arr[addr]);}	

#define WriteMemArrRet(arr,addr,data,sz)				\
			{if(sz==1)								\
				{arr[addr]=(u8)data;return;}				\
			else if (sz==2)							\
				{*(u16*)&arr[addr]=HOST_TO_LE16((u16)data);return;}		\
			else if (sz==4)							\
				{*(u32*)&arr[addr]=HOST_TO_LE32(data);return;}}

