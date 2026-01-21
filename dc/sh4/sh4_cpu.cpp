/*
	This file could really benefit from some cleanup
	Also, maby it should be shared with mainline ?
*/

//All non fpu opcodes :)
#include "types.h"


#include "dc/pvr/pvr_if.h"
#include "sh4_interpreter.h"
#include "dc/mem/sh4_mem.h"
#include "dc/mem/sh4_internal_reg.h"
#include "sh4_registers.h"
#include "sh4_interpreter.h"
#include "ccn.h"
#include "intc.h"
#include "tmu.h"
#include "dc/gdrom/gdrom_if.h"



#define GetN(str) ((str>>8) & 0xf)
#define GetM(str) ((str>>4) & 0xf)
#define GetImm4(str) ((str>>0) & 0xf)
#define GetImm8(str) ((str>>0) & 0xff)
#define GetSImm8(str) ((s8)((str>>0) & 0xff))
#define GetImm12(str) ((str>>0) & 0xfff)
#define GetSImm12(str) (((s16)((GetImm12(str))<<4))>>4)

#define iNimp cpu_iNimp
#define iWarn cpu_iWarn

//Read Mem macros

#define ReadMemU32(to,addr) to=ReadMem32(addr)
#define ReadMemS32(to,addr) to=(s32)ReadMem32(addr)
#define ReadMemS16(to,addr) to=(u32)(s32)(s16)ReadMem16(addr)
#define ReadMemS8(to,addr) to=(u32)(s32)(s8)ReadMem8(addr)

//Base,offset format
#define ReadMemBOU32(to,addr,offset)	ReadMemU32(to,addr+offset)
#define ReadMemBOS16(to,addr,offset)	ReadMemS16(to,addr+offset)
#define ReadMemBOS8(to,addr,offset)		ReadMemS8(to,addr+offset)

//Write Mem Macros
#define WriteMemU32(addr,data)				WriteMem32(addr,(u32)data)
#define WriteMemU16(addr,data)				WriteMem16(addr,(u16)data)
#define WriteMemU8(addr,data)				WriteMem8(addr,(u8)data)

//Base,offset format
#define WriteMemBOU32(addr,offset,data)		WriteMemU32(addr+offset,data)
#define WriteMemBOU16(addr,offset,data)		WriteMemU16(addr+offset,data)
#define WriteMemBOU8(addr,offset,data)		WriteMemU8(addr+offset,data)


bool sh4_sleeping;

// 0xxx

void cpu_iNimp(u32 op, const char* info)
{
	printf("not implemented opcode : %X : ", op);
	printf(info);

	//sh4_cpu.Stop();
}

void cpu_iWarn(u32 op, const char* info)
{
	printf("Check opcode : %X : ", op);
	printf(info);
	printf(" @ %X\n", curr_pc);
}

#include "sh4_cpu_movs.h"
#include "sh4_cpu_branch.h"
#include "sh4_cpu_arith.h"
#include "sh4_cpu_logic.h"
#include "dc/mem/mmu.h"

//************************ TLB/Cache ************************
//ldtlb
sh4op(i0000_0000_0011_1000)
{
	printf("ldtlb %d/%d\n",CCN_MMUCR.URC,CCN_MMUCR.URB);
	UTLB[CCN_MMUCR.URC].Data=CCN_PTEL;
	UTLB[CCN_MMUCR.URC].Address=CCN_PTEH;

	UTLB_Sync(CCN_MMUCR.URC);
}

//ocbi @<REG_N>
sh4op(i0000_nnnn_1001_0011)
{
	u32 n = GetN(op);
	//printf("ocbi @0x%08X \n",r[n]);
}

//ocbp @<REG_N>
sh4op(i0000_nnnn_1010_0011)
{
	u32 n = GetN(op);
	//printf("ocbp @0x%08X \n",r[n]);
}

//ocbwb @<REG_N>
sh4op(i0000_nnnn_1011_0011)
{
	u32 n = GetN(op);
	//printf("ocbwb @0x%08X \n",r[n]);
}

