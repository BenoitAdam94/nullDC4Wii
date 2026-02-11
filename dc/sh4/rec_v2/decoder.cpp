/*
	Ugly, hacky, bad code
	It decodes sh4 opcodes too
*/

#include "types.h"
#include "decoder.h"
#include "shil.h"
#include "ngen.h"
#include "dc/sh4/sh4_opcode_list.h"
#include "dc/sh4/sh4_registers.h"
#include "dc/mem/sh4_mem.h"
#include "decoder_opcodes.h"

#define NullAddress 0xFFFFFFFF

static shil_param mk_imm(u32 immv)
{
	return shil_param(FMT_IMM,immv);
}
static shil_param mk_reg(Sh4RegType reg)
{
	return shil_param(reg);
}
static shil_param mk_regi(int reg)
{
	return mk_reg((Sh4RegType)reg);
}
enum NextDecoderOperation
{
	NDO_NextOp,		//pc+=2
	NDO_End,		//End the block, Type = BlockEndType
	NDO_Delayslot,	//pc+=2, NextOp=DelayOp
	NDO_Jump,		//pc=JumpAddr,NextOp=JumpOp
};

void DecodedBlock::Setup(u32 rpc)
{
	start=rpc;
	cycles=0;
	opcodes=0;

	BranchBlock=0xFFFFFFFF;
	NextBlock=0xFFFFFFFF;

	BlockType=BET_SCL_Intr;
	oplist.clear();
}

DecodedBlock block;
struct
{
	NextDecoderOperation NextOp;
	NextDecoderOperation DelayOp;
	NextDecoderOperation JumpOp;
	u32 JumpAddr;
	u32 NextAddr;
	BlockEndType BlockType;

	struct
	{
		bool FPR64;	//64 bit fpu opcodes
		bool FSZ64;	//64 bit fpu moves
		bool RoundToZero;	//false -> Round to nearest.
		u32 rpc;
		bool is_delayslot;
	}cpu;

	ngen_features ngen;

	struct
	{
		bool has_readm;
		bool has_writem;
		bool has_fpu;
	} info;

	void Setup(u32 rpc,fpscr_type fpu_cfg)
	{
		cpu.rpc=rpc;
		cpu.is_delayslot=false;
		cpu.FPR64=fpu_cfg.PR;
		cpu.FSZ64=fpu_cfg.SZ;
		cpu.RoundToZero=fpu_cfg.RM==1;
		verify(fpu_cfg.RM<2);
		//what about fp/fs ?

		NextOp=NDO_NextOp;
		BlockType=BET_SCL_Intr;
		JumpAddr=0xFFFFFFFF;
		NextAddr=0xFFFFFFFF;

		info.has_readm=false;
		info.has_writem=false;
		info.has_fpu=false;
	}
} state;

static void dec_fallback(u32 op)
{
	shil_opcode opcd;
	opcd.op=shop_ifb;

	opcd.rs1=shil_param(FMT_IMM,OpDesc[op]->NeedPC());

	opcd.rs2=shil_param(FMT_IMM,state.cpu.rpc+2);
	opcd.rs3=shil_param(FMT_IMM,op);
	block.oplist.push_back(opcd);
}

#if 1
/*
#define		FMT_I32 OMG!THIS!IS!WRONG++!!
#define 	FMT_F32 OMG!THIS!IS!WRONG++!!
#define		FMT_F32 OMG!THIS!IS!WRONG++!!
#define 	FMT_TYPE OMG!THIS!IS!WRONG++!!

#define 	FMT_REG OMG!THIS!IS!WRONG++!!
#define 	FMT_IMM OMG!THIS!IS!WRONG++!!

#define 	FMT_PARAM OMG!THIS!IS!WRONG++!!
#define 	FMT_MASK OMG!THIS!IS!WRONG++!!
*/

static void dec_DynamicSet(u32 regbase,u32 offs=0)
{
	if (offs==0)
		block.Emit(shop_jdyn,reg_pc_dyn,mk_reg((Sh4RegType)regbase));
	else
		block.Emit(shop_jdyn,reg_pc_dyn,mk_reg((Sh4RegType)regbase),mk_imm(offs));
}

