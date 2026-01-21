//new memory mapping code ..."_vmem" ... Don't ask where i got the name , it somehow poped on my head :p
//
/*
_vmem v2 :

physical map :
	_vmem : generic functions
	dvmem : direct access (using memory mapping)
	nvmem : native acc (dyn/etc)

Translated map:
	tvmem : generic function, may use the exception mechanism
	dbg

*/
#include "_vmem.h"
#include "dc/aica/aica_if.h"

#define HANDLER_MAX 0x1F
#define HANDLER_COUNT (HANDLER_MAX+1)
//top registed handler
_vmem_handler			_vmem_lrp;

//handler tables
_vmem_ReadMem8FP*		_vmem_RF8[HANDLER_COUNT];
_vmem_WriteMem8FP*		_vmem_WF8[HANDLER_COUNT];

_vmem_ReadMem16FP*		_vmem_RF16[HANDLER_COUNT];
_vmem_WriteMem16FP*		_vmem_WF16[HANDLER_COUNT];

_vmem_ReadMem32FP*		_vmem_RF32[HANDLER_COUNT];
_vmem_WriteMem32FP*		_vmem_WF32[HANDLER_COUNT];

//upper 8b of the address
void* _vmem_MemInfo_ptr[0x100];

/*
	ext   tmp,addr[31:29];//Region			1
	ext   tmp2,addr[28:26];//Area			2
	lui	  tmp3,0;
	seqi tmp

	//RAM :
	ext   tmp,addr[31:29];//Region			1
	ext   tmp2,addr[28:26];//Area			2
	teqi  tmp,7;//not in P4 !				3
	tneqi tmp2,3;//In area 3 ?				4

	lui  tmp3,0x0A000 0000		//ram base	5
	ins  tmp3[23:0],tmp2[23:0]	//combine	6

	SW/SH/SB LW/LH/LB dst,tmp3				7

	//VRAM : same as above, but checks for area 1

	//SQ
	ext   tmp,addr[31:26];//RegionArea      1
	tneqi tmp,0x38;	//0xE0/E1/E2/E3			2

	lui  tmp2,0xXXXX 0000		//SQ ptr	3
	ori  tmp2,0xXXXX			//SQ ptr    4	<can be skipped if SQ is 64KB aligned>
	andi tmp3,addr,0x3F;		//or ext	5
	add  tmp3,tmp2,tmp3			//combine	6	<can be skiped if SQ is at 64 byte aligned>

	SW/SH/SB LW/LH/LB dst,tmp3				7

	//Full lookup
	ext tmp1,addr[31:22]				//1
	ins _vmem_info_base[9:2],tmp1[7:0]	//2 -> this corrupts _vmem_info_base, i have to restore it
	LW  tmp2,tmp1						//3
	[if write]
	add rdata,0,src;					//7

	SLLV tmp3,addr,tmp2;				//4
	J REST_<OP>;						//5
	SRLV tmp3,tmp3,tmp2;				//6
	[if read]
	add dst,0,rv;						//7


	REST_<OP>_RAM
	LUI _vmem_info_base,0xXXXX 0000;	//Restore ptr
	ORI _vmem_info_base,0x0000 XXXX;	//can be skiped if 64 kb aligned

	ins tmp2[4:0],0
	add paddr,tmp3,tmp2
	jr ra
	SW/SH/SB LW/LH/LB dst,tmp3

	//its possible to have a handler for regs only, too
	REST_<OP>_FULL //this is faster for ram acc. can be made to help reg reads instead
	LUI _vmem_info_base,0xXXXX 0000;	//Restore ptr
	ORI _vmem_info_base,0x0000 XXXX;	//can be skiped if 64 kb aligned
	bltz paddr,REGLOOKUP
	ins tmp2[4:0],0
	add paddr,tmp3,tmp2
	jr ra
	SW/SH/SB LW/LH/LB dst,tmp3

	REGLOOKUP:
	jump handlers+(paddr&0x7fff);//or whatever


	//Full lookup (WIP, not working)
	ext tmp1,addr[31:22]				//1
	shl tmp1,2							//2
	add tmp1,tmp1,_vmem_info_base		//3
	LW  tmp2,tmp1						//4
	SLLV tmp3,addr,tmp2;				//5
	J REST_<OP>:						//6
	SRLV tmp3,tmp3,tmp2;				//7

	and tmp4,addr,tmp3



	REST_<OP>
*/