//pref @<REG_N>
template<bool mmu_on>
INLINE void FASTCALL do_sqw(u32 Dest)
{
	//TODO : Check for enabled store queues ?
	u32* sq;
	u32 Address;
	sq = (u32*)&sq_both[Dest& 0x20];

	//Translate the SQ addresses as needed
	if (mmu_on)
	{
		Address=mmu_TranslateSQW(Dest&0xFFFFFFE0);
	}
	else
	{
		u32 QACR;
		if ((Dest& 0x20)==0)
			QACR = CCN_QACR0.Area;
		else
			QACR = CCN_QACR1.Area;

		Address = (Dest & 0x03FFFFE0) | (QACR << 26);
	}

	if (((Address >> 26) & 0x7) == 4)//Area 4
	{
		TAWriteSQ(Address,sq);
	}
	else
	{
		WriteMemBlock_nommu_ptr(Address,sq,8*4);
	}
}

void FASTCALL do_sqw_mmu(u32 dst) { do_sqw<true>(dst); }
void FASTCALL do_sqw_nommu(u32 dst) { do_sqw<false>(dst); }

sh4op(i0000_nnnn_1000_0011)
{
	u32 n = GetN(op);
	u32 Dest = r[n];

	if ((Dest>>26) == 0x38) //Store Queue
	{
		if (CCN_MMUCR.AT)
			do_sqw<true>(Dest);
		else
			do_sqw<false>(Dest);
	}
}


//************************ Set/Get T/S ************************
//sets
sh4op(i0000_0000_0101_1000)
{
	//iNimp("sets");
	sr.S = 1;
}

//clrs
sh4op(i0000_0000_0100_1000)
{
	//iNimp(op, "clrs");
	sr.S = 0;
}

//sett
sh4op(i0000_0000_0001_1000)
{
	//iNimp("sett");
	sr.T = 1;
}

//clrt
sh4op(i0000_0000_0000_1000)
{
	//iNimp("clrt");
	sr.T = 0;
}

//movt <REG_N>
sh4op(i0000_nnnn_0010_1001)
{
	//iNimp("movt <REG_N>");
	u32 n = GetN(op);
	r[n] = !!sr.T;
}

//************************ Reg Compares ************************
//cmp/pz <REG_N>
sh4op(i0100_nnnn_0001_0001)
{//ToDo : Check This [26/4/05]
	//iNimp("cmp/pz <REG_N>");
	u32 n = GetN(op);

	if (((s32)r[n]) >= 0)
		sr.T = 1;
	else
		sr.T = 0;
}

//cmp/pl <REG_N>
sh4op(i0100_nnnn_0001_0101)
{
	//iNimp("cmp/pl <REG_N>");
	u32 n = GetN(op);
	if ((s32)r[n] > 0)
		sr.T = 1;
	else
		sr.T = 0;
}

//cmp/eq #<imm>,R0
sh4op(i1000_1000_iiii_iiii)
{
	u32 imm = (u32)(s32)(GetSImm8(op));
	if (r[0] == imm)
		sr.T =1;
	else
		sr.T =0;
}

//cmp/eq <REG_M>,<REG_N>
sh4op(i0011_nnnn_mmmm_0000)
{
	//iNimp("cmp/eq <REG_M>,<REG_N>");
	u32 n = GetN(op);
	u32 m = GetM(op);

	if (r[m] == r[n])
		sr.T = 1;
	else
		sr.T = 0;
}

//cmp/hs <REG_M>,<REG_N>
sh4op(i0011_nnnn_mmmm_0010)
{//ToDo : Check Me [26/4/05]
	u32 n = GetN(op);
	u32 m = GetM(op);
	if (r[n] >= r[m])
		sr.T=1;
	else
		sr.T=0;
}

//cmp/ge <REG_M>,<REG_N>
sh4op(i0011_nnnn_mmmm_0011)
{
	//iNimp("cmp/ge <REG_M>,<REG_N>");
	u32 n = GetN(op);
	u32 m = GetM(op);
	if ((s32)r[n] >= (s32)r[m])
		sr.T = 1;
	else
		sr.T = 0;
}

//cmp/hi <REG_M>,<REG_N>
sh4op(i0011_nnnn_mmmm_0110)
{
	u32 n = GetN(op);
	u32 m = GetM(op);

	if (r[n] > r[m])
		sr.T=1;
	else
		sr.T=0;
}

//cmp/gt <REG_M>,<REG_N>
sh4op(i0011_nnnn_mmmm_0111)
{
	//iNimp("cmp/gt <REG_M>,<REG_N>");
	u32 n = GetN(op);
	u32 m = GetM(op);

	if (((s32)r[n]) > ((s32)r[m]))
		sr.T = 1;
	else
		sr.T = 0;
}