static void dec_End(u32 dst,BlockEndType flags,bool delay)
{
	if (state.ngen.OnlyDynamicEnds && flags == BET_StaticJump)
	{
		block.Emit(shop_mov32,mk_reg(reg_nextpc),mk_imm(dst));
		dec_DynamicSet(reg_nextpc);
		dec_End(NullAddress,BET_DynamicJump,delay);
		return;
	}

	if (state.ngen.OnlyDynamicEnds)
	{
		verify(flags == BET_DynamicJump);
	}

	state.BlockType=flags;
	state.NextOp=delay?NDO_Delayslot:NDO_End;
	state.DelayOp=NDO_End;
	state.JumpAddr=dst;
	state.NextAddr=state.cpu.rpc+2+(delay?2:0);
}

#define GetN(str) ((str>>8) & 0xf)
#define GetM(str) ((str>>4) & 0xf)
#define GetImm4(str) ((str>>0) & 0xf)
#define GetImm8(str) ((str>>0) & 0xff)
#define GetSImm8(str) ((s8)((str>>0) & 0xff))
#define GetImm12(str) ((str>>0) & 0xfff)
#define GetSImm12(str) (((s16)((GetImm12(str))<<4))>>4)


#define SR_STATUS_MASK 0x700083F2
#define SR_T_MASK 1

static u32 dec_jump_simm8(u32 op)
{
	return state.cpu.rpc + GetSImm8(op)*2 + 4;
}
static u32 dec_jump_simm12(u32 op)
{
	return state.cpu.rpc + GetSImm12(op)*2 + 4;
}
static u32 dec_set_pr()
{
	u32 retaddr=state.cpu.rpc + 4;
	block.Emit(shop_mov32,reg_pr,mk_imm(retaddr));
	return retaddr;
}
static void dec_write_sr(shil_param src)
{
	block.Emit(shop_and,mk_reg(reg_sr_status),src,mk_imm(SR_STATUS_MASK));
	block.Emit(shop_and,mk_reg(reg_sr_T),src,mk_imm(SR_T_MASK));
}
//bf <bdisp8>
sh4dec(i1000_1011_iiii_iiii)
{
	dec_End(dec_jump_simm8(op),BET_Cond_0,false);
}
//bf.s <bdisp8>
sh4dec(i1000_1111_iiii_iiii)
{
	block.Emit(shop_jcond,reg_pc_dyn,reg_sr_T);
	dec_End(dec_jump_simm8(op),BET_Cond_0,true);
}
//bt <bdisp8>
sh4dec(i1000_1001_iiii_iiii)
{
	dec_End(dec_jump_simm8(op),BET_Cond_1,false);
}
//bt.s <bdisp8>
sh4dec(i1000_1101_iiii_iiii)
{
	block.Emit(shop_jcond,reg_pc_dyn,reg_sr_T);
	dec_End(dec_jump_simm8(op),BET_Cond_1,true);
}
//bra <bdisp12>
sh4dec(i1010_iiii_iiii_iiii)
{
	dec_End(dec_jump_simm12(op),BET_StaticJump,true);
}
//braf <REG_N>
sh4dec(i0000_nnnn_0010_0011)
{
	u32 n = GetN(op);

	dec_DynamicSet(reg_r0+n,state.cpu.rpc + 4);
	dec_End(NullAddress,BET_DynamicJump,true);
}
//jmp @<REG_N>
sh4dec(i0100_nnnn_0010_1011)
{
	u32 n = GetN(op);

	dec_DynamicSet(reg_r0+n);
	dec_End(NullAddress,BET_DynamicJump,true);
}
//bsr <bdisp12>
sh4dec(i1011_iiii_iiii_iiii)
{
	//TODO: set PR
	dec_set_pr();
	dec_End(dec_jump_simm12(op),BET_StaticCall,true);
}
//bsrf <REG_N>
sh4dec(i0000_nnnn_0000_0011)
{
	u32 n = GetN(op);
	//TODO: set PR
	u32 retaddr=dec_set_pr();
	dec_DynamicSet(reg_r0+n,retaddr);
	dec_End(NullAddress,BET_DynamicCall,true);
}
//jsr @<REG_N>
sh4dec(i0100_nnnn_0000_1011) 
{
	u32 n = GetN(op);

	//TODO: Set pr
	dec_set_pr();
	dec_DynamicSet(reg_r0+n);
	dec_End(NullAddress,BET_DynamicCall,true);
}
//rts
sh4dec(i0000_0000_0000_1011)
{
	dec_DynamicSet(reg_pr);
	dec_End(NullAddress,BET_DynamicRet,true);
}
//rte
sh4dec(i0000_0000_0010_1011)
{
	//TODO: Write SR, Check intr
	dec_write_sr(reg_ssr);
	block.Emit(shop_sync_sr);
	dec_DynamicSet(reg_spc);
	dec_End(NullAddress,BET_DynamicIntr,true);
}
//trapa #<imm>
sh4dec(i1100_0011_iiii_iiii)
{
	//TODO: ifb
	dec_fallback(op);
	dec_DynamicSet(reg_nextpc);
	dec_End(NullAddress,BET_DynamicJump,false);
}
//sleep
sh4dec(i0000_0000_0001_1011)
{
	//TODO: ifb
	dec_fallback(op);
	dec_DynamicSet(reg_nextpc);
	dec_End(NullAddress,BET_DynamicJump,false);
}