void _vmem_get_ptrs(u32 sz,bool write,void*** vmap,void*** func)
{
	*vmap=_vmem_MemInfo_ptr;
	switch(sz)
	{
	case 1:
		*func=write?(void**)_vmem_WF8:(void**)_vmem_RF8;
		return;

	case 2:
		*func=write?(void**)_vmem_WF16:(void**)_vmem_RF16;
		return;

	case 4:
	case 8:
		*func=write?(void**)_vmem_WF32:(void**)_vmem_RF32;
		return;

	default:
		die("invalid size");
	}
}
void* _vmem_read_const(u32 addr,bool& ismem,u32 sz)
{
	u32   page=addr>>24;
	u32   iirf=(unat)_vmem_MemInfo_ptr[page];
	void* ptr=(void*)(iirf&~HANDLER_MAX);

	if (ptr==0)
	{
		ismem=false;
		const u32 id=iirf;
		if (sz==1)
		{
			return (void*)_vmem_RF8[id/4];
		}
		else if (sz==2)
		{
			return (void*)_vmem_RF16[id/4];
		}
		else if (sz==4)
		{
			return (void*)_vmem_RF32[id/4];
		}
		else
			die("Invalid size");
	}
	else
	{
		ismem=true;
		iirf &= HANDLER_MAX;
		addr<<=iirf;
		addr>>=iirf;
		#if HOST_ENDIAN==ENDIAN_BIG
		if (sz<4)
			addr^=4-sz;
		#endif
		return &(((u8*)ptr)[addr]);
	}
	die("Invalid mem sz");
}
template<typename T>
INLINE T fastcall _vmem_readt(u32 addr)
{
	const u32 sz=sizeof(T);

	u32   page=addr>>24;	//1 op, shift/extract
	unat  iirf=(unat)_vmem_MemInfo_ptr[page];	//2 ops, insert + read [vmem table will be on reg ]
	void* ptr=(void*)(iirf&~HANDLER_MAX);	//2 ops, and // 1 op insert
	//u32   mask=(u32)0xFFFFFFFF>>iirf;//2 ops, load -1 and shift
	//1 op for the mask
	//1 op for the add
	//1 op for ret
	//1 op for ram read (dslot)
	if (ptr==0)
	{
		const u32 id=iirf;
		if (sz==1)
		{
			return _vmem_RF8[id/4](addr);
		}
		else if (sz==2)
		{
			return _vmem_RF16[id/4](addr);
		}
		else if (sz==4)
		{
			return _vmem_RF32[id/4](addr);
		}
		else if (sz==8)
		{
			T rv=_vmem_RF32[id/4](addr);
			rv|=(T)((u64)_vmem_RF32[id/4](addr+4)<<32);
			
			return rv;
		}
		else
			die("Invalid size");
	}
	else
	{
		iirf &= HANDLER_MAX;
		addr<<=iirf;
		addr>>=iirf;
		#if HOST_ENDIAN==ENDIAN_BIG
		if (sz<4)
			addr^=4-sz;
		#endif
		T data=(*((T*)&(((u8*)ptr)[addr])));
		return data;
	}
}
template<typename T>
INLINE void fastcall _vmem_writet(u32 addr,T data)
{
	const u32 sz=sizeof(T);

	u32 page=addr>>24;
	unat  iirf=(unat)_vmem_MemInfo_ptr[page];
	void* ptr=(void*)(iirf&~HANDLER_MAX);
	//u32   mask=(u32)0xFFFFFFFF>>iirf;

	if (ptr==0)
	{
		const u32 id=iirf;
		if (sz==1)
		{
			 _vmem_WF8[id/4](addr,data);
		}
		else if (sz==2)
		{
			 _vmem_WF16[id/4](addr,data);
		}
		else if (sz==4)
		{
			 _vmem_WF32[id/4](addr,data);
		}
		else if (sz==8)
		{
			_vmem_WF32[id/4](addr,(u32)data);
			_vmem_WF32[id/4](addr+4,(u32)((u64)data>>32));
		}
		else
			die("Invalid size");
	}
	else
	{
		iirf &= HANDLER_MAX;
		addr<<=iirf;
		addr>>=iirf;
#if HOST_ENDIAN==ENDIAN_BIG
		if (sz<4)
			addr^=4-sz;
#endif
		*((T*)&(((u8*)ptr)[addr]))=data;
	}
}

//ReadMem/WriteMem functions
//ReadMem
u8 fastcall _vmem_ReadMem8(u32 Address)
{
	return _vmem_readt<u8>(Address);
}

u16 fastcall _vmem_ReadMem16(u32 Address)
{
	return _vmem_readt<u16>(Address);
}

u32 fastcall _vmem_ReadMem32(u32 Address)
{
	return _vmem_readt<u32>(Address);
}
u64 fastcall _vmem_ReadMem64(u32 Address)
{
	return _vmem_readt<u64>(Address);
}