//cmp/str <REG_M>,<REG_N>
sh4op(i0010_nnnn_mmmm_1100)
{
	//T -> 1 if -any- bytes are equal
	u32 n = GetN(op);
	u32 m = GetM(op);

	u32 temp;
	u32 HH, HL, LH, LL;

	temp = r[n] ^ r[m];

	HH=(temp&0xFF000000)>>24;
	HL=(temp&0x00FF0000)>>16;
	LH=(temp&0x0000FF00)>>8;
	LL=temp&0x000000FF;
	HH=HH&&HL&&LH&&LL;
	if (HH==0)
		sr.T=1;
	else
		sr.T=0;
}

//tst #<imm>,R0
sh4op(i1100_1000_iiii_iiii)
{
	//iNimp("tst #<imm>,R0");
	u32 utmp1 = r[0] & GetImm8(op);
	if (utmp1 == 0)
		sr.T = 1;
	else
		sr.T = 0;
}
//tst <REG_M>,<REG_N>
sh4op(i0010_nnnn_mmmm_1000)
{//ToDo : Check This [26/4/05]
	u32 n = GetN(op);
	u32 m = GetM(op);

	if ((r[n] & r[m])!=0)
		sr.T=0;
	else
		sr.T=1;

}
//************************ mulls! ************************
//mulu.w <REG_M>,<REG_N>
sh4op(i0010_nnnn_mmmm_1110)
{
	//iNimp("mulu.w <REG_M>,<REG_N>");//check  ++
	u32 n = GetN(op);
	u32 m = GetM(op);
	macl=((u16)r[n])*
		((u16)r[m]);
}

//muls.w <REG_M>,<REG_N>
sh4op(i0010_nnnn_mmmm_1111)
{
	//iNimp("muls <REG_M>,<REG_N>");
	u32 n = GetN(op);
	u32 m = GetM(op);

	macl = (u32)(((s16)(u16)r[n]) * ((s16)(u16)r[m]));
}
//dmulu.l <REG_M>,<REG_N>
sh4op(i0011_nnnn_mmmm_0101)
{
	//iNimp("dmulu.l <REG_M>,<REG_N>");
	u32 n = GetN(op);
	u32 m = GetM(op);
	u64 x;

	x = (u64)r[n] * (u64)r[m];

	macl = (u32)(x & 0xFFFFFFFF);
	mach = (u32)((x >> 32) & 0xFFFFFFFF);
}

//dmuls.l <REG_M>,<REG_N>
sh4op(i0011_nnnn_mmmm_1101)
{
	//iNimp("dmuls.l <REG_M>,<REG_N>");//check ++
	u32 n = GetN(op);
	u32 m = GetM(op);
	s64 x;

	x = (s64)(s32)r[n] * (s64)(s32)r[m];

	macl = (u32)(x & 0xFFFFFFFF);
	mach = (u32)((x >> 32) & 0xFFFFFFFF);
}


//mac.w @<REG_M>+,@<REG_N>+
sh4op(i0100_nnnn_mmmm_1111)
{
	u32 n = GetN(op);
	u32 m = GetM(op);
	if (sr.S!=0)
	{
		printf("mac.w @<REG_M>+,@<REG_N>+ : s=%d\n",!!sr.S);
	}
	else
	{
		s32 rm,rn;

		rn = (s32)(s16)ReadMem16(r[n]);
		r[n]+=2;
		//if (n==m)
		//{
		//	r[n]+=2;
		//	r[m]+=2;
		//}
		rm = (s32)(s16)ReadMem16(r[m]);

		r[m]+=2;

		s32 mul=rm * rn;
		s64 mac;// = (s64)(((u64)mach << 32) + macl);

		mac = macl;
		mac|= ((u64)mach << 32);

		mac+=mul;

		//macl = (u32)(mac & 0xFFFFFFFF);
		//mach = (u32)((mac >> 32) & 0xFFFFFFFF);
		macl = (u32)(mac);
		mach = (u32)(mac >> 32);
	}
}
//mac.l @<REG_M>+,@<REG_N>+
sh4op(i0000_nnnn_mmmm_1111)
{
	//iNimp("mac.l @<REG_M>+,@<REG_N>+");
	u32 n = GetN(op);
	u32 m = GetM(op);
	s32 rm, rn;
	s64 mul, mac, result;

	mac = macl;
	mac|= ((u64)mach << 32);

	verify(sr.S==0);

	ReadMemS32(rm,r[m]);
	r[m] += 4;
	ReadMemS32(rn,r[n]);
	r[n] += 4;

	mul = (s64)rm * (s64)rn;


	result = mac + mul;

	macl = (u32)(result);
	mach = (u32)(result >> 32);
	//printf("%I64u %I64u | %d %d | %d %d\n",mac,mul,macl,mach,rm,rn);
}