//ldc.l @<REG_N>+,SR
sh4dec(i0100_nnnn_0000_0111)
{
	/*
	u32 sr_t;
	ReadMemU32(sr_t,r[n]);
	if (sh4_exept_raised)
		return;
	sr.SetFull(sr_t);
	r[n] += 4;
	if (UpdateSR())
	{
		//FIXME olny if interrupts got on .. :P
		UpdateINTC();
	}*/
	dec_End(NullAddress,BET_StaticIntr,true);
}

//ldc <REG_N>,SR
sh4dec(i0100_nnnn_0000_1110)
{
	/*
	u32 n = GetN(op);
	sr.SetFull(r[n]);
	if (UpdateSR())
	{
		UpdateINTC();
	}*/
	dec_End(NullAddress,BET_StaticIntr,true);
}

//nop !
sh4dec(i0000_0000_0000_1001)
{
}

sh4dec(i1111_0011_1111_1101)
{
	//fpscr.SZ is bit 20
	block.Emit(shop_xor,reg_fpscr,reg_fpscr,mk_imm(1<<20));
	state.cpu.FSZ64=!state.cpu.FSZ64;
}
#endif

const Sh4RegType SREGS[] =
{
	reg_mach,
	reg_macl,
	reg_pr,
	reg_sgr,
	NoReg,
	reg_fpul,
	reg_fpscr,
	NoReg,

	NoReg,
	NoReg,
	NoReg,
	NoReg,
	NoReg,
	NoReg,
	NoReg,
	reg_dbr,
};

const Sh4RegType CREGS[] =
{
	reg_sr,
	reg_gbr,
	reg_vbr,
	reg_ssr,
	reg_spc,
	NoReg,
	NoReg,
	NoReg,

	reg_r0_Bank,
	reg_r1_Bank,
	reg_r2_Bank,
	reg_r3_Bank,
	reg_r4_Bank,
	reg_r5_Bank,
	reg_r6_Bank,
	reg_r7_Bank,
};