//WriteMem
void fastcall _vmem_WriteMem8(u32 Address,u8 data)
{
	_vmem_writet<u8>(Address,data);
}

void fastcall _vmem_WriteMem16(u32 Address,u16 data)
{
	_vmem_writet<u16>(Address,data);
}
void fastcall _vmem_WriteMem32(u32 Address,u32 data)
{
	_vmem_writet<u32>(Address,data);
}
void fastcall _vmem_WriteMem64(u32 Address,u64 data)
{
	_vmem_writet<u64>(Address,data);
}

//0xDEADC0D3 or 0
#define MEM_ERROR_RETURN_VALUE 0xDEADC0D3

//phew .. that was lota asm code ;) lets go back to C :D
//default mem handlers ;)
//defualt read handlers
u8 fastcall _vmem_ReadMem8_not_mapped(u32 addresss)
{
	printf("[sh4]Read8 from 0x%X, not mapped [_vmem default handler]\n",addresss);
	return (u8)MEM_ERROR_RETURN_VALUE;
}
u16 fastcall _vmem_ReadMem16_not_mapped(u32 addresss)
{
	printf("[sh4]Read16 from 0x%X, not mapped [_vmem default handler]\n",addresss);
	return (u16)MEM_ERROR_RETURN_VALUE;
}
u32 fastcall _vmem_ReadMem32_not_mapped(u32 addresss)
{
	printf("[sh4]Read32 from 0x%X, not mapped [_vmem default handler]\n",addresss);
	return (u32)MEM_ERROR_RETURN_VALUE;
}
//defualt write handers
void fastcall _vmem_WriteMem8_not_mapped(u32 addresss,u8 data)
{
	printf("[sh4]Write8 to 0x%X=0x%X, not mapped [_vmem default handler]\n",addresss,data);
}
void fastcall _vmem_WriteMem16_not_mapped(u32 addresss,u16 data)
{
	printf("[sh4]Write16 to 0x%X=0x%X, not mapped [_vmem default handler]\n",addresss,data);
}
void fastcall _vmem_WriteMem32_not_mapped(u32 addresss,u32 data)
{
	printf("[sh4]Write32 to 0x%X=0x%X, not mapped [_vmem default handler]\n",addresss,data);
}
//code to register handlers
//0 is considered error :)
_vmem_handler _vmem_register_handler(
									 _vmem_ReadMem8FP* read8,
									 _vmem_ReadMem16FP* read16,
									 _vmem_ReadMem32FP* read32,

									 _vmem_WriteMem8FP* write8,
									 _vmem_WriteMem16FP* write16,
									 _vmem_WriteMem32FP* write32
									 )
{
	_vmem_handler rv=_vmem_lrp++;

	verify(rv<HANDLER_COUNT);

	_vmem_RF8[rv] =read8==0  ?	_vmem_ReadMem8_not_mapped  :	read8;
	_vmem_RF16[rv]=read16==0 ?	_vmem_ReadMem16_not_mapped :	read16;
	_vmem_RF32[rv]=read32==0 ?	_vmem_ReadMem32_not_mapped :	read32;

	_vmem_WF8[rv] =write8==0 ?	_vmem_WriteMem8_not_mapped :	write8;
	_vmem_WF16[rv]=write16==0?	_vmem_WriteMem16_not_mapped:	write16;
	_vmem_WF32[rv]=write32==0?	_vmem_WriteMem32_not_mapped:	write32;

	return rv;
}
u32 FindMask(u32 msk)
{
	u32 s=-1;
	u32 rv=0;
	while(msk!=s>>rv)
		rv++;
	return rv;
}
//map a registed handler to a mem region :)
void _vmem_map_handler(_vmem_handler Handler,u32 start,u32 end)
{
	verify(start<0x100);
	verify(end<0x100);
	verify(start<=end);
	for (u32 i=start;i<=end;i++)
	{
		_vmem_MemInfo_ptr[i]=((u8*)0)+(0x00000000 + Handler*4);
		//_vmem_MemInfo_mask(i)=(Handler*4);
	}
}
//map a memory block to a mem region :)
void _vmem_map_block(void* base,u32 start,u32 end,u32 mask)
{
	verify(start<0x100);
	verify(end<0x100);
	verify(start<=end);
	verify((0xFF & (unat)base)==0);
	verify(base!=0);
	u32 j=0;
	for (u32 i=start;i<=end;i++)
	{
		_vmem_MemInfo_ptr[i]=&(((u8*)base)[j]) + FindMask(mask);
		//_vmem_MemInfo_mask(i)=mask;
		j+=0x1000000;
	}
}
void _vmem_mirror_mapping(u32 new_region,u32 start,u32 size)
{
	u32 end=start+size-1;
	verify(start<0x100);
	verify(end<0x100);
	verify(start<=end);
	verify(!((start>=new_region) && (end<=new_region)));

	u32 j=new_region;
	for (u32 i=start;i<=end;i++)
	{
		_vmem_MemInfo_ptr[j&0xFF]=_vmem_MemInfo_ptr[i&0xFF];
		//_vmem_MemInfo_mask(j&0xFF)=_vmem_MemInfo_mask(i&0xFF);
		j++;
	}
}
//init/reset/term
void _vmem_init()
{
	_vmem_reset();
}