//mul.l <REG_M>,<REG_N>
sh4op(i0000_nnnn_mmmm_0111)
{
	u32 n = GetN(op);
	u32 m = GetM(op);
	macl = (u32)((((s32)r[n]) * ((s32)r[m])));
}
//************************ Div ! ************************
//div0u
sh4op(i0000_0000_0001_1001)
{//ToDo : Check This [26/4/05]
	//iNimp("div0u");
	sr.Q = 0;
	sr.M = 0;
	sr.T = 0;
}
//div0s <REG_M>,<REG_N>
sh4op(i0010_nnnn_mmmm_0111)
{//ToDo : Check This [26/4/05]
	//iNimp("div0s <REG_M>,<REG_N>");
	u32 n = GetN(op);
	u32 m = GetM(op);

	//new implementation
	sr.Q=r[n]>>31;
	sr.M=r[m]>>31;
	sr.T=sr.M^sr.Q;
	return;
	/*
	if ((r[n] & 0x80000000)!=0)
	sr.Q = 1;
	else
	sr.Q = 0;

	if ((r[m] & 0x80000000)!=0)
		sr.M = 1;
	else
		sr.M = 0;

	if (sr.Q == sr.M)
		sr.T = 0;
	else
		sr.T = 1;
		*/
}

//div1 <REG_M>,<REG_N>
sh4op(i0011_nnnn_mmmm_0100)
{
	u32 n=GetN(op);
	u32 m=GetM(op);

	unsigned long tmp0, tmp2;
	unsigned char old_q, tmp1;

	old_q = sr.Q;
	sr.Q = (u8)((0x80000000 & r[n]) !=0);

	r[n] <<= 1;
	r[n] |= (unsigned long)sr.T;

	tmp0 = r[n];	// this need only be done once here ..
	tmp2 = r[m];

	if( 0 == old_q )
	{	if( 0 == sr.M )
	{
		r[n] -= tmp2;
		tmp1	= (r[n]>tmp0);
		sr.Q	= (sr.Q==0) ? tmp1 : (u8)(tmp1==0) ;
	}	else	{
		r[n] += tmp2;
		tmp1	=(r[n]<tmp0);
		sr.Q	= (sr.Q==0) ? (u8)(tmp1==0) : tmp1 ;
	}
	}	else
	{	if( 0 == sr.M )
	{
		r[n] += tmp2;
		tmp1	=(r[n]<tmp0);
		sr.Q	= (sr.Q==0) ? tmp1 : (u8)(tmp1==0) ;
	}	else	{
		r[n] -= tmp2;
		tmp1	=(r[n]>tmp0);
		sr.Q	= (sr.Q==0) ? (u8)(tmp1==0) : tmp1 ;
	}
	}
	sr.T = (sr.Q==sr.M);
}

//************************ Simple maths ************************
//addc <REG_M>,<REG_N>
sh4op(i0011_nnnn_mmmm_1110)
{
	u32 n = GetN(op);
	u32 m = GetM(op);
	u32 tmp1 = r[n] + r[m];
	u32 tmp0 = r[n];

	r[n] = tmp1 + !!sr.T;

	if (tmp0 > tmp1)
		sr.T = 1;
	else
		sr.T = 0;

	if (tmp1 > r[n])
		sr.T = 1;
}

// addv <REG_M>,<REG_N>
sh4op(i0011_nnnn_mmmm_1111)
{
	iNimp(op, "addv <REG_M>,<REG_N>");
	u32 n = GetN(op);
	u32 m = GetM(op);
	//s64 br=(s64)(s32)r[n]+(s64)(s32)r[m];
	u32 rm=r[m];
	u32 rn=r[n];
	/*__asm
	{
		mov eax,rm;
		mov ecx,rn;
		add eax,ecx;
		seto sr.T;
		mov rn,eax;
	};*/
	r[n]=rn;
	/*
	if (br >=0x80000000)
		sr.T=1;
	else if (br < (s64) (0xFFFFFFFF80000000u))
		sr.T=1;
	else
		sr.T=0;
	/*if (br>>32)
		sr.T=1;
	else
		sr.T=0;*/

	//r[n]+=r[m];
}

