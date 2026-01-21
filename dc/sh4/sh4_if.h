#pragma once
#include "types.h"
#include "intc_types.h"

enum Sh4RegType
{
	//GPRs
	reg_r0,
	reg_r1,
	reg_r2,
	reg_r3,
	reg_r4,
	reg_r5,
	reg_r6,
	reg_r7,
	reg_r8,
	reg_r9,
	reg_r10,
	reg_r11,
	reg_r12,
	reg_r13,
	reg_r14,
	reg_r15,

	//FPU, bank 0
	reg_fr_0,
	reg_fr_1,
	reg_fr_2,
	reg_fr_3,
	reg_fr_4,
	reg_fr_5,
	reg_fr_6,
	reg_fr_7,
	reg_fr_8,
	reg_fr_9,
	reg_fr_10,
	reg_fr_11,
	reg_fr_12,
	reg_fr_13,
	reg_fr_14,
	reg_fr_15,

	//FPU, bank 1
	reg_xf_0,
	reg_xf_1,
	reg_xf_2,
	reg_xf_3,
	reg_xf_4,
	reg_xf_5,
	reg_xf_6,
	reg_xf_7,
	reg_xf_8,
	reg_xf_9,
	reg_xf_10,
	reg_xf_11,
	reg_xf_12,
	reg_xf_13,
	reg_xf_14,
	reg_xf_15,

	//GPR Interrupt bank
	reg_r0_Bank,
	reg_r1_Bank,
	reg_r2_Bank,
	reg_r3_Bank,
	reg_r4_Bank,
	reg_r5_Bank,
	reg_r6_Bank,
	reg_r7_Bank,

	//Misc regs
	reg_gbr,
	reg_ssr,
	reg_spc,
	reg_sgr,
	reg_dbr,
	reg_vbr,
	reg_mach,
	reg_macl,
	reg_pr,
	reg_fpul,
    reg_nextpc,
	reg_sr,				//includes T (combined on read/separated on write)
	reg_sr_status,		//Only the status bits
	reg_sr_T,			//only T
	reg_fpscr,
	
	reg_pc_dyn,			//Write only, for dynarec only (dynamic block exit address)

	sh4_reg_count,

	/*
		These are virtual registers, used by the dynarec decoder
	*/
	regv_dr_0,
	regv_dr_2,
	regv_dr_4,
	regv_dr_6,
	regv_dr_8,
	regv_dr_10,
	regv_dr_12,
	regv_dr_14,

	regv_xd_0,
	regv_xd_2,
	regv_xd_4,
	regv_xd_6,
	regv_xd_8,
	regv_xd_10,
	regv_xd_12,
	regv_xd_14,

	regv_fv_0,
	regv_fv_4,
	regv_fv_8,
	regv_fv_12,

	regv_xmtrx,

	NoReg=-1
};

//Varius sh4 registers

//Status register bitfield
struct sr_type
{
	union
	{
		struct
		{
#if HOST_ENDIAN==ENDIAN_LITTLE
			u32 T_h		:1;//<<0
			u32 S		:1;//<<1
			u32 rsvd0	:2;//<<2
			u32 IMASK	:4;//<<4
			u32 Q		:1;//<<8
			u32 M		:1;//<<9
			u32 rsvd1	:5;//<<10
			u32 FD		:1;//<<15
			u32 rsvd2	:12;//<<16
			u32 BL		:1;//<<28
			u32 RB		:1;//<<29
			u32 MD		:1;//<<20
			u32 rsvd3	:1;//<<31
#else
			u32 rsvd3	:1;//<<31
			u32 MD		:1;//<<20
			u32 RB		:1;//<<29
			u32 BL		:1;//<<28
			u32 rsvd2	:12;//<<16
			u32 FD		:1;//<<15
			u32 rsvd1	:5;//<<10
			u32 M		:1;//<<9
			u32 Q		:1;//<<8
			u32 IMASK	:4;//<<4
			u32 rsvd0	:2;//<<2
			u32 S		:1;//<<1
			u32 T_h		:1;//<<0
#endif
		};
		u32 status;
	};
	u32 T;
	INLINE u32 GetFull()
	{
		return (status & 0x700083F2) | T;
	}

	INLINE void SetFull(u32 value)
	{
		status=value & 0x700083F2;
		T=value&1;
	}

};

//FPSCR (fpu status and control register) bitfield
struct fpscr_type
{
	union
	{
		u32 full;
		struct
		{
#if HOST_ENDIAN==ENDIAN_LITTLE
			u32 RM:2;
			u32 finexact:1;
			u32 funderflow:1;
			u32 foverflow:1;
			u32 fdivbyzero:1;
			u32 finvalidop:1;
			u32 einexact:1;
			u32 eunderflow:1;
			u32 eoverflow:1;
			u32 edivbyzero:1;
			u32 einvalidop:1;
			u32 cinexact:1;
			u32 cunderflow:1;
			u32 coverflow:1;
			u32 cdivbyzero:1;
			u32 cinvalid:1;
			u32 cfpuerr:1;
			u32 DN:1;
			u32 PR:1;
			u32 SZ:1;
			u32 FR:1;
			u32 pad:10;
#else
			u32 pad:10;
			u32 FR:1;
			u32 SZ:1;
			u32 PR:1;
			u32 DN:1;
			u32 cfpuerr:1;
			u32 cinvalid:1;
			u32 cdivbyzero:1;
			u32 coverflow:1;
			u32 cunderflow:1;
			u32 cinexact:1;
			u32 einvalidop:1;
			u32 edivbyzero:1;
			u32 eoverflow:1;
			u32 eunderflow:1;
			u32 einexact:1;
			u32 finvalidop:1;
			u32 fdivbyzero:1;
			u32 foverflow:1;
			u32 funderflow:1;
			u32 finexact:1;
			u32 RM:2;
#endif
		};
		struct
		{
#if HOST_ENDIAN==ENDIAN_LITTLE
			u32 nil:2+1+1+1+1+4+8+1;
			u32 PR_SZ:2;
			u32 nilz:11;
#else
			u32 nilz:11;
			u32 PR_SZ:2;
			u32 nil:2+1+1+1+1+4+8+1;
#endif
		};
	};
};


typedef void RunFP();
typedef void StopFP();
typedef void StepFP();
typedef void SkipFP();
typedef void ResetFP(bool Manual);
typedef void InitFP();
typedef void TermFP();
typedef bool IsCpuRunningFP();

typedef void FASTCALL sh4_int_RaiseExeptionFP(u32 ExeptionCode,u32 VectorAddress);

/*
	The interface stuff should be replaced with something nicer
*/
//sh4 interface
struct sh4_if
{
	RunFP* Run;
	StopFP* Stop;
	StepFP* Step;
	SkipFP* Skip;
	ResetFP* Reset;
	InitFP* Init;
	TermFP* Term;

	TermFP* ResetCache;

	IsCpuRunningFP* IsCpuRunning;
};

//Get an interface to sh4 interpreter
void Get_Sh4Interpreter(sh4_if* cpu);
void Get_Sh4Recompiler(sh4_if* cpu);

//free it
void Release_Sh4If(sh4_if* cpu);

#define BPT_OPCODE		0x8A00