void _vmem_reset()
{
	//clear read tables
	memset(_vmem_RF8,0,sizeof(_vmem_RF8));
	memset(_vmem_RF16,0,sizeof(_vmem_RF16));
	memset(_vmem_RF32,0,sizeof(_vmem_RF32));
	//clear write tables
	memset(_vmem_WF8,0,sizeof(_vmem_WF8));
	memset(_vmem_WF16,0,sizeof(_vmem_WF16));
	memset(_vmem_WF32,0,sizeof(_vmem_WF32));
	//clear meminfo table
	memset(_vmem_MemInfo_ptr,0,sizeof(_vmem_MemInfo_ptr));
	//memset(_vmem_MemInfo_mask,0,sizeof(_vmem_MemInfo_mask));

	//reset registation index
	_vmem_lrp=0;

	//register default functions (0) for slot 0 :)
	verify(_vmem_register_handler(0,0,0,0,0,0)==0);
}

void _vmem_term()
{

}

#include "dc/pvr/pvr_if.h"
#include "sh4_mem.h"

/*
	A0
	A8
	B0
	B8

	pspvmem
	16 mb map chunks
	direct maps : RAM(16 mb),VRAM(8mb, mirror),SQ(64 bytes)
	ptr can be, 0, A, B

	u32 mask=-1;					//1
	u32 lookup=&lut[0];				//1
	u32 idx=sh4_adr>>24;			//1
	u32 t=lookup[idx];				//1

	if (t>0xDF)						//1
		goto special_handle;		//x
	mask>>=t&31;	 //4 bits, dslot, 1

	u32 addr=sh4_adr&mask;			//1

	special_handle:
	t<<=2;
	funct=rio[t];
	goto funct

	u32 lookup=&lut[0];				//2
	u32 idx=sh4_adr>>24;			//1
	idx=idx<<2;						//1
	mask=lookup[idx];				//1
	bltz(mask,special)				//1
	ptr=lookup[idx+1024];			//1
	addr=mask&sh4_addr;				//1
	ptr=addr+ptr;					//1
	rv=[ptr]						//1
*/
#if HOST_OS==OS_PSP
#define SLIM_RAM ((u8*)0x0A000000)
//0x0A00 0000 -> RAM	(directly mapped)
//0x0B00 0000 -> VRAM	(directly mapped)
//0x0B80 0000 -> ARAM	(soft map)
#elif HOST_OS!=OS_WII
ALIGN(256) u8 SLIM_RAM[ARAM_SIZE+VRAM_SIZE+RAM_SIZE];
#endif
bool _vmem_reserve()
{
#if HOST_OS == OS_WII
	u32 level=IRQ_Disable();
	u8* ram_alloc=(u8*)SYS_GetArena2Lo();
	//align to 256 bytes ...
	ram_alloc+=255;
	ram_alloc=(u8*)(((unat)ram_alloc)&~255);
	SYS_SetArena2Lo(ram_alloc+ARAM_SIZE+VRAM_SIZE+RAM_SIZE);

	extern u8* vram_buffer;
	vram_buffer=(u8*)SYS_GetArena2Lo();
	vram_buffer+=64;

	SYS_SetArena2Lo(vram_buffer+(VRAM_SIZE*2));

	IRQ_Restore(level);
	printf("Wii ram ptr: %08X, vram buffer: %08X -- GDDR3: %.2f MB free \n",ram_alloc,vram_buffer,((unat)SYS_GetArena2Hi()-(unat)SYS_GetArena2Lo())/1024.f/1024);
#else
	u8* ram_alloc=SLIM_RAM;
#endif
	

	aica_ram.size=ARAM_SIZE;
	aica_ram.data=ram_alloc;
	ram_alloc+=ARAM_SIZE;
	
	vram.size=VRAM_SIZE;
	vram.data=ram_alloc;
	ram_alloc+=VRAM_SIZE;

	mem_b.size=RAM_SIZE;
	mem_b.data=ram_alloc;
	ram_alloc+=RAM_SIZE;

	return true;
}
void _vmem_release()
{
	//TODO
}