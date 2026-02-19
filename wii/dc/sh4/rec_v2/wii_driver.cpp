/*
	wii dynarec
	based on the mips one !

	PPC/Wii calling rules:
	Registers:
		32 32-bit gprs
			r0     volatile, temp
			r1     stack pointer, grows down
			r2     TOC (what ?) Pointer (who cares)
		    r3:10  volatile, first 8 params.r3 is also return value
		    r11    volatile (used as 'environment' pointers for calls by ptr .. what?)
		    r12    volatile (used for ming (mingw ?) & magic as well as linking)
		    r13:31 preserved (19 of em)
		
		32 64-bit fprs (single, vector or double)
		    f0     volatile, scratch
			f1:4   volatile, params, return
			f5:13  volatile, params
			f14:31 preserved (18 of em)
		LR  Link 
		XER (exception register)
		FPSCR
		CR (CR0:1;CR5:7 volatile, CR2:4 preserved)

	When calling 

	Call stack: 
		Return is stored on the link register and its saved at a specific location on function entry
		lr is stored on sp+4
		stack grows towards zero

*/
#include "types.h"
#include "dc\sh4\sh4_opcode_list.h"

#include "dc\sh4\sh4_registers.h"
#include "dc\sh4\ccn.h"
#include "dc\sh4\rec_v2\ngen.h"
#include "dc\mem\sh4_mem.h"
#include "emitter\PPCEmit\ppc_emitter.h"

// Define "invalid" value for non-mapped-registery
const ppc_freg ppc_finvalid = (ppc_freg)-1;  // or a out of range value (ex: 32+)
const ppc_ireg ppc_rinvalid = (ppc_ireg)-1;  // or a out of range value (ex: 32+)

// This is defined in main.cpp
extern "C" int get_debug_loop();

// ppc_li: Loads a 32-bit immediate value into a PowerPC register.
void ppc_li(u32 D,u32 imm)
{
	if (is_s16(imm))
	{
		ppc_addi(D,0,imm);
		return;
	}
	else
	{
		ppc_addis(D,0,imm>>16);
		ppc_ori(D,D,(u16)imm);
	}
}

// =======================
// JUMP OFFSET CALCULATION
// =======================

snat ppc_jdiff_raw(void* dst)
{
	return (u8*)dst-(u8*)emit_GetCCPtr();
}
snat ppc_jdiff(void* dst)
{
	return ppc_jdiff_raw(dst)>>2;
}

void ppc_bx(void* dst,u32 LK)
{
	snat offs = ppc_jdiff_raw(dst);
	//offs must fit in 24 bits
	verify(offs<33554432 && offs>-33554432);
	offs>>=2;
	// does this work ? //
	ppc_bx(offs,0,LK);
}

void ppc_call(void* funct)
{
	ppc_bx(funct,1);
}
template<typename T> void ppc_call(T* dst) { return ppc_call((void*)dst); }

void ppc_jump(void* funct)
{
	ppc_bx(funct,0);
}
template<typename T> void ppc_jump(T* dst) { return ppc_jump((void*)dst); }
void ppc_call_and_jump(void* funct)
{
	ppc_call(funct);
	ppc_mtctr(ppc_r3);
	ppc_bctr();
}
template<typename T> void ppc_call_and_jump(T* dst) { return ppc_call_and_jump((void*)dst); }
void make_address_range_executable(void* addr, u32 size)
{
	//what gives?
	DCFlushRange(addr, size);
	ICInvalidateRange(addr, size);
}

void ppc_lip(u32 D,void* ptr)
{
	ppc_li(D,(u8*)ptr-(u8*)0);
}
template<typename T> void ppc_lip(T* ptr) { return ppc_lip((void*)ptr); }

ppc_ireg ppc_cycles = ppc_r29;
ppc_ireg ppc_contex = ppc_r30;
ppc_ireg ppc_djump = ppc_r31;
ppc_ireg ppc_next_pc = ppc_rarg0;