void dec_param(DecParam p,shil_param& r1,shil_param& r2, u32 op)
{
	switch(p)
	{
		//constants
	case PRM_PC_D8_x2:
		r1=mk_imm((state.cpu.rpc+4)+(GetImm8(op)<<1));
		break;

	case PRM_PC_D8_x4:
		r1=mk_imm(((state.cpu.rpc+4)&0xFFFFFFFC)+(GetImm8(op)<<2));
		break;
	
	case PRM_ZERO:
		r1= mk_imm(0);
		break;

	case PRM_ONE:
		r1= mk_imm(1);
		break;

	case PRM_TWO:
		r1= mk_imm(2);
		break;

	case PRM_TWO_INV:
		r1= mk_imm(~2);
		break;

	case PRM_ONE_F32:
		r1= mk_imm(0x3f800000);
		break;

	//imms
	case PRM_SIMM8:
		r1=mk_imm(GetSImm8(op));
		break;
	case PRM_UIMM8:
		r1=mk_imm(GetImm8(op));
		break;

	//direct registers
	case PRM_R0:
		r1=mk_reg(reg_r0);
		break;

	case PRM_RN:
		r1=mk_regi(reg_r0+GetN(op));
		break;

	case PRM_RM:
		r1=mk_regi(reg_r0+GetM(op));
		break;

	case PRM_FRN_SZ:
		if (state.cpu.FSZ64)
		{
			int rx=GetN(op)/2;
			if (GetN(op)&1)
				rx+=regv_xd_0;
			else
				rx+=regv_dr_0;

			r1=mk_regi(rx);
			break;
		}
	case PRM_FRN:
		r1=mk_regi(reg_fr_0+GetN(op));
		break;

	case PRM_FRM_SZ:
		if (state.cpu.FSZ64)
		{
			int rx=GetM(op)/2;
			if (GetM(op)&1)
				rx+=regv_xd_0;
			else
				rx+=regv_dr_0;

			r1=mk_regi(rx);
			break;
		}
	case PRM_FRM:
		r1=mk_regi(reg_fr_0+GetM(op));
		break;

	case PRM_FPUL:
		r1=mk_regi(reg_fpul);
		break;

	case PRM_FPN:	//float pair, 3 bits
		r1=mk_regi(regv_dr_0+GetN(op)/2);
		break;

	case PRM_FVN:	//float quad, 2 bits
		r1=mk_regi(regv_fv_0+GetN(op)/4);
		break;

	case PRM_FVM:	//float quad, 2 bits
		r1=mk_regi(regv_fv_0+(GetN(op)&0x3));
		break;

	case PRM_XMTRX:	//float matrix, 0 bits
		r1=mk_regi(regv_xmtrx);
		break;

	case PRM_FRM_FR0:
		r1=mk_regi(reg_fr_0+GetM(op));
		r2=mk_regi(reg_fr_0);
		break;

	case PRM_SR_T:
		r1=mk_regi(reg_sr_T);
		break;

	case PRM_SR_STATUS:
		r1=mk_regi(reg_sr_status);
		break;

	case PRM_SREG:	//FPUL/FPSCR/MACH/MACL/PR/DBR/SGR
		r1=mk_regi(SREGS[GetM(op)]);
		break;
	case PRM_CREG:	//SR/GBR/VBR/SSR/SPC/<RM_BANK>
		r1=mk_regi(CREGS[GetM(op)]);
		break;
	
	//reg/imm reg/reg
	case PRM_RN_D4_x1:
	case PRM_RN_D4_x2:
	case PRM_RN_D4_x4:
		{
			u32 shft=p-PRM_RN_D4_x1;
			r1=mk_regi(reg_r0+GetN(op));
			r2=mk_imm(GetImm4(op)<<shft);
		}
		break;

	case PRM_RN_R0:
		r1=mk_regi(reg_r0+GetN(op));
		r2=mk_regi(reg_r0);
		break;

	case PRM_RM_D4_x1:
	case PRM_RM_D4_x2:
	case PRM_RM_D4_x4:
		{
			u32 shft=p-PRM_RM_D4_x1;
			r1=mk_regi(reg_r0+GetM(op));
			r2=mk_imm(GetImm4(op)<<shft);
		}
		break;

	case PRM_RM_R0:
		r1=mk_regi(reg_r0+GetM(op));
		r2=mk_regi(reg_r0);
		break;

	case PRM_GBR_D8_x1:
	case PRM_GBR_D8_x2:
	case PRM_GBR_D8_x4:
		{
			u32 shft=p-PRM_GBR_D8_x1;
			r1=mk_regi(reg_gbr);
			r2=mk_imm(GetImm8(op)<<shft);
		}
		break;

	default:
		die("Nop supported param used");
	}
}
bool dec_generic(u32 op)
{
	DecMode mode;DecParam d;DecParam s;shilop natop;u32 e;
	if (OpDesc[op]->decode==0)
		return false;
	
	u64 inf=OpDesc[op]->decode;

	e=(u32)(inf>>32);
	mode=(DecMode)((inf>>24)&0xFF);
	d=(DecParam)((inf>>16)&0xFF);
	s=(DecParam)((inf>>8)&0xFF);
	natop=(shilop)((inf>>0)&0xFF);

	//if ((op&0xF00F)>=0x4005 && op<0x8000)
	{
	//	return false;
	}

	bool transfer_64=false;
	if (op>=0xF000)
	{
		state.info.has_fpu=true;
		//return false;//fpu off for now
		if (state.cpu.FPR64 /*|| state.cpu.FSZ64*/)
			return false;

		if (state.cpu.FSZ64 && (d==PRM_FRN_SZ || d==PRM_FRM_SZ || s==PRM_FRN_SZ || s==PRM_FRM_SZ))
		{
			transfer_64=true;
		}
	}

	shil_param rs1,rs2,rs3,rd;

	dec_param(s,rs2,rs3,op);
	dec_param(d,rs1,rs3,op);

	switch(mode)
	{
	case DM_ReadSRF:
		// Read SR with flags combined
		block.Emit(shop_mov32, rs1, reg_sr_status);
		block.Emit(shop_or, rs1, rs1, reg_sr_T);
		break;
	
	case DM_ADC:
		// Add/subtract with carry
		block.Emit(natop, rs1, rs1, rs2, 0, mk_reg(reg_sr_T), mk_reg(reg_sr_T));
		break;
	
	case DM_NEGC:
		// Negate with carry: 0 - src - T
		block.Emit(natop, rs1, rs2, mk_reg(reg_sr_T), 0, shil_param(), mk_reg(reg_sr_T));
		break;

	case DM_WriteTOp:
		block.Emit(natop,reg_sr_T,rs1,rs2);
		break;

	case DM_DT:
		verify(natop==shop_sub);
		block.Emit(natop,rs1,rs1,rs2);
		block.Emit(shop_seteq,mk_reg(reg_sr_T),rs1,mk_imm(0));
		break;

	case DM_Shift:
		if (natop==shop_shl && e==1)
			block.Emit(shop_shr,mk_reg(reg_sr_T),rs1,mk_imm(31));
		else if (e==1)
			block.Emit(shop_and,mk_reg(reg_sr_T),rs1,mk_imm(1));

		block.Emit(natop,rs1,rs1,mk_imm(e));
		break;

	case DM_Rot:
		if (!(((s32)e>=0?e:-e)&0x1000))
		{
			if ((s32)e<0)
			{
				//left rorate
				block.Emit(shop_shr,mk_reg(reg_sr_T),rs2,mk_imm(31));
				e=-e;
			}
			else
			{
				//right rotate
				block.Emit(shop_and,mk_reg(reg_sr_T),rs2,mk_imm(1));
			}
		}
		e&=31;

		block.Emit(natop,rs1,rs2,mk_imm(e));
		break;

	case DM_BinaryOp://d=d op s
		if (e&1)
			block.Emit(natop,rs1,rs1,rs2,0,rs3);
		else
			block.Emit(natop,shil_param(),rs1,rs2,0,rs3);
		break;

	case DM_UnaryOp: //d= op s
		if (transfer_64 && natop==shop_mov32) 
			natop=shop_mov64;

		if (natop==shop_cvt_i2f_n && state.cpu.RoundToZero)
			natop=shop_cvt_i2f_z;

		if (e&1)
			block.Emit(natop,shil_param(),rs1);
		else
			block.Emit(natop,rs1,rs2);
		break;

	case DM_WriteM:	//write(d,s)
		{
			//0 has no effect, so get rid of it
			if (rs3.is_imm() && rs3._imm==0)
				rs3=shil_param();

			state.info.has_writem=true;
			if (transfer_64) e=(s32)e*2;
			bool update_after=false;
			if ((s32)e<0)
			{
				if (rs1._reg!=rs2._reg)	//reg shoudn't be updated if its writen
				{
					block.Emit(shop_sub,rs1,rs1,mk_imm(-e));
				}
				else
				{
					verify(rs3.is_null());
					rs3=mk_imm(e);
					update_after=true;
				}
			}

			block.Emit(shop_writem,shil_param(),rs1,rs2,(s32)e<0?-e:e,rs3);

			if (update_after)
			{
				block.Emit(shop_sub,rs1,rs1,mk_imm(-e));
			}
		}
		break;

	case DM_ReadM:
		//0 has no effect, so get rid of it
		if (rs3.is_imm() && rs3._imm==0)
				rs3=shil_param();

		state.info.has_readm=true;
		if (transfer_64) e=(s32)e*2;

		block.Emit(shop_readm,rs1,rs2,shil_param(),(s32)e<0?-e:e,rs3);
		if ((s32)e<0)
		{
			if (rs1._reg!=rs2._reg)//the reg shoudn't be updated if it was just readed
				block.Emit(shop_add,rs2,rs2,mk_imm(-e));
		}
		break;

	case DM_fiprOp:
		{
			shil_param rdd=mk_regi(rs1._reg+3);
			block.Emit(natop,rdd,rs1,rs2);
		}
		break;

	case DM_EXTOP:
		{
			block.Emit(natop,rs1,rs2,mk_imm(e==1?0xFF:0xFFFF));
		}
		break;
	
	case DM_MUL:
		{
			shilop op;
			shil_param rd=mk_reg(reg_macl);
			shil_param rd2=shil_param();

			switch((s32)e)
			{
				case 16:	op=shop_mul_u16; break;
				case -16:	op=shop_mul_s16; break;

				case -32:	op=shop_mul_i32; break;

				case 64:	op=shop_mul_u64; rd2 = mk_reg(reg_mach); break;
				case -64:	op=shop_mul_s64; rd2 = mk_reg(reg_mach); break;

				default:
					die("DM_MUL: Failed to classify opcode");
			}

			block.Emit(op,rd,rs1,rs2,0,shil_param(),rd2);
		}
		break;

	case DM_DIV0:
		{
			if (e==1)
			{
				//crear QM (bits 8,9)
				u32 qm=(1<<8)|(1<<9);
				block.Emit(shop_and,mk_reg(reg_sr_status),mk_reg(reg_sr_status),mk_imm(~qm));
				//clear T !
				block.Emit(shop_mov32,mk_reg(reg_sr_T),mk_imm(0));
			}
			else
			{
				//sr.Q=r[n]>>31;
				//sr.M=r[m]>>31;
				//sr.T=sr.M^sr.Q;

				//This is nasty because there isn't a temp reg ..
				//VERY NASTY

				//Clear Q & M
				block.Emit(shop_and,mk_reg(reg_sr_status),mk_reg(reg_sr_status),mk_imm(~((1<<8)|(1<<9))));

				//sr.Q=r[n]>>31;
				block.Emit(shop_sar,mk_reg(reg_sr_T),rs1,mk_imm(31));
				block.Emit(shop_and,mk_reg(reg_sr_T),mk_reg(reg_sr_T),mk_imm(1<<8));
				block.Emit(shop_or,mk_reg(reg_sr_status),mk_reg(reg_sr_status),mk_reg(reg_sr_T));

				//sr.M=r[m]>>31;
				block.Emit(shop_sar,mk_reg(reg_sr_T),rs2,mk_imm(31));
				block.Emit(shop_and,mk_reg(reg_sr_T),mk_reg(reg_sr_T),mk_imm(1<<9));
				block.Emit(shop_or,mk_reg(reg_sr_status),mk_reg(reg_sr_status),mk_reg(reg_sr_T));

				//sr.T=sr.M^sr.Q;
				block.Emit(shop_xor,mk_reg(reg_sr_T),rs1,rs2);
				block.Emit(shop_shr,mk_reg(reg_sr_T),mk_reg(reg_sr_T),mk_imm(31));
			}
		}
		break;

	default:
		verify(false);
	}

	return true;
}