//subc <REG_M>,<REG_N>
sh4op(i0011_nnnn_mmmm_1010)
{//ToDo : Check This [26/4/05]
	//iNimp(op,"subc <REG_M>,<REG_N>");
	u32 n = GetN(op);
	u32 m = GetM(op);

	u32 tmp1 = (u32)(((s32)r[n]) - ((s32)r[m]));
	u32 tmp0 = r[n];
	r[n] = tmp1 - !!sr.T;

	if (tmp0 < tmp1)
		sr.T=1;
	else
		sr.T=0;

	if (tmp1 < r[n])
		sr.T=1;
}

//subv <REG_M>,<REG_N>
sh4op(i0011_nnnn_mmmm_1011)
{
	iNimp(op, "subv <REG_M>,<REG_N>");
	u32 n = GetN(op);
	u32 m = GetM(op);
	/*s64 br=(s64)(s32)r[n]-(s64)(s32)r[m];

	if (br >=0x80000000)
		sr.T=1;
	else if (br < (s64) (0xFFFFFFFF80000000u))
		sr.T=1;
	else
		sr.T=0;*/
	/*if (br>>32)
		sr.T=1;
	else
		sr.T=0;*/

	//r[n]-=r[m];
	u32 rm=r[m];
	u32 rn=r[n];
	/*__asm
	{
		mov eax,rm;
		mov ecx,rn;
		sub eax,ecx;
		seto sr.T;
		mov rn,eax;
	};*/
	r[n]=rn;
}
//dt <REG_N>
sh4op(i0100_nnnn_0001_0000)
{
	u32 n = GetN(op);
	r[n]-=1;
	if (r[n] == 0)
		sr.T=1;
	else
		sr.T=0;
}

//negc <REG_M>,<REG_N>
sh4op(i0110_nnnn_mmmm_1010)
{
	//iNimp("negc <REG_M>,<REG_N>");
	u32 n = GetN(op);
	u32 m = GetM(op);

	//r[n]=-r[m]-sr.T;
	u32 tmp=0 - r[m];
	r[n]=tmp - !!sr.T;

	if (0<tmp)
		sr.T=1;
	else
		sr.T=0;

	if (tmp<r[n])
		sr.T=1;

	//NO , T IS *_CARRY_* , *NOT* sign :)
	//sr.T=r[n]>>31;
}


//neg <REG_M>,<REG_N>
sh4op(i0110_nnnn_mmmm_1011)
{//ToDo : Check This [26/4/05]
	u32 n = GetN(op);
	u32 m = GetM(op);
	r[n] = -r[m];
}

//not <REG_M>,<REG_N>
sh4op(i0110_nnnn_mmmm_0111)
{
	//iNimp("not <REG_M>,<REG_N>");
	u32 n = GetN(op);
	u32 m = GetM(op);

	r[n] = ~r[m];
}


//************************ shifts/rotates ************************
//shll <REG_N>
sh4op(i0100_nnnn_0000_0000)
{//ToDo : Check This [26/4/05]
	u32 n = GetN(op);

	sr.T = r[n] >> 31;
	r[n] <<= 1;
}
//shal <REG_N>
sh4op(i0100_nnnn_0010_0000)
{
	u32 n=GetN(op);
	sr.T=r[n]>>31;
	r[n]=((s32)r[n])<<1;
}


//shlr <REG_N>
sh4op(i0100_nnnn_0000_0001)
{//ToDo : Check This [26/4/05]
	u32 n = GetN(op);
	sr.T = r[n] & 0x1;
	r[n] >>= 1;
}

//shar <REG_N>
sh4op(i0100_nnnn_0010_0001)
{//ToDo : Check This [26/4/05] x2
	//iNimp("shar <REG_N>");
	u32 n = GetN(op);

	sr.T=r[n] & 1;
	r[n]=((s32)r[n])>>1;
}