void ppc_emit(u32 insn)
{
	emit_Write32(insn);
}

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

// =======================
// BLOCK BEGIN/END
// =======================

void ngen_Begin(DecodedBlock* block,bool force_checks)
{
	compile_state.Reset();

	ppc_addic(ppc_cycles,ppc_cycles,-block->cycles,1);
	
	ppc_label* jdst=ppc_CreateLabel();
	ppc_bcx(BO_FALSE,BI_CR0_LT,0,0,0);

	ppc_li(ppc_next_pc,block->start);
	ppc_jump(loop_do_update_write);

	jdst->MarkLabel();
}

// =====================
// MEMORY ACCESS HELPERS
// =====================

//1 opcode
void ppc_sh_load(u32 D,u32 sh4_reg)
{
	ppc_lwz(D,ppc_contex,Sh4cntx.offset(sh4_reg));
}
void ppc_sh_load(u32 D,shil_param prm)
{
	verify(prm.is_reg());
	ppc_sh_load(D,prm._reg);
}
void ppc_sh_load_f32(u32 D,u32 sh4_reg)
{
	ppc_lfs(D,ppc_contex,Sh4cntx.offset(sh4_reg));
}
void ppc_sh_load_f32(u32 D,shil_param prm)
{
	verify(prm.is_reg());
	ppc_sh_load_f32(D,prm._reg);
}
void ppc_sh_load_u16(u32 D,u32 sh4_reg)
{
	ppc_lhz(D,ppc_contex,Sh4cntx.offset(sh4_reg));
}
void ppc_sh_load_u16(u32 D,shil_param prm)
{
	verify(prm.is_reg());
	ppc_sh_load_u16(D,prm._reg);
}
//1 opcode
void ppc_sh_addr(u32 D,u32 sh4_reg)
{
	ppc_addi(D,ppc_contex,Sh4cntx.offset(sh4_reg));
}
void ppc_sh_addr(u32 D,shil_param prm)
{
	verify(prm.is_reg());
	ppc_sh_addr(D,prm._reg);
}
//1 opcode
void ppc_sh_store(u32 D,u32 sh4_reg)
{
	ppc_stw(D,ppc_contex,Sh4cntx.offset(sh4_reg));
}
void ppc_sh_store(u32 D,shil_param prm)
{
	verify(prm.is_reg());
	ppc_sh_store(D,prm._reg);
}
void ppc_sh_store_f32(u32 D,u32 sh4_reg)
{
	ppc_stfs(D,ppc_contex,Sh4cntx.offset(sh4_reg));
}
u32 ppc_addr_high(u32 rD,void* ptr)
{
	unat diff=(u8*)ptr-(u8*)0;
	u32 rv=(s32)(s16)diff;
	diff-=rv;
	ppc_addis(rD,0,diff>>16);

	return rv;
}

void ppc_sh_store_f32(u32 D,shil_param prm)
{
	verify(prm.is_reg());
	ppc_sh_store_f32(D,prm._reg);
}

// ==========================
// CALLING CONVENTION ADAPTER
// ==========================

struct CC_PS
{
	CanonicalParamType type;
	shil_param* par;
};
vector<CC_PS> CC_pars;
void ngen_CC_Start(shil_opcode* op) 
{ 
	CC_pars.clear();
}
void ngen_CC_Param(shil_opcode* op,shil_param* par,CanonicalParamType tp) 
{
	switch(tp)
	{
		case CPT_f32rv:
			ppc_sh_store_f32(ppc_frv0, *par);
			break;

		case CPT_u32rv:
		case CPT_u64rvL:
			ppc_sh_store(ppc_rrv0, *par);
			break;

		case CPT_u64rvH:
			ppc_sh_store(ppc_rrv1, *par);
			break;

		case CPT_u32:
		case CPT_ptr:
		case CPT_f32:
			{
				CC_PS t={tp,par};
				CC_pars.push_back(t);
			}
			break;

		default:
			die("invalid tp");
	}
}
void ngen_CC_Call(shil_opcode*op,void* function) 
{
	u32 rd_fp=ppc_farg0;
	u32 rd_gpr=ppc_rarg0;
	for (int i=CC_pars.size();i-->0;)
	{
		if (CC_pars[i].type==CPT_ptr)
		{
			ppc_sh_addr(rd_gpr,*CC_pars[i].par);
		}
		else
		{
			if (CC_pars[i].par->is_reg())
			{
				if (CC_pars[i].type==CPT_f32)
				{
					ppc_sh_load_f32(rd_fp,*CC_pars[i].par);
					rd_fp++;
				}
				else
				{
					ppc_sh_load(rd_gpr,*CC_pars[i].par);
				}
			}
			else
				ppc_li(rd_gpr,CC_pars[i].par->_imm);
		}
		rd_gpr++;
	}
	//printf("used reg r0 to r%d, %d params, calling %08X\n",rd-1,CC_pars.size(),function);
	ppc_call(function);
}

