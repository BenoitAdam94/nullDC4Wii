#define FULL_VFPU 0
#define HALF_VFPU 0

#include "types.h"
#include "dc\sh4\sh4_opcode_list.h"

#include "dc\sh4\sh4_registers.h"
#include "dc\sh4\ccn.h"
#include "dc\sh4\rec_v2\ngen.h"
#include "dc\mem\sh4_mem.h"
#include "psp\psp_emit.h"
void __debugbreak() { fflush(stdout); *(int*)0=1;}

void make_address_range_executable(u32 address_start, u32 address_end)
{
	address_start = (address_start + 0) & -64;
	address_end   = (address_end   +63) & -64;

	for (; address_start < address_end; address_start += 64)
	{
		__builtin_allegrex_cache(0x1a, address_start);
		__builtin_allegrex_cache(0x1a, address_start);
		__builtin_allegrex_cache(0x08, address_start);
		__builtin_allegrex_cache(0x08, address_start);
	}
}

#if 1
#define REGISTER_BIT1(b1) (1<<b1)
#define REGISTER_BIT2(b1,b2) REGISTER_BIT1(b1)|REGISTER_BIT1(b2)
#define REGISTER_BIT4(b1,b2,b3,b4) REGISTER_BIT2(b1,b2)|REGISTER_BIT2(b3,b4)
#define REGISTER_BIT8(b1,b2,b3,b4,b5,b6,b7,b8) REGISTER_BIT4(b1,b2,b3,b4)|REGISTER_BIT4(b5,b6,b7,b8)

#define CALLEESAVED_REGISTERS REGISTER_BIT1(host_s8)|REGISTER_BIT8(host_s7, host_s6, host_s5, host_s4, host_s3, host_s2, host_s1, host_s0)
#define CALLERSAVED_REGISTERS REGISTER_BIT2(host_t9, host_t8)|REGISTER_BIT4(host_t7, host_t6, host_t5, host_t4)

#define OREGS_NUM (9 /*s0-s8 */ /* + 6 t4-t9 */)

#define NGPR_NUM 32

#define NREG_PRELOAD 1
#define NREG_REMAP   2
#define NREG_LOCK    4

#define OREGS_NUM (9/*+6*/)

u8 oregs[OREGS_NUM] =
{
#define SAFE_NREGS 0
	// those registers are saved-callee, so there is no need to spill them before calling a C function
	psp_s0, psp_s1, psp_s2, psp_s3, psp_s4, psp_s5, psp_s6, psp_s7, psp_s8,

#define TEMP_NREGS 8
	// those registers are saved-caller, so you need to spill them before calling a C function
	//psp_t4, psp_t5, psp_t6, psp_t7, psp_t8, psp_t9
};

#define EREGS_NUM sh4_reg_count

struct eregs_s
{
	u32 nreg : 8;
} eregs[EREGS_NUM]; // emulated registers

struct ngpr_s
{
	u32 ereg        : 8;
	u32 locked      : 1; // reserve it permanently or temporally so it cannot be mapped or spilled
	u32 mapped      : 1; // an emulated register is associated
	u32 dirty       : 1; // it will be spilled
} ngpr[NGPR_NUM]; // native registers

struct nhilo_s
{
	u16 mapped      : 1; // an emulated register is associated
	u16 dirty       : 1; // it will be spilled
} nhilo[2]; // native registers

void regcache_reset();

void spill_ngpr(u32 n = OREGS_NUM);

u32 map_ngpr(u32 ereg, u32 action);
u32 unmap_ngpr(u32 ereg, bool update = true);

void lock_ngpr(u32 ereg);
void unlock_ngpr(u32 ereg);

void set_ngpr_dirty(u32 nreg) { ngpr[nreg].dirty = 1; }

void spill_hilo();

void map_hilo(u32 hilo);

bool is_hilo_mapped(u32 hilo);
#endif

void emit_insn(psp_insn_t insn)
{
	emit_Write32(insn.word);
}

u32* GetRegPtr(u32 reg)
{
	return Sh4_int_GetRegisterPtr((Sh4RegType)reg);
}

/*
	psp static reg alloc
	s0 - pointer to the sh4 context
	s1 - cycle counter
	s2 - bRun ptr
	a0 - next pc
*/
const psp_gpr_t psp_ctx_reg = psp_gp;
const psp_gpr_t psp_cycle_reg = psp_k0;
const psp_gpr_t psp_bRun_reg = psp_s2;
const psp_gpr_t psp_next_pc_reg = psp_a0;

#include <stdarg.h>

//1+n opcodes
void emit_mpush(u32 n, ...)
{
	va_list ap;
	va_start(ap, n);
	emit_addiu(psp_sp, psp_sp, u16(-4*n));
	while (n--)
	{
		u32 reg = va_arg(ap, u32);
#if 0
		if (reg < reg_fpr)
			emit_sw(reg-reg_gpr,psp_sp,u16(4*n));
		else
			emit_swc1(reg-reg_fpr,psp_sp,u16(4*n));
#else
		emit_sw(reg,psp_sp,u16(4*n));
#endif
	}
	va_end(ap);
}
//1+n opcodes
void emit_mpop(u32 n, ...)
{
    va_list ap;
    va_start(ap, n);
    for (u32 i = n; i--> 0;)
    {
        u32 reg = va_arg(ap, u32);
#if 0
        if (reg < reg_fpr)
            emit_lw(reg-reg_gpr,psp_sp,u16(4*i));
        else
            emit_lwc1(reg-reg_fpr,psp_sp,u16(4*i));
#else
        emit_lw(reg,psp_sp,u16(4*i));
#endif
    }
    va_end(ap);
    emit_addiu(psp_sp, psp_sp, u16(+4*n));
}
//2 opcodes
void emit_li(u32 reg,u32 data,u32 sz=0)
{
	if (is_u16(data) && sz!=2)
	{
		emit_ori(reg,psp_zero,data&0xFFFF);
	}
	else if (is_s16(data) && sz!=2)
	{
		emit_addiu(reg,psp_zero,data&0xFFFF);
	}
	else
	{
		emit_lui(reg,data>>16);
		if ((sz==2) || (data&0xFFFF))
		{
			verify(sz!=1);
			emit_ori(reg,reg,data&0xFFFF);
		}
	}
}
//load upper base
//1 opcode
u32 emit_lub(u32 reg,u32 data)
{
	u32 hi = data>>16;
	u32 lo = data&0xFFFF;

	if (lo & 0x8000)
	{
		hi++;
		lo = data - (hi<<16); 
	}
	emit_lui(reg,u16(hi));
	return lo;
}
//load upper address base
//1 opcode
u32 emit_luab(u32 reg,void*data) { return emit_lub(reg,(u32)data); }
//2 opcodes
void emit_la(u32 reg,void* data)
{
	emit_li(reg,(u32)data);
}
//2 opcodes
void emit_lba(u32 reg,void* data)
{
	u32 lo = emit_lub(reg,(u32)data);
	emit_lb(reg,reg,lo);
}
//2 opcodes
void emit_lha(u32 reg,void* data)
{
	u32 lo = emit_lub(reg,(u32)data);
	emit_lh(reg,reg,lo);
}
//2 opcodes
void emit_lwa(u32 reg,void* data)
{
	u32 lo = emit_lub(reg,(u32)data);
	emit_lw(reg,reg,lo);
}
//2 opcodes
void emit_swa(u32 reg1,u32 reg2,void* data)
{
	u32 lo = emit_lub(reg2,(u32)data);
	emit_sw(reg1,reg2,lo);
}
//1 opcode
void emit_sh_load(u32 rt,u32 sh4_reg)
{
	emit_lw(rt,psp_ctx_reg,Sh4cntx.offset(sh4_reg));
}
void emit_sh_load(u32 rt,shil_param prm)
{
	verify(prm.is_reg());
	emit_sh_load(rt,prm._reg);
}
void emit_sh_load_hu(u32 rt,u32 sh4_reg)
{
	emit_lhu(rt,psp_ctx_reg,Sh4cntx.offset(sh4_reg));
}
void emit_sh_load_hu(u32 rt,shil_param prm)
{
	verify(prm.is_reg());
	emit_sh_load_hu(rt,prm._reg);
}
//1 opcode
void emit_sh_addr(u32 rt,u32 sh4_reg)
{
	emit_addiu(rt,psp_ctx_reg,Sh4cntx.offset(sh4_reg));
}
void emit_sh_addr(u32 rt,shil_param prm)
{
	verify(prm.is_reg());
	emit_sh_addr(rt,prm._reg);
}
//1 opcode
void emit_sh_store(u32 rt,u32 sh4_reg)
{
	emit_sw(rt,psp_ctx_reg,Sh4cntx.offset(sh4_reg));
}
void emit_sh_store(u32 rt,shil_param prm)
{
	verify(prm.is_reg());
	emit_sh_store(rt,prm._reg);
}
//1 opcode
void emit_sh_lvq(u32 vt,u32 sh4_reg)
{
	emit_lvq(vt,psp_ctx_reg,Sh4cntx.offset(sh4_reg));
}
void emit_sh_lvq(u32 vt,shil_param prm)
{
	verify(prm.is_r32fv()==4);
	emit_sh_lvq(vt,prm._reg);
}
//1 opcode
void emit_sh_svq(u32 vt,u32 sh4_reg)
{
	emit_svq(vt,psp_ctx_reg,Sh4cntx.offset(sh4_reg));
}
void emit_sh_svq(u32 vt,shil_param prm)
{
	verify(prm.is_r32fv()==4);
	emit_sh_svq(vt,prm._reg);
}
//1 opcode
void emit_sh_lvs(u32 vt,u32 sh4_reg)
{
	emit_lvs(vt,psp_ctx_reg,Sh4cntx.offset(sh4_reg));
}
void emit_sh_lvs(u32 vt,shil_param prm)
{
	verify(prm.is_r32f());
	emit_sh_lvs(vt,prm._reg);
}
//1 opcode
void emit_sh_svs(u32 vt,u32 sh4_reg)
{
	emit_svs(vt,psp_ctx_reg,Sh4cntx.offset(sh4_reg));
}
void emit_sh_svs(u32 vt,shil_param prm)
{
	verify(prm.is_r32f());
	emit_sh_svs(vt,prm._reg);
}