int stuffcmp(const void* p1,const void* p2)
{
	sh4_opcodelistentry* a=(sh4_opcodelistentry*)p1;
	sh4_opcodelistentry* b=(sh4_opcodelistentry*)p2;

	return b->fallbacks-a->fallbacks;
}
DecodedBlock* dec_DecodeBlock(u32 startpc,fpscr_type fpu_cfg,u32 max_cycles)
{
	block.Setup(startpc);
	state.Setup(startpc,fpu_cfg);
#ifndef HOST_NO_REC
	ngen_GetFeatures(&state.ngen);
#endif
	for(;;)
	{
		switch(state.NextOp)
		{
		case NDO_Delayslot:
			state.NextOp=state.DelayOp;
			state.cpu.is_delayslot=true;
			//there is no break here by design
		case NDO_NextOp:
			{
				if (block.cycles>=max_cycles && !state.cpu.is_delayslot)
				{
					dec_End(state.cpu.rpc,BET_StaticJump,false);
				}
				else
				{
					u32 op=ReadMem16(state.cpu.rpc);
					block.opcodes++;
					if (op>=0xF000)
						block.cycles+=0;
					else
						block.cycles+=CPU_RATIO;

					if (state.ngen.OnlyDynamicEnds || !OpDesc[op]->rec_oph)
					{
						if (state.ngen.InterpreterFallback || !dec_generic(op))
						{
							dec_fallback(op);
							if (OpDesc[op]->SetPC())
							{
								dec_DynamicSet(reg_nextpc);
								dec_End(NullAddress,BET_DynamicJump,false);
							}
							if (OpDesc[op]->SetFPSCR() && !state.cpu.is_delayslot)
							{
								dec_End(state.cpu.rpc+2,BET_StaticJump,false);
							}
						}
					}
					else
					{
						OpDesc[op]->rec_oph(op);
					}

					state.cpu.rpc+=2;
				}
			}
			break;

		case NDO_Jump:
			state.NextOp=state.JumpOp;
			state.cpu.rpc=state.JumpAddr;
			break;

		case NDO_End:
			goto _end;
		}
	}

_end:
	block.NextBlock=state.NextAddr;
	block.BranchBlock=state.JumpAddr;
	block.BlockType=state.BlockType;

	//cycle tricks
	{
		//Small-n-simple idle loop detector :p
		if (state.info.has_readm && !state.info.has_writem && !state.info.has_fpu && block.opcodes<6)
		{
			if (block.BlockType==BET_Cond_0 || block.BlockType==BET_Cond_1)
			{
				block.cycles*=30;
			}

			if (block.BranchBlock==block.start)
			{
				block.cycles*=10;
			}
		}

		//if in syscalls area (ip.bin etc) skip fast :p
		if ((block.start&0x1FFF0000)==0x0C000000)
		{
			if (block.start&0x8000)
			{
				//ip.bin (boot loader/img etc)
				block.cycles*=15;
			}
			else
			{
				//syscalls
				block.cycles*=5;
			}
			
		}
		//make sure we don't use wayy-too-many cycles ;p
		block.cycles=min(block.cycles,max_cycles);
	}

	static int cc=0;
	cc++;
	if ((cc&31)==0 && HOST_OS==OS_WINDOWS)
	{
		vector<sh4_opcodelistentry> stuff;
		for (u32 i=0;opcodes[i].oph;i++)
		{
			if (opcodes[i].fallbacks)
				stuff.push_back(opcodes[i]);
		}
		if (stuff.size() && 0)
		{
			printf("Compile stats:\n");
			qsort(&stuff[0],stuff.size(),sizeof(stuff[0]),stuffcmp);
			for (u32 i=0;i<10 && i<stuff.size();i++)
			{
				printf("%05I64u :%04X %s\n",stuff[i].fallbacks,stuff[i].rez,stuff[i].diss);
			}
		}
	}
	return &block;
}
void dec_Cleanup()
{
	block.oplist.clear();
}