// =================
// BINARY OPERATIONS
// =================

void binop_start(shil_opcode* op)
{
	verify(!op->rs1.is_null() && !op->rs2.is_null() && !op->rd.is_null());

	verify(op->rs1.is_reg());
	//verify(!op->rs2.is_imm() || op->rs2.is_imm_s16());
	
	ppc_sh_load(ppc_rarg0,op->rs1);

	if (op->rs2.is_imm())
	{
		ppc_li(ppc_rarg1,op->rs2._imm);
	}
	else if (op->rs2.is_reg())
	{
		ppc_sh_load(ppc_rarg1,op->rs2);
	}
}

void binop_end(shil_opcode* op)
{
	ppc_sh_store(ppc_rarg0,op->rd);
}

void binop_start_fpu(shil_opcode* op)
{
	verify(!op->rs1.is_null() && !op->rs2.is_null() && !op->rd.is_null());

	verify(op->rs1.is_reg());
	verify(op->rs2.is_reg());
	
	ppc_sh_load_f32(ppc_farg0,op->rs1);
	ppc_sh_load_f32(ppc_farg1,op->rs2);
}

void binop_end_fpu(shil_opcode* op)
{
	ppc_sh_store_f32(ppc_farg0,op->rd);
}


void ngen_CC_Finish(shil_opcode* op) 
{ 
	CC_pars.clear(); 
}
void DoStatic(u32 pc)
{
	ppc_li(ppc_rarg0,pc);
	ppc_call(ngen_LinkBlock_Static_stub);
}

// ====================================
// ngen_End: Block Exit Code Generation
// ====================================

void ngen_End(DecodedBlock* block)
{
	switch(block->BlockType)
	{
	case BET_Cond_0:
	case BET_Cond_1:
		{
			//printf("COND %d\n",block->BlockType&1);
			//die("not supported");
			u32 reg;
			if (compile_state.has_jcond)
			{
				reg=ppc_djump;
			}
			else
			{
				reg=ppc_rarg0;
				ppc_sh_load(ppc_rarg0,reg_sr_T);
			}
			
			ppc_cmpi(ppc_cr0,reg,block->BlockType&1,0);
	
			ppc_label* jtrue=ppc_CreateLabel();
			ppc_bcx(BO_TRUE,BI_CR0_EQ,0,0,0);

			DoStatic(block->NextBlock);
			jtrue->MarkLabel();
			DoStatic(block->BranchBlock);
		}
		break;

	case BET_DynamicCall:
	case BET_DynamicJump:
	case BET_DynamicRet:
		//printf("Dynamic !\n");
		//mov reg,djump
		ppc_mov(ppc_rarg0,ppc_djump);
		//jmp no update
		ppc_jump(loop_no_update);
		break;

	case BET_StaticIntr:
	case BET_DynamicIntr:
		printf("BET: Interrupt !\n");
		{
			u32 reg;
			if (block->BlockType==BET_StaticIntr)
			{
				ppc_li(ppc_rarg0,block->BranchBlock);
				reg=ppc_rarg0;
			}
			else
			{
				reg=ppc_djump;
			}
			ppc_sh_store(reg,reg_nextpc);
			ppc_call(&UpdateINTC);

			ppc_sh_load(ppc_next_pc,reg_nextpc);
			ppc_jump(loop_no_update);
		}
		break;

	case BET_StaticCall:
	case BET_StaticJump:
    // DOLPHIN ERROR LOOP
    if(get_debug_loop() == 1){
		  printf("Static 0x%08X!\n",block->BranchBlock);
    }
		DoStatic(block->BranchBlock);
		break;

	default:
		printf("END TYPE: %d\n",block->BlockType);
		die("wtfh end type\n");
	}
}