const u32 djump_temp=psp_s8;	//djump_reg or djump_cond

void* loop_no_update;
void* ngen_LinkBlock_Static_stub;
void* ngen_LinkBlock_Dynamic_1st_stub;
void* ngen_LinkBlock_Dynamic_2nd_stub;
void* ngen_BlockCheckFail_stub;
void* loop_do_update_write;
void (*loop_code)() ;
void (*ngen_FailedToFindBlock)();

struct
{
	bool has_jcond;

	void Reset()
	{
		has_jcond=false;
	}
} compile_state;
u32 last_block;

void ngen_Begin(DecodedBlock* block,bool force_checks)
{
	compile_state.Reset();

	//emit_la(psp_a0,emit_GetCCPtr());
	//emit_la(psp_a1,&last_block);
	//emit_sw(psp_a0,psp_a1,0);

	if (force_checks)
	{
		u32 addr=block->start;
		if (addr&3)
		{
			addr&=~3;
			if (block->opcodes>1)
				addr+=4;
		}
		u32* ptr=(u32*)GetMemPtr(addr,4);
		
		emit_li(psp_a0,*ptr);
		u32 low=emit_luab(psp_a1,ptr);
		emit_lw(psp_a1,psp_a1,low);

		u32* jdst=(u32*)emit_GetCCPtr();
		jdst+=4;
		emit_beq(psp_a0,psp_a1,jdst);		//same ?
		low=emit_lub(psp_a0,block->start);	//delay slot, first step of loading block->start

		emit_j(ngen_BlockCheckFail_stub);		//jump
		emit_addiu(psp_a0,psp_a0,low);		//delay slot, 2nd step of loading block->start
		//jump dst is here
	}

	u32* jdst=(u32*)emit_GetCCPtr();
	jdst+=6;
	emit_bgez(psp_cycle_reg,jdst);	//if cycles <0 break the chain, esle skip 5 opcodes
	emit_addiu(psp_cycle_reg,psp_cycle_reg,-block->cycles);	//sub cycles		| 1
	
	emit_li(psp_next_pc_reg,block->start,2);				//load pc			| 2& 3
	emit_j(loop_do_update_write);							//jumpity jump		| 4
	emit_addiu(psp_cycle_reg,psp_cycle_reg,block->cycles);	//re-add cycles		| 5
}

//static !
void DoStatic(u32 target)
{
	emit_lui(psp_a0,target>>16);
	emit_jal(ngen_LinkBlock_Static_stub);
	emit_ori(psp_a0,psp_a0,target&0xFFFF);
}
void* FASTCALL ngen_LinkBlock_Static(u32 pc,u32* patch)
{
	next_pc=pc;
	
	DynarecCodeEntry* rv=rdv_FindOrCompile();
	
	emit_ptr=patch;
	{
		emit_j(rv);
		emit_nop();
	}
	emit_ptr=0;
	make_address_range_executable((u32)patch, (u32)(patch+2));

	return (void*)rv;
}

//Dynamic block: 1st time -> call stub & modify to compare
//On compare fail -> jump directly
void DoDynamic(u32 target)
{
	emit_jal(ngen_LinkBlock_Dynamic_1st_stub);
	emit_move(psp_a0,djump_temp);

	emit_Skip(4*6);

	/*
	//guess mode:
	li(psp_a0,expect_addr);		//2
	bne djump_temp,psp_a0,fail	//1
	nop();						//1
	j expect_dst				//1
	nop();						//1
	fail:	
	jal DynamicFallbackStub		//1
	nop()						//1

	total: 8 opcodes

	//fallback:
	emit_j(loop_no_update);
	emit_move(psp_a0,djump_temp);
	*/
}

void* FASTCALL ngen_LinkBlock_Dynamic_1st(u32 pc,u32* patch)
{
	next_pc=pc;
	
	DynarecCodeEntry* rv=rdv_FindOrCompile();
	
	emit_ptr=patch;
	{
		emit_li(psp_a0,pc);		//2
		u32* dst=(u32*)emit_GetCCPtr()+2;
		dst+=2;
		emit_bne(djump_temp,psp_a0,dst);	//1
		emit_nop();							//1
		emit_j(rv);							//1
		emit_nop();							//1
		//fail:	
		emit_jal(ngen_LinkBlock_Dynamic_2nd_stub);		//1
		emit_nop();						//1
	}
	make_address_range_executable((u32)patch, (u32)emit_ptr);
	emit_ptr=0;

	return (void*)rv;
}