//shad <REG_M>,<REG_N>
sh4op(i0100_nnnn_mmmm_1100)
{
	//iNimp(op,"shad <REG_M>,<REG_N>");
	u32 n = GetN(op);
	u32 m = GetM(op);
	u32 sgn = r[m] & 0x80000000;
	if (sgn == 0)
		r[n] <<= (r[m] & 0x1F);
	else if ((r[m] & 0x1F) == 0)
	{
		if ((r[n] & 0x80000000) == 0)
			r[n] = 0;
		else
			r[n] = 0xFFFFFFFF;
	}
	else
		r[n] = ((s32)r[n]) >> ((~r[m] & 0x1F) + 1);
}


//shld <REG_M>,<REG_N>
sh4op(i0100_nnnn_mmmm_1101)
{//ToDo : Check This [26/4/05] x2
	//iNimp("shld <REG_M>,<REG_N>");
	//HACK : CHECKME
	/*u32 n = GetN(op);
	u32 m = GetM(op);
	s32 s;

	s =  (s32)(r[m] & 0x80000000);
	if (s == 0)
		r[n] <<= (r[m]);
	else if ((r[m] & 0x1f) == 0)
		r[n] = 0;
	else
		r[n] = (u32)r[n] >> (32-r[m]);*/ // -> bug ! 32-r[m] -> -(r[m]&0x1F)

	u32 n = GetN(op);
	u32 m = GetM(op);
	u32 sgn = r[m] & 0x80000000;
	if (sgn == 0)
		r[n] <<= (r[m] & 0x1F);
	else if ((r[m] & 0x1F) == 0)
	{
		r[n] = 0;
	}
	else
		r[n] = ((u32)r[n]) >> ((~r[m] & 0x1F) + 1);
}



//rotcl <REG_N>
sh4op(i0100_nnnn_0010_0100)
{
	//iNimp("rotcl <REG_N>");
	u32 n = GetN(op);
	u32 t;

	t = !!sr.T;

	sr.T = r[n] >> 31;

	r[n] <<= 1;

	r[n]|=t;
}


//rotl <REG_N>
sh4op(i0100_nnnn_0000_0100)
{
	//iNimp("rotl <REG_N>");
	u32 n = GetN(op);

	sr.T=r[n]>>31;

	r[n] <<= 1;

	r[n]|=!!sr.T;
}

//rotcr <REG_N>
sh4op(i0100_nnnn_0010_0101)
{
	//iNimp("rotcr <REG_N>");
	u32 n = GetN(op);
	u32 t;


	t = r[n] & 0x1;

	r[n] >>= 1;

	r[n] |=(!!sr.T)<<31;

	sr.T = t;
}


//rotr <REG_N>
sh4op(i0100_nnnn_0000_0101)
{
	//iNimp("rotr <REG_N>");
	u32 n = GetN(op);
	sr.T = r[n] & 0x1;
	r[n] >>= 1;
	r[n] |= ((!!sr.T) << 31);
}
//************************ byte reorder/sign ************************
//swap.b <REG_M>,<REG_N>
sh4op(i0110_nnnn_mmmm_1000)
{
	//iNimp("swap.b <REG_M>,<REG_N>");
	u32 m = GetM(op);
	u32 n = GetN(op);
	r[n] = (r[m] & 0xFFFF0000) | ((r[m]&0xFF)<<8) | ((r[m]>>8)&0xFF);
}


//swap.w <REG_M>,<REG_N>
sh4op(i0110_nnnn_mmmm_1001)
{
	u32 n = GetN(op);
	u32 m = GetM(op);

	u16 t = (u16)(r[m]>>16);
	r[n] = (r[m] << 16) | t;
}



//extu.b <REG_M>,<REG_N>
sh4op(i0110_nnnn_mmmm_1100)
{
	u32 n = GetN(op);
	u32 m = GetM(op);
	r[n] = (u32)(u8)r[m];
}


//extu.w <REG_M>,<REG_N>
sh4op(i0110_nnnn_mmmm_1101)
{
	u32 n = GetN(op);
	u32 m = GetM(op);
	r[n] =(u32)(u16) r[m];
}


//exts.b <REG_M>,<REG_N>
sh4op(i0110_nnnn_mmmm_1110)
{
	//iNimp("exts.b <REG_M>,<REG_N>");
	u32 n = GetN(op);
	u32 m = GetM(op);

	r[n] = (u32)(s32)(s8)(u8)(r[m]);

}


//exts.w <REG_M>,<REG_N>
sh4op(i0110_nnnn_mmmm_1111)
{
	//iNimp("exts.w <REG_M>,<REG_N>");
	u32 n = GetN(op);
	u32 m = GetM(op);

	r[n] = (u32)(s32)(s16)(u16)(r[m]);
}