//f14:f29
ppc_freg GetFloatReg(u32 reg)
{
	if (reg>=reg_fr_0 && reg<=reg_fr_15)
	{
		return (ppc_freg)((reg-reg_fr_0)+ppc_f14);
	}

	return ppc_finvalid;
}

//r14:r28 (shr11 missing)
ppc_ireg GetIntReg(u32 reg)
{
	if (reg>=reg_r0 && reg<=reg_r15 && reg!=reg_r11)
	{
		if (reg>=reg_r11) reg--;

		return (ppc_ireg)((reg-reg_r0 )+ppc_r14);
	}

	return ppc_rinvalid;
}
void reg_flush_all()
{
	/*
	for(u32 i=0;i<=sh4_reg_count;i++)
	{
		ppc_ireg ri=GetIntReg(i);
		ppc_freg rf=GetFloatReg(i);
		if (rf!=ppc_finvalid)
			ppc_sh_store_f32(rf,i);
		else if (ri!=ppc_rinvalid)
			ppc_sh_store(ri,i);
	}
	*/
}
void reg_reload_all()
{
	/*
	for(u32 i=0;i<=sh4_reg_count;i++)
	{
		ppc_ireg ri=GetIntReg(i);
		ppc_freg rf=GetFloatReg(i);
		if (rf!=ppc_finvalid)
			ppc_sh_load_f32(rf,i);
		else if (ri!=ppc_rinvalid)
			ppc_sh_load(ri,i);
	}
	*/
}
void FASTCALL do_sqw_mmu(u32 dst);
void FASTCALL do_sqw_nommu(u32 dst);

// =====================
// OPERATION COMPILATION
// =====================