void* FASTCALL ngen_LinkBlock_Dynamic_2nd(u32 pc,u32* patch)
{
	next_pc=pc;
	
	DynarecCodeEntry* rv=rdv_FindOrCompile();
	
	emit_ptr=patch;
	{
		emit_j(loop_no_update);
		emit_move(psp_a0,djump_temp);
	}
	make_address_range_executable((u32)patch, (u32)emit_ptr);
	emit_ptr=0;

	return (void*)rv;
}
void ngen_End(DecodedBlock* block)
{
	switch(block->BlockType)
	{
	case BET_Cond_0:
	case BET_Cond_1:
		{
			//printf("COND %d\n",block->BlockType&1);

			u32 reg;
			if (compile_state.has_jcond)
			{
				reg=djump_temp;
			}
			else
			{
				reg=psp_a0;
				emit_sh_load(psp_a0,reg_sr_T);
			}

			u32* target=(u32*)emit_GetCCPtr();
			target+=2; // for the branches
			target+=3;	//skip 3 opcodes

			if (block->BlockType&1)
			{	//==1 -> branch block else skip to next block
				emit_beq(reg,psp_zero,target);
			}
			else
			{	//==0 -> branch block else skip to next block
				emit_bne(reg,psp_zero,target);
			}
			
			emit_nop();	//delay slot

			DoStatic(block->BranchBlock);

			DoStatic(block->NextBlock);
		}
		break;

	case BET_DynamicCall:
	case BET_DynamicJump:
	case BET_DynamicRet:
		//printf("Dynamic !\n");
		DoDynamic(block->NextBlock);
		break;

	case BET_StaticIntr:
	case BET_DynamicIntr:
		u32 reg;
		//printf("Interrupt !\n");
		if (block->BlockType==BET_StaticIntr)
		{
			emit_li(psp_a0,block->BranchBlock);
			reg=psp_a0;
		}
		else
		{
			reg=djump_temp;
		}
		emit_jal(UpdateINTC);
		emit_sh_store(reg,reg_nextpc);	//delay slot

		emit_j(loop_no_update);
		emit_sh_load(psp_next_pc_reg,reg_nextpc);	//deay slot

		break;

	case BET_StaticCall:
	case BET_StaticJump:
		//printf("Static 0x%08X!\n",block->BranchBlock);
		DoStatic(block->BranchBlock);
		break;
		
	default:
		die("invalid end mode");
	}
}