//xtrct <REG_M>,<REG_N>
sh4op(i0010_nnnn_mmmm_1101)
{
	//iNimp("xtrct <REG_M>,<REG_N>");
	u32 n = GetN(op);
	u32 m = GetM(op);

	r[n] = ((r[n] >> 16) & 0xFFFF) | ((r[m] << 16) & 0xFFFF0000);
}


//************************ xxx.b #<imm>,@(R0,GBR) ************************
//tst.b #<imm>,@(R0,GBR)
sh4op(i1100_1100_iiii_iiii)
{
	iNimp(op, "tst.b #<imm>,@(R0,GBR)");
}


//and.b #<imm>,@(R0,GBR)
sh4op(i1100_1101_iiii_iiii)
{
	iNimp(op, "and.b #<imm>,@(R0,GBR)");
}


//xor.b #<imm>,@(R0,GBR)
sh4op(i1100_1110_iiii_iiii)
{
	iNimp(op, "xor.b #<imm>,@(R0,GBR)");
}


//or.b #<imm>,@(R0,GBR)
sh4op(i1100_1111_iiii_iiii)
{
	//iNimp(op, "or.b #<imm>,@(R0,GBR)");
	u8 temp = (u8)ReadMem8(gbr +r[0]);
	temp |= GetImm8(op);
	WriteMem8(gbr +r[0], temp);
}
//tas.b @<REG_N>
sh4op(i0100_nnnn_0001_1011)
{
	//iNimp("tas.b @<REG_N>");
	u32 n = GetN(op);
	u8 val;

	val=(u8)ReadMem8(r[n]);

	u32 srT;
	if (val == 0)
		srT = 1;
	else
		srT = 0;

	val |= 0x80;

	WriteMem8(r[n], val);

	sr.T=srT;
}

//************************ Opcodes that read/write the status registers ************************
//stc SR,<REG_N>
sh4op(i0000_nnnn_0000_0010)//0002
{
	u32 n = GetN(op);
	r[n] = sr.GetFull();
}

 //sts FPSCR,<REG_N>
 sh4op(i0000_nnnn_0110_1010)
{
	//iNimp("sts FPSCR,<REG_N>");
	u32 n = GetN(op);
	r[n] = fpscr.full;
	UpdateFPSCR();
}

//sts.l FPSCR,@-<REG_N>
 sh4op(i0100_nnnn_0110_0010)
{
	u32 n = GetN(op);
	r[n] -= 4;
	WriteMemU32(r[n],fpscr.full);
}

//stc.l SR,@-<REG_N>
 sh4op(i0100_nnnn_0000_0011)
{
	//iNimp("stc.l SR,@-<REG_N>");
	u32 n = GetN(op);
	r[n] -= 4;
	WriteMemU32(r[n], sr.GetFull());
}

//lds.l @<REG_N>+,FPSCR
 sh4op(i0100_nnnn_0110_0110)
{
	u32 n = GetN(op);

	ReadMemU32(fpscr.full,r[n]);

	UpdateFPSCR();
	r[n] += 4;
}

//ldc.l @<REG_N>+,SR
 sh4op(i0100_nnnn_0000_0111)
{
	//iNimp("ldc.l @<REG_N>+,SR");
	u32 n = GetN(op);

	u32 sr_t;
	ReadMemU32(sr_t,r[n]);

	sr.SetFull(sr_t);
	r[n] += 4;
	if (UpdateSR())
	{
		//FIXME olny if interrupts got on .. :P
		UpdateINTC();
	}
}

//lds <REG_N>,FPSCR
 sh4op(i0100_nnnn_0110_1010)
{
	u32 n = GetN(op);
	fpscr.full = r[n];
	UpdateFPSCR();
}

//ldc <REG_N>,SR
 sh4op(i0100_nnnn_0000_1110)
{
	//iNimp("ldc <REG_N>,SR");
	u32 n = GetN(op);
	sr.SetFull(r[n]);
	if (UpdateSR())
	{
		UpdateINTC();
	}
}


//bah
//Not implt
sh4op(iNotImplemented)
{
	cpu_iNimp(op,"Unknown opcode");
}

sh4op(gdrom_hle_op)
{
	EMUERROR("GDROM HLE NOT SUPPORTED");
}