// ngen_CompileBlock: Main Block Compilation Loop
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

		case shop_readm:
			{
				void* fuct=0;
				bool isram=false;
				verify(op->rs1.is_imm() || op->rs1.is_r32i());

				if (op->rs1.is_imm())
				{
					void* ptr=_vmem_read_const(op->rs1._imm,isram,op->flags);
					if (isram)
					{
						if (op->flags==1)
						{
							ppc_lbz(ppc_r3,ppc_r4,ppc_addr_high(ppc_r4,ptr));
							ppc_extsbx(ppc_r3,ppc_r3,0);
						}	
						else if (op->flags==2)
							ppc_lha(ppc_r3,ppc_r4,ppc_addr_high(ppc_r4,ptr));
						else if (op->flags==4)
							ppc_lwz(ppc_r3,ppc_r4,ppc_addr_high(ppc_r4,ptr));
						else
						{
							die("Invalid mem read size");
						}
					}
					else
					{
						ppc_li(ppc_rarg0,op->rs1._imm);
						fuct=ptr;
					}
				}
				else
				{
					ppc_sh_load(ppc_rarg0,op->rs1);
					if (op->rs3.is_imm())
					{
						verify(op->rs3.is_imm_s16());
						ppc_addi(ppc_rarg0,ppc_rarg0,op->rs3._imm);
					}
					else if (op->rs3.is_r32i())
					{
						ppc_sh_load(ppc_rarg1,op->rs3);
						ppc_addx(ppc_rarg0,ppc_rarg0,ppc_rarg1,0,0);
					}
					else if (!op->rs3.is_null())
					{
						die("invalid rs3");
					}
				}

				if (!isram)
				{
					switch(op->flags)
					{
					case 1:
						if (!fuct) fuct=(void*)ReadMem8;
						ppc_call(fuct);
						ppc_extsbx(ppc_rrv0,ppc_rrv0,0);
						break;
					case 2:
						if (!fuct) fuct=(void*)ReadMem16;
						ppc_call(fuct);
						ppc_extshx(ppc_rrv0,ppc_rrv0,0);
						break;
					case 4:
						if (!fuct) fuct=(void*)ReadMem32;
						ppc_call(fuct);
						break;
					case 8:
						if (!fuct) fuct=(void*)ReadMem64;
						ppc_call(fuct);
						break;
					default:
						verify(false);
					}
				}
				
				ppc_sh_store(ppc_rrv0,op->rd);

				if (op->flags==8)
				{
					ppc_sh_store(ppc_rrv1,op->rd._reg+1);
				}
			}
			break;

		case shop_writem:
			{
				ppc_sh_load(ppc_rarg0,op->rs1);
				

				if (op->flags==8)
				{
					ppc_sh_load(ppc_rarg2,op->rs2);
					ppc_sh_load(ppc_rarg3,op->rs2._reg+1);
				}
				else
					ppc_sh_load(ppc_rarg1,op->rs2);

				if (op->rs3.is_imm())
				{
					verify(op->rs3.is_imm_s16());
					ppc_addi(ppc_rarg0,ppc_rarg0,op->rs3._imm);
				}
				else if (op->rs3.is_r32i())
				{
					ppc_sh_load(ppc_rarg3,op->rs3);

					ppc_addx(ppc_rarg0,ppc_rarg0,ppc_rarg3,0,0);
				}
				else if (!op->rs3.is_null())
				{
					printf("rs3: %08X\n",op->rs3.type);
					die("invalid rs3");
				}

				switch(op->flags)
				{
				case 1:
					ppc_andi(ppc_rarg1,ppc_rarg1,0xFF);
					ppc_call(&WriteMem8);
					break;
				case 2:
					ppc_andi(ppc_rarg1,ppc_rarg1,0xFFFF);
					ppc_call(&WriteMem16);
					break;
				case 4:
					ppc_call(&WriteMem32);
					break;
				case 8:
					ppc_call(&WriteMem64);
					break;
				default:
					die("invalid size on memwrite");
				}
			}
			break;

		case shop_ifb:
			{
				reg_flush_all();
				if (op->rs1._imm)
				{
					ppc_li(ppc_rarg0,op->rs2._imm);
					ppc_sh_store(ppc_rarg0,reg_nextpc);
				}
				ppc_li(ppc_rarg0,op->rs3._imm);
				ppc_call(OpDesc[op->rs3._imm]->oph);
				reg_reload_all();
			}
			break;
			
		case shop_jdyn:
			{
				ppc_sh_load(ppc_djump,op->rs1);
				
				if (op->rs2.is_imm())
				{
					if (op->rs2.is_imm_s16())
					{
						ppc_addi(ppc_djump,ppc_djump,op->rs2._imm);
					}
					else
					{
						ppc_li(ppc_rarg0,op->rs2._imm);
						ppc_addx(ppc_djump,ppc_djump,ppc_rarg0,0,0);
					}
				}
			}
			break;
			
		case shop_jcond:
			{
				compile_state.has_jcond=true;
				ppc_sh_load(ppc_djump,op->rs1);
			}
			break;
			
		case shop_mov64:
			{
				verify(op->rd.is_r64());
				verify(op->rs1.is_r64());

				ppc_sh_load(ppc_rarg0,op->rs1);
				ppc_sh_load(ppc_rarg1,op->rs1._reg+1);

				ppc_sh_store(ppc_rarg0,op->rd);
				ppc_sh_store(ppc_rarg1,op->rd._reg+1);
			}
			break;

		case shop_mov32:
			{
				verify(op->rd.is_r32());

				if (op->rs1.is_imm())
				{
					ppc_li(ppc_rarg0,op->rs1._imm);
				}
				else if (op->rs1.is_r32())
				{
					ppc_sh_load(ppc_rarg0,op->rs1);
				}
				else
				{
					die("Invalid mov32 size");
				}

				ppc_sh_store(ppc_rarg0,op->rd);
			}
			break;

		case shop_add: binop_start(op); ppc_addx(ppc_rarg0,ppc_rarg0,ppc_rarg1,0,0); binop_end(op); break;
		case shop_sub: binop_start(op); ppc_subfx(ppc_rarg0,ppc_rarg1,ppc_rarg0,0,0); binop_end(op); break;
		
		case shop_or: binop_start(op); ppc_orx(ppc_rarg0,ppc_rarg0,ppc_rarg1,0); binop_end(op); break;
		case shop_and: binop_start(op); ppc_andx(ppc_rarg0,ppc_rarg0,ppc_rarg1,0); binop_end(op); break;
		case shop_xor: binop_start(op); ppc_xorx(ppc_rarg0,ppc_rarg0,ppc_rarg1,0); binop_end(op); break;

		case shop_shl: binop_start(op); ppc_slwx(ppc_rarg0,ppc_rarg0,ppc_rarg1,0); binop_end(op); break;
		case shop_shr: binop_start(op); ppc_srwx(ppc_rarg0,ppc_rarg0,ppc_rarg1,0); binop_end(op); break;
		case shop_sar: binop_start(op); ppc_srawx(ppc_rarg0,ppc_rarg0,ppc_rarg1,0); binop_end(op); break;
		case shop_mul_i32: binop_start(op); ppc_mullwx(ppc_rarg0,ppc_rarg0,ppc_rarg1,0,0); binop_end(op); break;


		case shop_fadd: binop_start_fpu(op); ppc_faddsx(ppc_farg0,ppc_farg0,ppc_farg1,0); binop_end_fpu(op); break;
		case shop_fsub: binop_start_fpu(op); ppc_fsubsx(ppc_farg0,ppc_farg0,ppc_farg1,0); binop_end_fpu(op); break;
		case shop_fmul: binop_start_fpu(op); ppc_fmulsx(ppc_farg0,ppc_farg0,ppc_farg1,0); binop_end_fpu(op); break;
		case shop_fdiv: binop_start_fpu(op); ppc_fdivsx(ppc_farg0,ppc_farg0,ppc_farg1,0); binop_end_fpu(op); break;


		default:
			//canonical fallback ~
			shil_chf[op->op](op);
			break;