#define ngen_Bin_nostore(bin_op,bin_opi,is,tf)	\
{	bool short_form=false;\
	emit_sh_load(psp_a0,op->rs1);	\
	\
	if (op->rs2.is_imm())	\
	{	\
		if (is_##is##16(tf op->rs2._imm))	\
		{	\
			short_form=true;	\
			emit_##bin_opi(psp_a0,psp_a0,tf op->rs2._imm);	\
		}	\
		else	\
		{	\
			emit_li(psp_a1,op->rs2._imm);	\
		}	\
	}	\
	else if (op->rs2.is_r32i())	\
	{	\
		emit_sh_load(psp_a1,op->rs2._reg);	\
	}	\
	else	\
	{	\
		printf("%d \n",op->rs2.type);	\
		verify(false);	\
	}	\
	\
	if (!short_form) emit_##bin_op(psp_a0,psp_a0,psp_a1);	\
}


#define ngen_Bin(bin_op,bin_opi,is,tf)	\
{	\
	ngen_Bin_nostore(bin_op,bin_opi,is,tf) \
	emit_sh_store(psp_a0,op->rd._reg);	\
}

#define ngen_Unary(un_op)	\
{	\
	emit_sh_load(psp_a0,op->rs1);	\
	\
	if (op->rs1.is_r32i())	\
	{	\
		emit_sh_load(psp_a1,op->rs1);	\
	}	\
	else	\
	{	\
		printf("%d \n",op->rs1.type);	\
		verify(false);	\
	}	\
	\
	emit_##un_op(psp_a0,psp_a1);	\
	emit_sh_store(psp_a0,op->rd);	\
}

void* _vmem_read_const(u32 addr,bool& ismem,u32 sz);

bool readwriteparams(shil_opcode* op,bool& wasram,void*& ptr)
{
	if (op->rs1.is_imm())
	{
		verify(op->op==shop_readm);
		ptr=_vmem_read_const(op->rs1._imm,wasram,op->flags);
		return true;
	}
	else
	{
		emit_sh_load(psp_a0,op->rs1);

		if (!op->rs3.is_null())
		{
			if (op->rs3.is_imm())
			{
				if (op->rs3._imm==0)
				{
					printf("op->rs3 == 0:: wtf?\n");
				}
				else if (is_s16(op->rs3._imm))
				{
					emit_addiu(psp_a0,psp_a0,op->rs3._imm);
				}
				else
				{
					emit_li(psp_a3,op->rs3._imm);
					emit_addiu(psp_a0,psp_a0,psp_a3);
				}
			}
			else if (op->rs3.is_r32i())
			{
				emit_sh_load(psp_a3,op->rs3);
				emit_addu(psp_a0,psp_a0,psp_a3);
			}
			else
			{
				die("invalid rs3");
			}			
		}
		return false;
	}

}
u32 emit_SlideDelay() { emit_Skip(-4); u32 rv=*(u32*)emit_GetCCPtr();  return rv;}
/*
	fpu helper stuff
*/
void r_fadd(f32* fn,f32* fm)	{ *fn += *fm;	}
void r_fsub(f32* fn,f32* fm)	{ *fn -= *fm;	}
void r_fmul(f32* fn,f32* fm)	{ *fn *= *fm;	}
void r_fdiv(f32* fn,f32* fm)	{ *fn /= *fm;	}
u32 r_fcmp_gt(f32* fn,f32* fm)	{ return *fn > *fm ? 1:0; }
u32 r_fcmp_eq(f32* fn,f32* fm)	{ return *fn == *fm ? 1:0; }

void r_fsca(f32* fn,u32 pi_index)
{
	__asm__ volatile (
		"mtv	%2, S100\n"
		"vi2f.s S100, S100, 14\n"
		"vrot.p C000, S100, [s,c]\n"
		"sv.s	S000, %0\n"
		"sv.s	S001, %1\n"
	: "=m" ( *fn ),"=m" ( *(fn+1) ) : "r" ( pi_index ) );
}

void r_fsrra(f32* fn)
{
	__asm__ 
		(
	".set			push\n"
	".set			noreorder\n"
	"lv.s			s000,  %1\n"
	"vrsq.s			s000, s000\n"
	"sv.s			s000, %0\n"
	".set			pop\n"
	: "=m"(*fn)
	: "m"(*fn)
		);
}

void r_fipr(f32* fn,f32* fm)
{
	__asm__ 
		(
	".set			push\n"
	".set			noreorder\n"
	"lv.q			c110,  %1\n"
	"lv.q			c120,  %2\n"
	"vdot.q			s000, c110, c120\n"
	"sv.s			s000, %0\n"
	".set			pop\n"
	: "=m"(*(fn+3))
	: "m"(*fn),"m"(*fm)
		);
}

void r_fsqrt(f32* fn)
{
	__asm__ 
		(
		".set			push\n"
		".set			noreorder\n"
		"lv.s			s000,  %1\n"
		"vsqrt.s		s000, s000\n"
		"sv.s			s000, %0\n"
		".set			pop\n"
		: "=m"(*fn)
		: "m"(*fn)
		);
}

//rs1+rs2*rs3
void r_fmac(f32* fn,f32* f0,f32* fm)
{
	__asm__ 
		(
		".set			push\n"
		".set			noreorder\n"
		"lv.s			s001,  %0\n"
		"lv.s			s000,  %1\n"
		"lv.s			s010,  %2\n"
		"vhdp.p			s000, c010, c000\n"
		"sv.s			s000, %0\n"
		".set			pop\n"
		: "+m"(*fn)
		: "m"(*f0), "m"(*fm)
		);
	//*fn =*fn+*f0* *fm;
}

int r_f2i_t(f32* fn)
{
	return (int)*fn;
}
void r_i2f_z(f32* dst,int src)
{
	*dst=src;
}
void r_i2f_n(f32* dst,int src)
{
	*dst=src;
}

void r_ftrv(f32* fn,f32* mtrx)
{
	__asm__ 
		(
	".set			push\n"
	".set			noreorder\n"
	"lv.q			c100,  %1\n"
	"lv.q			c110,  %2\n"
	"lv.q			c120,  %3\n"
	"lv.q			c130,  %4\n"
	"lv.q			c200,  %5\n"
	"vtfm4.q		c000, e100, c200\n"
	"sv.q			c000, %0\n"
	".set			pop\n"
	: "=m"(*fn)
	: "m"(*mtrx),"m"(*(mtrx+4)),"m"(*(mtrx+8)),"m"(*(mtrx+12)), "m"(*fn)
		);
}

void FASTCALL do_sqw_mmu(u32 dst);
void FASTCALL do_sqw_nommu(u32 dst);

void ngen_CC_Start(shil_opcode* op) { die("Unsuported for psp.."); }
void ngen_CC_Param(shil_opcode* op,shil_param* par,CanonicalParamType tp) { die("Unsuported for psp.."); }
void ngen_CC_Call(shil_opcode*op,void* function) { die("Unsuported for psp.."); }
void ngen_CC_Finish(shil_opcode* op) { die("Unsuported for psp.."); }

DynarecCodeEntry* ngen_Compile(DecodedBlock* block,bool force_checks)
{
	if (emit_FreeSpace()<16*1024)
		return 0;
	
	DynarecCodeEntry* rv=(DynarecCodeEntry*)emit_GetCCPtr();
	
	ngen_Begin(block,force_checks);

	for (size_t i=0;i<block->oplist.size();i++)
	{
		shil_opcode* op=&block->oplist[i];
		switch(op->op)
		{
		case shop_ifb:
			{
				if (op->rs1._imm)
				{
					emit_li(psp_a0,op->rs2._imm);
					emit_sh_store(psp_a0,reg_nextpc);
				}
				emit_jal(OpPtr[op->rs3._imm]);
				emit_li(psp_a0,op->rs3._imm,1);//delayslot must be single opcode
			}
			break;

		case shop_jdyn:
			{
				emit_sh_load(djump_temp,op->rs1);
				if (op->rs2.is_imm())
				{
					if (is_s16(op->rs2._imm))
					{
						emit_addiu(djump_temp,djump_temp,op->rs2._imm);
					}
					else
					{
						emit_li(psp_a1,op->rs2._imm);
						emit_addu(djump_temp,djump_temp,psp_a1);
					}
				}
			}
			break;

		case shop_jcond:
			{
				compile_state.has_jcond=true;
				emit_sh_load(djump_temp,op->rs1);
			}
			break;

		case shop_mov64:
			{
				emit_sh_load(psp_a0,op->rs1._reg);
				emit_sh_load(psp_a1,op->rs1._reg+1);

				emit_sh_store(psp_a0,op->rd._reg);
				emit_sh_store(psp_a1,op->rd._reg+1);
			}
			break;

		case shop_mov32:
			{
				if (op->rs1.is_imm())
				{
					emit_li(psp_a0,op->rs1._imm);
				}
				else if (op->rs1.is_r32())
				{
					emit_sh_load(psp_a0,op->rs1);
				}
				else
				{
					goto defaulty;
				}

				emit_sh_store(psp_a0,op->rd);
			}
			break;

		case shop_readm:
			{
				//die("shop_readm");
				void* fuct=0;
				bool wasram=false;
				void* ptr;

				if (readwriteparams(op,wasram,ptr))
				{
					if (wasram)
					{
						u32 offs=emit_luab(psp_a0,ptr);
						if (op->flags==1)
						{
							//x86e->Emit(op_movsx8to32,EAX,ptr);
							emit_lb(psp_v0,psp_a0,offs);
						}
						else if (op->flags==2)
						{
							//x86e->Emit(op_movsx16to32,EAX,ptr);
							emit_lh(psp_v0,psp_a0,offs);
						}
						else if (op->flags==4)
						{
							//x86e->Emit(op_mov32,EAX,ptr);
							emit_lw(psp_v0,psp_a0,offs);
						}
						else
						{
							die("Invalid mem read size");
						}
					}
					else
					{
						fuct=ptr;
						printf("%08X\n",op->rs1);
						die("not done quite yet!");
					}
				}
				
				if (!wasram)
				{
					u32 delay=emit_SlideDelay();
					switch(op->flags)
					{
						case 1:
							if (!fuct) fuct=(void*)ReadMem8;
							emit_jal(fuct);
							emit_Write32(delay);	//dslot
							emit_seb(psp_v0,psp_v0);
							break;
						case 2:
							if (!fuct) fuct=(void*)ReadMem16;
							emit_jal(fuct);
							emit_Write32(delay);	//dslot
							emit_seh(psp_v0,psp_v0);
							break;
						case 4:
							if (!fuct) fuct=(void*)ReadMem32;
							emit_jal(fuct);
							emit_Write32(delay);	//dslot
							break;
						case 8:
							if (!fuct) fuct=(void*)ReadMem64;
							emit_jal(fuct);
							emit_Write32(delay);	//dslot
							break;
					}
				}

				emit_sh_store(psp_v0,op->rd._reg);
				if (op->flags==8)
					emit_sh_store(psp_v1,op->rd._reg+1);
			}
			break;

		case shop_writem:
			{
				//die("shop_writem");
				bool wasram;
				void* ptr;

				if (readwriteparams(op,wasram,ptr))
				{
					die("not supported yet");
				}

				if (op->flags==8)
					emit_sh_load(psp_a3,op->rs2._reg+1);

				switch(op->flags)
				{
					case 1:
						emit_jal(&WriteMem8);
						break;
					case 2:
						emit_jal(&WriteMem16);
						break;
					case 4:
						emit_jal(&WriteMem32);
						break;
					case 8:
						emit_jal(&WriteMem64);
						break;
					default:
						verify(false);
				}

				//<delay slot>
				if (op->flags!=8)
					emit_sh_load(psp_a1,op->rs2);
				else
					emit_sh_load(psp_a2,op->rs2._reg);
				//</delay slot>
				
			}
			break;

		case shop_sync_sr:
			{
				emit_jal(UpdateSR);
				emit_nop();
			}
			break;

		case shop_shl:
			ngen_Bin(sllv,sll,u,+);
			break;
		case shop_shr:
			ngen_Bin(srlv,srl,u,+);
			break;
		case shop_sar:
			ngen_Bin(srav,sra,u,+);
			break;
		case shop_ror:
			ngen_Bin(rotrv,rotr,u,+);
			break;

		case shop_shad:
		case shop_shld:
			{
				/*
					//emit_srav//emit_sllv//emit_srlv

					if (x>=0)
						emit_sllv (<<x)
					else if (x&0x1f)
						emit_neg x
						emit_srlv/emit_srav (>>-x)
					else
						0/emit_sra(31)
					
					x,in
					nx=-x;
					zx=x&1f;
					s1=in<<x;
					s2=in>>nx;
					s3=0/in>>31;
					
					s23=zx==0?s3:s2;
					b=x<0?1:0;
					s1=b!=0?s23:s1;
				*/

				/*
					a0: val
					a1: shift
					a2: -shift
					a3: shift&1f
					t3: shift<0

					t0: a0<<a1 (shift)
					t1: a0>>a2 (-shift)
					t2: 0/a0>>31
				*/
				psp_gpr_t val=psp_a0,shft=psp_a1,negshft=psp_a2,shft_1f=psp_a3;
				psp_gpr_t sh_l=psp_t0,sh_r=psp_t1,sh_z=psp_t2,isltz=psp_t3;

				emit_sh_load(shft,op->rs2);
				emit_sh_load(val,op->rs1);
				emit_negu(negshft,shft);
				emit_andi(shft_1f,shft,0x1F);

				emit_slt(isltz,shft,psp_zero);

				emit_sllv(sh_l,val,shft);

				if (op->op==shop_shad)
				{
					emit_srav(sh_r,val,negshft);
					emit_sra(sh_z,val,31);
				}
				else
				{
					emit_srlv(sh_r,val,negshft);
					sh_z=psp_zero;
				}

				emit_slti(shft_1f,shft_1f,1);	//shft_1f=shft_1f<1?1:0	//for some odd reason using just movz doesn't work ...
				emit_movn(sh_r,sh_z,shft_1f);	//if (shft_1f==0) sh_r=sh_z;
				//s1=b!=0?s23:s1;
				emit_movn(sh_l,sh_r,isltz);		//if (shft<0) sh_l=sh_r;//if (isltz!=0) sh_l=sh_r

				emit_sh_store(sh_l,op->rd);
			}
			break;

		case shop_and:
			ngen_Bin(and,andi,u,+);
			break;

		case shop_or:
			ngen_Bin(or,ori,u,+);
			break;

		case shop_xor:
			ngen_Bin(xor,xori,u,+);
			break;

		case shop_add:
			ngen_Bin(addu,addiu,s,+);
			break;
		case shop_sub:
			ngen_Bin(subu,addiu,s,-);
			break;

		case shop_neg:
			ngen_Unary(negu);
			break;
			
		case shop_not:
			ngen_Unary(not);
			break;

		case shop_test:
			{
				ngen_Bin_nostore(and,andi,u,+);
				emit_sltiu(psp_a0,psp_a0,1);//if a0==0 -> a0=1 else a0=0
				emit_sh_store(psp_a0,op->rd);
			}
			break;
			
		case shop_seteq:
			{
				verify(op->rs1.is_r32i());
				verify(op->rs2.is_r32i() || (op->rs2.is_imm() && is_s16(-op->rs2._imm)));

				u32 rs1,rs2,rf;
				rs1=psp_a0;
				rs2=psp_a1;
				rf=psp_at;

				emit_sh_load(rs1,op->rs1);
				

				if (op->rs2.is_r32i())
				{
					emit_sh_load(rs2,op->rs2);
					emit_xor(rf,rs1,rs2);
				}
				else
				{
					if (op->rs2._imm==0)
					{
						rf=psp_a0;
					}
					else
					{
						emit_addiu(rf,rs1,-op->rs2._imm);
					}
				}
				
				//if rf ==0 -> at=1; else at=0
				emit_sltiu(psp_at,rf,1);
				emit_sh_store(psp_at,op->rd);
			}
			break;

		case shop_setge:
		case shop_setgt:
		case shop_setae:
		case shop_setab:
			{
				verify(op->rs1.is_r32i());
				verify(op->rs2.is_r32i() || (op->rs2.is_imm() && op->rs2._imm==0));

				bool flip=op->op==shop_setge || op->op==shop_setae;
				bool usgnd=op->op>=shop_setae;

				u32 rs1=psp_a0;
				u32 rs2=psp_a1;

				emit_sh_load(rs1,op->rs1);

				if (op->rs2.is_imm())
					rs2=psp_zero;
				else
					emit_sh_load(rs2,op->rs2);

				if (flip) 
				{	//swap operands !
					u32 t=rs1;
					rs1=rs2;
					rs2=t;
				}

				if (usgnd)
				{	//unsigned
					emit_sltu(psp_at,rs2,rs1);
				}
				else
				{	//signed
					emit_slt(psp_at,rs2,rs1);
				}

				if (flip)
				{
					emit_xori(psp_at,psp_at,1);
				}
				emit_sh_store(psp_at,op->rd);
			}
			break;

				//fpu
		case shop_fadd:
			{
				verify(op->rs1._reg==op->rd._reg);
#if FULL_VFPU
				u32 vd = op->rd; vd = (vd&3)*32|(vd&12)>>2;
				u32 vt = op->rs2; vt = (vt&3)*32|(vt&12)>>2;
				emit_vadds(vd, vt, vd);
#elif HALF_VFPU
				emit_sh_lvs(0, op->rd);
				emit_sh_lvs(1, op->rs2);
				emit_vadds(0, 1, 0);
				emit_sh_svs(0, op->rd);
#else
				emit_sh_addr(psp_a0,op->rd);
				emit_jal(r_fadd);
				emit_sh_addr(psp_a1,op->rs2);
#endif
			}
			break;
		case shop_fsub:
			{
				verify(op->rs1._reg==op->rd._reg);
#if FULL_VFPU
				u32 vd = op->rd; vd = (vd&3)*32|(vd&12)>>2;
				u32 vt = op->rs2; vt = (vt&3)*32|(vt&12)>>2;
				emit_vsubs(vd, vt, vd);
#elif HALF_VFPU
				emit_sh_lvs(0, op->rd);
				emit_sh_lvs(1, op->rs2);
				emit_vsubs(0, 1, 0);
				emit_sh_svs(0, op->rd);
#else
				emit_sh_addr(psp_a0,op->rd);
				emit_jal(r_fsub);
				emit_sh_addr(psp_a1,op->rs2);
#endif
			}
			break;
		case shop_fmul:
			{
				verify(op->rs1._reg==op->rd._reg);
#if FULL_VFPU
				u32 vd = op->rd; vd = (vd&3)*32|(vd&12)>>2;
				u32 vt = op->rs2; vt = (vt&3)*32|(vt&12)>>2;
				emit_vmuls(vd, vt, vd);
#elif HALF_VFPU
				emit_sh_lvs(0, op->rd);
				emit_sh_lvs(1, op->rs2);
				emit_vmuls(0, 1, 0);
				emit_sh_svs(0, op->rd);
#else
				emit_sh_addr(psp_a0,op->rd);
				emit_jal(r_fmul);
				emit_sh_addr(psp_a1,op->rs2);
#endif
			}
			break;
		case shop_fdiv:
			{
				verify(op->rs1._reg==op->rd._reg);
#if FULL_VFPU
				u32 vd = op->rd; vd = (vd&3)*32|(vd&12)>>2;
				u32 vt = op->rs2; vt = (vt&3)*32|(vt&12)>>2;
				emit_vrcps(vt, 31);
				emit_vmuls(vd, 31, vd);
#elif HALF_VFPU
				emit_sh_lvs(0, op->rd);
				emit_sh_lvs(1, op->rs2);
				emit_vrcps(1, 1);
				emit_vmuls(0, 1, 0);
				emit_sh_svs(0, op->rd);
#else
				emit_sh_addr(psp_a0,op->rd);
				emit_jal(r_fdiv);
				emit_sh_addr(psp_a1,op->rs2);
#endif
			}
			break;
			
		case shop_fabs:
			{
				//die("shop_fabs");
#if FULL_VFPU
				u32 vd = op->rd; vd = (vd&3)*32|(vd&12)>>2;
				u32 vs = op->rs1; vs = (vs&3)*32|(vs&12)>>2;
				emit_vabss(vs, vd);
#elif HALF_VFPU
				emit_sh_lvs(0, op->rs1);
				emit_vabss(0, 0);
				emit_sh_svs(0, op->rd);
#else
				emit_sh_load(psp_a0,op->rs1);
				emit_li(psp_at,0x7FFFFFFF);
				emit_and(psp_a0,psp_a0,psp_at);
				emit_sh_store(psp_a0,op->rd);
#endif
			}
			break;

		case shop_fneg:
			{
				//die("shop_fneg");
#if FULL_VFPU
				u32 vd = op->rd; vd = (vd&3)*32|(vd&12)>>2;
				u32 vs = op->rs1; vs = (vs&3)*32|(vs&12)>>2;
				emit_vnegs(vs, vd);
#elif HALF_VFPU
				emit_sh_lvs(0, op->rs1);
				emit_vnegs(0, 0);
				emit_sh_svs(0, op->rd);
#else
				emit_sh_load(psp_a0,op->rs1);
				emit_li(psp_at,0x80000000);
				emit_xor(psp_a0,psp_a0,psp_at);
				emit_sh_store(psp_a0,op->rd);
#endif
			}
			break;

		case shop_fsca:
			{
				//die("shop_fsca");
#if FULL_VFPU
				u32 vd = op->rd; vd = (vd&3)*32|(vd&12)>>2;
				u32 vs = op->rs1; vs = (vs&3)*32|(vs&12)>>2;
				emit_vi2fs(vs, 31, 14);
				emit_vrotp_sc(31, vd);
#elif 0 && HALF_VFPU
				emit_sh_lvs(0, op->rs1);
				emit_vi2fs(0, 4, 14);
				emit_vrotp_sc(4, 0);
				emit_sh_svs(0, op->rd + 0);
				emit_sh_svs(32, op->rd + 1);
#else
				emit_sh_load_hu(psp_a1,op->rs1);
				emit_jal(r_fsca);
				emit_sh_addr(psp_a0,op->rd);
#endif
			}
			break;

		case shop_fipr:
			{
				//die("shop_fipr");
				verify(op->rs1._reg+3==op->rd._reg);
#if FULL_VFPU
				//emit_vdotq(vs, vt, vd);
#elif HALF_VFPU
				/*
				"lv.q			c110,  %1\n"
				"lv.q			c120,  %2\n"
				"vdot.q			s000, c110, c120\n"
				"sv.s			s000, %0\n"
				*/
				emit_sh_lvq(5, op->rs1);
				emit_sh_lvq(6, op->rs2);
				emit_vdotq(5, 6, 0);
				emit_sh_svs(0, op->rd);
#else
				emit_sh_addr(psp_a0,op->rs1);
				emit_jal(r_fipr);
				emit_sh_addr(psp_a1,op->rs2);
#endif
			}
			break;

		case shop_fsqrt:
			{
				//die("shop_fsqrt");
				verify(op->rs1._reg==op->rd._reg);
#if FULL_VFPU
				u32 vs = op->rs1; vs = (vs&3)*32|(vs&12)>>2;
				u32 vd = op->rd;  vd = (vd&3)*32|(vd&12)>>2;
				emit_vsqrts(vs, vd);
#elif 0 && HALF_VFPU
				emit_sh_lvs(0, op->rd);
				emit_vsqrts(0, 0);
				emit_sh_svs(0, op->rd);
#else
				emit_jal(r_fsqrt);
				emit_sh_addr(psp_a0,op->rd);
#endif
			}
			break;

		case shop_ftrv:
			{
				//die("shop_ftrv");
				verify(op->rs1._reg==op->rd._reg);
#if FULL_VFPU
#elif HALF_VFPU
				/*
				"lv.q			c100,  %1\n"
				"lv.q			c110,  %2\n"
				"lv.q			c120,  %3\n"
				"lv.q			c130,  %4\n"
				"lv.q			c200,  %5\n"
				"vtfm4.q		c000, e100, c200\n"
				"sv.q			c000, %0\n"
				*/
				u32* p=(u32*)emit_GetCCPtr();

				emit_sh_lvq(4, op->rs2._reg+0);		//4=c100
				emit_sh_lvq(5, op->rs2._reg+4);		//5=c110
				emit_sh_lvq(6, op->rs2._reg+8);		//6=c120
				emit_sh_lvq(7, op->rs2._reg+12);		//7=c130
				emit_sh_lvq(8, op->rs1);		//8=c200
				emit_vtfm4q(36, 8, 0);			//36 is e100, 8 is c200, 0 is c000
				emit_sh_svq(0, op->rd);			//0 is c000

				//printf("%08X %08X %08X %08X %08X %08X %08X",p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
#else
				emit_sh_addr(psp_a0,op->rd);
				emit_jal(r_ftrv);
				emit_sh_addr(psp_a1,op->rs2);
#endif
			}
			break;

		case shop_fmac:
			{
				//die("shop_fmac");
				verify(op->rs1._reg==op->rd._reg);
#if FULL_VFPU
#elif HALF_VFPU
				emit_sh_lvs(0, op->rs2); // f0
				emit_sh_lvs(1, op->rs3); // fm
				emit_sh_lvs(2, op->rs1); // fn
				emit_vmuls(0, 1, 0);      // fm * f0 
				emit_vadds(0, 2, 0);      // fm * f0 + fn 
				emit_sh_svs(0, op->rd);
#else
				emit_sh_addr(psp_a0,op->rd);
				emit_sh_addr(psp_a1,op->rs2);
				emit_jal(r_fmac);
				emit_sh_addr(psp_a2,op->rs3);
#endif
			}
			break;

		case shop_fsrra:
			{
				//die("shop_fsrra");
				verify(op->rs1._reg==op->rd._reg);
#if FULL_VFPU
#elif 0 && HALF_VFPU
				emit_sh_lvs(0, op->rd);
				emit_vrsqs(0, 0);
				emit_sh_svs(0, op->rd);
#else
				emit_jal(r_fsrra);
				emit_sh_addr(psp_a0,op->rd);
#endif
			}
			break;

		case shop_fseteq:
		case shop_fsetgt:
			{
#if FULL_VFPU
				u32 vs = op->rs1; vs = (vs&3)*32|(vs&12)>>2;
				u32 vt = op->rs2; vt = (vt&3)*32|(vt&12)>>2;
				if (op->op==shop_fseteq)
				{
					emit_vslts(vs, vt, 126);
					emit_vslts(vt, vs, 127);
					emit_vscmps(126, 127, 127);
				}
				else
				{
					emit_vscmps(vs, vt, 127);
				}
				//emit_vabss(127, 127);
				emit_sh_svs(127, op->rd);
#elif 0 && HALF_VFPU
				emit_sh_lvs(0, op->rs1);
				emit_sh_lvs(1, op->rs2);
				if (op->op==shop_fseteq)
				{
					emit_vsges(0, 1, 2);
					emit_vslts(1, 0, 3);
					emit_vsubs(2, 3, 0);
				}
				else
				{
					emit_vslts(1, 0, 0);
				}
				emit_vf2izs(0, 0, 0);
				emit_sh_svs(0, op->rd);
#else
				emit_sh_addr(psp_a0,op->rs1);
				
				if (op->op==shop_fseteq)
					emit_jal(r_fcmp_eq);
				else
					emit_jal(r_fcmp_gt);

				emit_sh_addr(psp_a1,op->rs2);	//delayslot
				emit_sh_store(psp_v0,op->rd);
#endif
			}
			break;

		case shop_ext_s8:
		case shop_ext_s16:
			{
				verify(op->rd.is_r32i());
				verify(op->rs1.is_r32i());

				emit_sh_load(psp_a0,op->rs1);
				
				if (op->op==shop_ext_s8)
					emit_seb(psp_a0,psp_a0);
				else
					emit_seh(psp_a0,psp_a0);
				
				emit_sh_store(psp_a0,op->rd);
			}
			break;

		case shop_cvt_f2i_t:
			{
				verify(op->rd.is_r32i());
				verify(op->rs1.is_r32f());

				emit_jal(r_f2i_t);
				emit_sh_addr(psp_a0,op->rs1);
				emit_sh_store(psp_v0,op->rd);
			}
			break;

			//i hope that the round mode bit is set properly here :p
		case shop_cvt_i2f_n:
		case shop_cvt_i2f_z:
			{
				verify(op->rd.is_r32f());
				verify(op->rs1.is_r32i());

				emit_sh_load(psp_a1,op->rs1);

				if (op->op==shop_cvt_i2f_n)
				{
					emit_jal(r_i2f_n);
				}
				else
				{
					emit_jal(r_i2f_z);
				}

				emit_sh_addr(psp_a0,op->rd);
			}
			break;

		case shop_pref:
			{
				verify(op->rs1.is_r32i());
				//x86e->Emit(op_mov32 ,ECX,GetRegPtr(op->rs1));
				emit_sh_load(psp_a0,op->rs1);
				emit_srl(psp_a1,psp_a0,26);
				emit_li(psp_a2,0x38);

				void (fastcall * handl)(u32);

				if (CCN_MMUCR.AT)
					handl=&do_sqw_mmu;
				else
					handl=&do_sqw_nommu;

				u32* target=(u32*)emit_GetCCPtr();
				target+=4;
				emit_bne(psp_a1,psp_a2,target);
				emit_nop();
				emit_jal(handl);
				emit_nop();

				//jump target here !
			}
			break;
defaulty:
		default:
			printf("OH CRAP %d\n",op->op);
			die("Recompiled doesn't know about that opcode");
		}
	}

	ngen_End(block);

	make_address_range_executable((u32)rv, (u32)emit_GetCCPtr());
	return rv;
}



void ngen_ResetBlocks()
{
}

void ngen_mainloop()
{
	if (loop_code==0)
	{

		loop_code=(void(*)())emit_GetCCPtr();
		{
			emit_mpush(12,
				reg_gpr+psp_gp,
				reg_gpr+psp_k0,
				reg_gpr+psp_ra,
				reg_gpr+psp_s0,
				reg_gpr+psp_s1,
				reg_gpr+psp_s2,
				reg_gpr+psp_s3,
				reg_gpr+psp_s4,
				reg_gpr+psp_s5,
				reg_gpr+psp_s6,
				reg_gpr+psp_s7,
				reg_gpr+psp_s8);

#if FULL_VFPU
			emit_li(psp_at, (u32)fr_hex);
			emit_lvq(0, psp_at, reg_fr_0-reg_fr_0);
			emit_lvq(1, psp_at, reg_fr_4-reg_fr_0);
			emit_lvq(2, psp_at, reg_fr_8-reg_fr_0);
			emit_lvq(3, psp_at, reg_fr_12-reg_fr_0);
			emit_lvq(4, psp_at, reg_xf_0-reg_fr_0);
			emit_lvq(5, psp_at, reg_xf_4-reg_fr_0);
			emit_lvq(6, psp_at, reg_xf_8-reg_fr_0);
			emit_lvq(7, psp_at, reg_xf_12-reg_fr_0);
#endif
			//cntx base
			emit_la(psp_ctx_reg,&Sh4cntx);
			
			//brun ptr
			//emit_la(psp_bRun_reg,(void*)&sh4_int_bCpuRun);

			//load pc
			emit_sh_load(psp_next_pc_reg,reg_nextpc);
			
			//and cycles
			emit_li(psp_cycle_reg,SH4_TIMESLICE);
			
			//next_pc _MUST_ be on ecx
	//no_update
	loop_no_update=emit_GetCCPtr();
#if 1
			verify(is_u16(BM_BLOCKLIST_MASK<<(BM_BLOCKLIST_SHIFT)));
			
			emit_andi(psp_a1,psp_a0,BM_BLOCKLIST_MASK<<(BM_BLOCKLIST_SHIFT));
			//a1: idx offset
			u32 offs=emit_luab(psp_a2,cache);	//load base top 16 to a2
			emit_addu(psp_a1,psp_a1,psp_a2);	//add em (a1+a2)
			emit_lw(psp_a1,psp_a1,offs);		//add lower 16 and load pointer to a1
			
			//a1: block ptr
			emit_lw(psp_a3,psp_a1,4);	//pc addr
			u32* dst=(u32*)emit_GetCCPtr()+2;
			dst+=4;
			emit_bne(psp_a3,psp_a0,dst);
			emit_lw(psp_a2,psp_a1,0);	//code ptr

			emit_lw(psp_a3,psp_a1,8);		//load count
			emit_addiu(psp_a3,psp_a3,1);	//inc

			emit_jr(psp_a2);
			emit_sw(psp_a3,psp_a1,8);		//store count
#endif
			emit_jal(&bm_GetCode);
			emit_nop();		//delayslot

			emit_jr(psp_v0);
			emit_nop();		//delayslot
	//do_update_write
	loop_do_update_write=emit_GetCCPtr();

			emit_sh_store(psp_next_pc_reg,reg_nextpc);
			//next_pc _MUST_ be on ram

	//do_update
	//loop_do_update=emit_GetCCPtr();

			emit_jal(UpdateSystem);
			emit_addiu(psp_cycle_reg,psp_cycle_reg,SH4_TIMESLICE);	//delayslot

			emit_lba(psp_at,(void*)&sh4_int_bCpuRun);
			emit_bne(psp_at,psp_zero,loop_no_update);
			emit_sh_load(psp_next_pc_reg,reg_nextpc);	//delayslot

#if FULL_VFPU
			emit_li(psp_at, (u32)fr_hex);
			emit_svq(0, psp_at, reg_fr_0-reg_fr_0);
			emit_svq(1, psp_at, reg_fr_4-reg_fr_0);
			emit_svq(2, psp_at, reg_fr_8-reg_fr_0);
			emit_svq(3, psp_at, reg_fr_12-reg_fr_0);
			emit_svq(4, psp_at, reg_xf_0-reg_fr_0);
			emit_svq(5, psp_at, reg_xf_4-reg_fr_0);
			emit_svq(6, psp_at, reg_xf_8-reg_fr_0);
			emit_svq(7, psp_at, reg_xf_12-reg_fr_0);
#endif

	//cleanup
			emit_mpop(12,
				reg_gpr+psp_gp,
				reg_gpr+psp_k0,
				reg_gpr+psp_ra,
				reg_gpr+psp_s0,
				reg_gpr+psp_s1,
				reg_gpr+psp_s2,
				reg_gpr+psp_s3,
				reg_gpr+psp_s4,
				reg_gpr+psp_s5,
				reg_gpr+psp_s6,
				reg_gpr+psp_s7,
				reg_gpr+psp_s8);

			emit_jra();
			emit_nop();//delay slot
		
		} //that was mainloop


		//ngen_FailedToFindBlock
		ngen_FailedToFindBlock=(void(*)())emit_GetCCPtr();
		{
			emit_jal(&rdv_FailedToFindBlock);	
			emit_nop();		//delay slot

			emit_jr(psp_v0);
			emit_nop();		//delay slot
		}

		ngen_LinkBlock_Static_stub=emit_GetCCPtr();
		{
			emit_addiu(psp_a1,psp_ra,(u16)-12);
			emit_jal(&ngen_LinkBlock_Static);	
			emit_nop();		//delay slot

			emit_jr(psp_v0);
			emit_nop();		//delay slot
		}

		ngen_LinkBlock_Dynamic_1st_stub=emit_GetCCPtr();
		{
			emit_addiu(psp_a1,psp_ra,(u16)-8);
			emit_jal(&ngen_LinkBlock_Dynamic_1st);	
			emit_nop();		//delay slot

			emit_jr(psp_v0);
			emit_nop();		//delay slot
		}

		ngen_LinkBlock_Dynamic_2nd_stub=emit_GetCCPtr();
		{
			emit_addiu(psp_a1,psp_ra,(u16)-8);
			emit_jal(&ngen_LinkBlock_Dynamic_2nd);	
			emit_move(psp_a0,djump_temp);		//delay slot

			emit_jr(psp_v0);
			emit_nop();		//delay slot
		}

		ngen_BlockCheckFail_stub=emit_GetCCPtr();
		{
			emit_jal(&rdv_BlockCheckFail);	
			emit_nop();		//delay slot

			emit_jr(psp_v0);
			emit_nop();		//delay slot
		}

		//Make _SURE_ this code is not overwriten !
		emit_SetBaseAddr();
		/*
			char file[512];
			sprintf(file,"host0:/dynarec_%08X.bin",loop_code);
			FILE* f=fopen(file,"wb");
			if (!f) { sprintf(file,"dynarec_%08X.bin",loop_code); f=fopen(file,"wb"); }
			fwrite((void*)loop_code,1,CODE_SIZE-emit_FreeSpace(),f);
			fclose(f);
		*/

		make_address_range_executable((u32)loop_code, (u32)emit_GetCCPtr());
	}

	loop_code();
}

#if 1
void regcache_reset()
{
	for (int i = 0; i < EREGS_NUM; ++i)
	{
		eregs[i].nreg = 255;
	}

	for (int i = 0; i < NGPR_NUM; ++i)
	{
		ngpr[i].locked = 1;
		ngpr[i].mapped = 0;
		ngpr[i].dirty  = 0;
		ngpr[i].ereg   = 255;
	}

	for (int i = 0; i < 2; ++i)
	{
		nhilo[i].mapped = 0;
		nhilo[i].dirty  = 0;
	}

	for (int i = 0; i < OREGS_NUM; ++i)
	{
		u32 nreg = oregs[i];

		ngpr[nreg].locked = 0;
	}
}

void spill_ngpr(u32 n)
{
	return;
	u32 ereg, nreg;

	for (u32 i = 0; i < OREGS_NUM; ++i)
	{
		nreg = oregs[i];
		ereg = ngpr[nreg].ereg;

		if (!ngpr[nreg].locked && ngpr[nreg].mapped && ereg != 255)
		{
			if (ngpr[nreg].dirty)
				emit_sh_store(nreg,ereg);

			eregs[ereg].nreg   = 255;
			ngpr[nreg].ereg   = 255;
			ngpr[nreg].dirty  = 0;
			ngpr[nreg].mapped = 0;
			n--;
			if (!n) break;
		}
	}
}

static u32 __ngpr = psp_a0;

u32 map_ngpr(u32 ereg, u32 action)
{
	u32 nreg, old_nreg = 255, i;

	nreg = eregs[ereg].nreg;

	// already mapped ?
	if (nreg != 255)
	{
		if (ngpr[nreg].mapped && (ngpr[nreg].ereg != 255))
		{
			// if no remap, just return the already associated native register
			if (!(action & NREG_REMAP))
				return nreg;

			ngpr[nreg].locked = 1;

			old_nreg = nreg;
		}
	}

redo:
	nreg = 255;

	for (i = 0; i < OREGS_NUM; ++i)
	{
		nreg = oregs[i];

		// new native register should be not locked or mapped to be a candidate
		if (!ngpr[nreg].locked && !ngpr[nreg].mapped && (ngpr[nreg].ereg == 255))
			break;
	}

	// no candidate ? so we need to spill a native register
	if (i == OREGS_NUM)
	{
		spill_ngpr(1);
		goto redo;
	}

	ngpr[nreg].ereg   = ereg;
	ngpr[nreg].mapped = 1;
	ngpr[nreg].locked = 0;
	ngpr[nreg].dirty  = 0;
	eregs[ereg].nreg   = nreg;

	if (action & NREG_PRELOAD)
		emit_sh_load(nreg,ereg);

	// unlock and free the old native register
	if (old_nreg != 255)
	{
		ngpr[old_nreg].locked = 0;
		ngpr[old_nreg].mapped = 0;
		ngpr[old_nreg].dirty  = 0;
		ngpr[old_nreg].ereg   = 255;
	}

	return nreg;
}

u32 unmap_ngpr(u32 ereg, bool update)
{
	u32 nreg = eregs[ereg].nreg;

	if (nreg < NGPR_NUM)
	{
		if (ngpr[nreg].mapped)
		{
			if (update && ngpr[nreg].dirty)
				emit_sh_store(nreg,ereg);

			ngpr[nreg].ereg   = 255;
			ngpr[nreg].dirty  = 0;
			ngpr[nreg].mapped = 0;
			if (eregs[ereg].nreg == nreg)
				eregs[ereg].nreg = 255;
		}
	}

	return nreg;
}

void lock_ngpr(u32 ereg)
{
	u32 nreg = eregs[ereg].nreg;

	if (nreg < NGPR_NUM)
		ngpr[nreg].locked = 1;
}

void unlock_ngpr(u32 ereg)
{
	u32 nreg = eregs[ereg].nreg;

	if (nreg < NGPR_NUM)
		ngpr[nreg].locked = 0;
}

void spill_hilo()
{
	return;
	for (u32 i = 0; i < 2; ++i)
	{
		if (nhilo[i].mapped)
		{
			if (nhilo[i].dirty)
			{
				nhilo[i].dirty  = 0;
				if (i)
					emit_mfhi(psp_at);
				else
					emit_mflo(psp_at);
				emit_sh_store(psp_at,reg_macl-i);
			}

			nhilo[i].mapped = 0;
		}
	}
}

void map_hilo(u32 hilo)
{
	return;
	nhilo[hilo].mapped = 1;
}

bool is_hilo_mapped(u32 hilo)
{
	return false;
	return nhilo[hilo].mapped;
}
#endif

void ngen_GetFeatures(ngen_features* dst)
{
	dst->InterpreterFallback=false;
	dst->OnlyDynamicEnds=false;
}