defaulty:
			printf("OH CRAP %d\n",op->op);
			die("Recompiler doesn't know about that opcode");
		}
	}

	ngen_End(block);

	make_address_range_executable((u8*)rv, (u8*)emit_GetCCPtr()-(u8*)rv);
	return rv;
}



void ngen_ResetBlocks()
{
}

void* FASTCALL ngen_LinkBlock_Static(u32 pc,u32* patch)
{
	next_pc=pc;
	
	DynarecCodeEntry* rv=rdv_FindOrCompile();
	
	emit_ptr=patch;
	{
		ppc_jump(rv);
	}
	emit_ptr=0;

	make_address_range_executable(patch, 1*sizeof(u32));

	return (void*)rv;
}

// =========
// MAIN LOOP
// =========

void ngen_mainloop()
{
	if (loop_code==0)
	{

		loop_code=(void(*)())emit_GetCCPtr();
		{
			/*
			create stack frame, push regsters, etc ..
			*/
			u32 stac_alloc_size=8+20*4;
			ppc_mfspr(ppc_r0,ppc_spr_lr);
			ppc_addi(ppc_sp,ppc_sp,-stac_alloc_size);
			
			//store link register
			ppc_stw(ppc_r0,ppc_sp,stac_alloc_size+4);

			//store gprs
			for (int i=0;i<19;i++)
			{
				ppc_stw(ppc_r13+i,ppc_sp,stac_alloc_size-4-i*4);
			}

			/*
			pre load registers/counters etc ..
			*/
			reg_reload_all();

			//cntx base
			ppc_lip(ppc_contex,&Sh4cntx);

			//cycles
			ppc_li(ppc_cycles,SH4_TIMESLICE);

			//and pc!
			ppc_sh_load(ppc_next_pc,reg_nextpc);

			//no_update
			loop_no_update=emit_GetCCPtr();

			//handy function !
			ppc_call_and_jump(bm_GetCode);

			//do_update_write
			loop_do_update_write=emit_GetCCPtr();


			//next_pc _MUST_ be on ram since update system uses it for interrupt processing
			ppc_sh_store(ppc_next_pc,reg_nextpc);
			ppc_addi(ppc_cycles,ppc_cycles,SH4_TIMESLICE);	//add cycles ...

			ppc_call(UpdateSystem);	//call UpdateSystem
			ppc_sh_load(ppc_next_pc,reg_nextpc);
			//
			ppc_jump(loop_no_update);
			//right

			ppc_lbz(ppc_rarg0,ppc_rarg0,ppc_addr_high(ppc_rarg0,(void*)&sh4_int_bCpuRun));
			ppc_sh_load(ppc_next_pc,reg_nextpc);

			ppc_cmpi(ppc_cr0,ppc_rarg0,1,0);	//set flags

			//does this even work ?
			//ppc_bcx(BO_TRUE,BI_CR0_EQ,ppc_jdiff(loop_no_update),0,0);
			

			/*
			//write back registers and stuff ...
			*/

			//cleanup
			/*
			Clean up the stack frame and return ...
			*/

			//restore link register
			ppc_lwz(ppc_r0,ppc_sp,stac_alloc_size+4);
			ppc_mtlr(ppc_r0);

			//restore gprs 13 .. 31
			for (int i=0;i<19;i++)
			{
				ppc_lwz(ppc_r14+i,ppc_sp,stac_alloc_size-4-i*4);
			}

			

			//destroy stack frame
			ppc_addi(ppc_sp,ppc_sp,stac_alloc_size);

			//return
			ppc_blr();

		} //that was mainloop


		//ngen_FailedToFindBlock
		ngen_FailedToFindBlock=(void(*)())emit_GetCCPtr();
		{
			ppc_call_and_jump(&rdv_FailedToFindBlock);
		}

    // ====================
    // STATIC BLOCK LINKING
    // ====================

		ngen_LinkBlock_Static_stub=emit_GetCCPtr();
		{
			//not used for now
			ppc_mfspr(ppc_rarg1,ppc_spr_lr);
			
			ppc_addi(ppc_rarg1,ppc_rarg1,(u32)-12);
			ppc_call_and_jump(&ngen_LinkBlock_Static);
		}

		ngen_LinkBlock_Dynamic_1st_stub=emit_GetCCPtr();
		{
			//not used for now
		}

		ngen_LinkBlock_Dynamic_2nd_stub=emit_GetCCPtr();
		{
			//not used for now
		}

		ngen_BlockCheckFail_stub=emit_GetCCPtr();
		{
			ppc_call_and_jump(&rdv_BlockCheckFail);
		}

		//Make _SURE_ this code is not overwriten !
		emit_SetBaseAddr();
		
		char file[512];
		sprintf(file,"dynarec_%08X.bin",loop_code);
		char* path=GetEmuPath(file);

		FILE* f=fopen(path,"wb");
		free(path);

		if (f) 
		{
			fwrite((void*)loop_code,1,CODE_SIZE-emit_FreeSpace(),f);
			fflush(f);
			fclose(f);
		}
		
    // CACHE COHERENCY
    make_address_range_executable((u8*)loop_code, (u8*)emit_GetCCPtr()-(u8*)loop_code);
	}

	loop_code();
}

void ngen_GetFeatures(ngen_features* dst)
{
	dst->InterpreterFallback=false;
	dst->OnlyDynamicEnds=false;
}