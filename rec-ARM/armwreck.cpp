#include <unistd.h>
#include <sys/mman.h>
#include "types.h"

#include "dc/sh4/sh4_opcode_list.h"

#include "dc/sh4/sh4_registers.h"
#include "dc/sh4/rec_v2/ngen.h"
#include "dc/mem/sh4_mem.h"

/*

	ARM ABI
		r0~r1: scratch, params, return
		r2~r3: scratch, params
		8 regs, v6 is platoform depedant
			r4~r11
		r12 is "The Intra-Procedure-call scratch register"
		r13 stack
		r14 link
		r15 pc

		Registers s0-s15 (d0-d7, q0-q3) do not need to be preserved (and can be used for passing arguments or returning results in standard procedure-call variants).
		Registers s16-s31 (d8-d15, q4-q7) must be preserved across subroutine calls;
		Registers d16-d31 (q8-q15), if present, do not need to be preserved.

	Block linking
	Reg alloc
	Callstack cache
*/

///////////////////////////////////////
#include "exchandler.h"
//ExceptionHandler g_objExceptionHandler;
///////////////////////////////////////

#ifdef _ANDROID
#include <sys/syscall.h>  // for cache flushing.
#endif

void CacheFlush(void* code, void* pEnd)
{
#ifndef _ANDROID
	__clear_cache((void*)code, pEnd);
#else	
	void* start=code;
	size_t size=(u8*)pEnd-(u8*)start;

	//ass seen on :  google's V8 code
  // Ideally, we would call
  //   syscall(__ARM_NR_cacheflush, start,
  //           reinterpret_cast<intptr_t>(start) + size, 0);
  // however, syscall(int, ...) is not supported on all platforms, especially
  // not when using EABI, so we call the __ARM_NR_cacheflush syscall directly.

  register uint32_t beg asm("a1") = reinterpret_cast<uint32_t>(start);
  register uint32_t end asm("a2") =
      reinterpret_cast<uint32_t>(start) + size;
  register uint32_t flg asm("a3") = 0;
  #ifdef __ARM_EABI__
    #if defined (__arm__) && !defined(__thumb__)
      // __arm__ may be defined in thumb mode.
      register uint32_t scno asm("r7") = __ARM_NR_cacheflush;
      asm volatile(
          "svc 0x0"
          : "=r" (beg)
          : "0" (beg), "r" (end), "r" (flg), "r" (scno));
    #else
      // r7 is reserved by the EABI in thumb mode.
      asm volatile(
      "@   Enter ARM Mode  \n\t"
          "adr r3, 1f      \n\t"
          "bx  r3          \n\t"
          ".ALIGN 4        \n\t"
          ".ARM            \n"
      "1:  push {r7}       \n\t"
          "mov r7, %4      \n\t"
          "svc 0x0         \n\t"
          "pop {r7}        \n\t"
      "@   Enter THUMB Mode\n\t"
          "adr r3, 2f+1    \n\t"
          "bx  r3          \n\t"
          ".THUMB          \n"
      "2:                  \n\t"
          : "=r" (beg)
          : "0" (beg), "r" (end), "r" (flg), "r" (__ARM_NR_cacheflush)
          : "r3");
    #endif
  #else
    #if defined (__arm__) && !defined(__thumb__)
      // __arm__ may be defined in thumb mode.
      asm volatile(
          "svc %1"
          : "=r" (beg)
          : "i" (__ARM_NR_cacheflush), "0" (beg), "r" (end), "r" (flg));
    #else
      // Do not use the value of __ARM_NR_cacheflush in the inline assembly
      // below, because the thumb mode value would be used, which would be
      // wrong, since we switch to ARM mode before executing the svc instruction
      asm volatile(
      "@   Enter ARM Mode  \n\t"
          "adr r3, 1f      \n\t"
          "bx  r3          \n\t"
          ".ALIGN 4        \n\t"
          ".ARM            \n"
      "1:  svc 0x9f0002    \n"
      "@   Enter THUMB Mode\n\t"
          "adr r3, 2f+1    \n\t"
          "bx  r3          \n\t"
          ".THUMB          \n"
      "2:                  \n\t"
          : "=r" (beg)
          : "0" (beg), "r" (end), "r" (flg)
          : "r3");
    #endif
  #endif
	#if 0
		const int syscall = 0xf0002;
		__asm __volatile (
		"mov	 r0, %0\n"			
			"mov	 r1, %1\n"
			"mov	 r7, %2\n"
			"mov     r2, #0x0\n"
			"svc     0x00000000\n"
			:
		:	"r" (code), "r" (pEnd), "r" (syscall)
			:	"r0", "r1", "r7"
			);
	#endif
#endif
}

u32* GetRegPtr(u32 reg)
{
	return Sh4_int_GetRegisterPtr((Sh4RegType)reg);
}



#include "armwreck.h"
using namespace ARM_Wreck;

WreckAPI LoadSh4Reg(eReg Rt, shil_param Sh4_Reg, eCC CC=CC_AL)
{
	if (!Sh4_Reg.is_r32())
		printf("REG FAIL: %d\n",Sh4_Reg._reg);
	verify(Sh4_Reg.is_r32());
	LoadSh4Reg(Rt,Sh4_Reg._reg,CC);
}
WreckAPI LoadSh4Reg64(eReg Rt, shil_param Sh4_Reg, eCC CC=CC_AL)
{
	verify(Sh4_Reg.is_r64());
	//possibly use ldm/ldrd ?
	LoadSh4Reg(Rt,Sh4_Reg._reg,CC);
	LoadSh4Reg((eReg)(Rt+1),Sh4_Reg._reg+1,CC);
}
WreckAPI StoreSh4Reg(eReg Rt, shil_param Sh4_Reg, eCC CC=CC_AL)
{
	verify(Sh4_Reg.is_r32());
	StoreSh4Reg(Rt,Sh4_Reg._reg,CC);
}
WreckAPI StoreSh4Reg64(eReg Rt, shil_param Sh4_Reg, eCC CC=CC_AL)
{
	verify(Sh4_Reg.is_r64());
	//possibly use stm/strd ?
	StoreSh4Reg(Rt,Sh4_Reg._reg,CC);
	StoreSh4Reg((eReg)(Rt+1),Sh4_Reg._reg+1,CC);
}


#ifdef naked
#undef naked
#define naked __attribute__((naked))
#endif




u32 blockno=0;

u32 djump_reg;
u32 djump_cond;


const u32* _gbr = &gbr;
const u32* _next_pc = &Sh4cntx.pc;
const f32* _fr      = fr;



 
/*  THERE HAS TO BE A BETTER WAY TO DO THIS !!

#undef r
#undef r_bank
#undef gbr
#undef ssr Sh4cntx.ssr
#undef spc Sh4cntx.spc
#undef sgr Sh4cntx.sgr
#undef dbr Sh4cntx.dbr
#undef vbr Sh4cntx.vbr
#undef mach Sh4cntx.mach
#undef macl Sh4cntx.macl
#undef pr Sh4cntx.pr
#undef fpul Sh4cntx.fpul
#undef next_pc Sh4cntx.pc
#undef curr_pc (next_pc-2)
#undef sr Sh4cntx.sr
#undef fpscr Sh4cntx.fpscr
#undef old_sr Sh4cntx.old_sr
#undef old_fpscr Sh4cntx.old_fpscr
#undef fr Sh4cntx.fr
#undef xf Sh4cntx.xf
#undef fr_hex ((u32*)fr)
#undef xf_hex ((u32*)xf)
*/


extern "C" void no_update();
extern "C" void do_update();
extern "C" void do_update_write();





struct t_compile_state
{
	bool has_jcond;

	void Reset()
	{
		has_jcond=false;
	}

} compile_state;


vector<t_label> ARM_Wreck::Labels;


extern "C" void ngen_LinkBlock_Static_stub();
extern "C" void ngen_LinkBlock_cond_EQ_stub();		//mode 0
extern "C" void ngen_LinkBlock_cond_AL_stub();		//mode 1
extern "C" void ngen_LinkBlock_Dynamic_1st_stub();
extern "C" void ngen_LinkBlock_Dynamic_2nd_stub();

extern "C" u32* FASTCALL ngen_LinkBlock_Static(u32 pc, u8* patch)
{
	next_pc=pc;

	DynarecCodeEntry* rv=rdv_FindOrCompile();
	
	emit_ptr=(u32*)patch;
    //JUMP((u32)rv);
        MOV32(v1,(u32)rv);
        BX(v1);

    CacheFlush((void*)patch, emit_ptr);

    emit_ptr=0;

	return (u32*)rv;
}
extern "C" u32* FASTCALL ngen_LinkBlock_condX(u32 pc, u8* patch,u32 mode)
{
	next_pc=pc;

	DynarecCodeEntry* rv=rdv_FindOrCompile();
	
	emit_ptr=(u32*)patch;
	{
        MOV32(v1,(u32)rv);
		BX(v1,mode==0?CC_EQ:CC_AL);
	}
    CacheFlush((void*)patch, emit_ptr);
    emit_ptr=0;

	return (u32*)rv;
}

extern "C" void* ngen_LinkBlock_Dynamic_2nd(u32 pc,u32* patch)
{
	next_pc=pc;
	
	DynarecCodeEntry* rv=rdv_FindOrCompile();
	
	emit_ptr=patch;
	{
		LoadImmBase(r0,(u32)(&djump_reg));	//3
        JUMP((u32)no_update);				//1
	}
	
	CacheFlush((void*)patch, emit_ptr);
	emit_ptr=0;


	return (void*)rv;
}

extern "C" void* ngen_LinkBlock_Dynamic_1st(u32 pc,u32* patch)
{
	next_pc=pc;

	DynarecCodeEntry* rv=rdv_FindOrCompile();
	
	emit_ptr=patch;
	{
		LoadImmBase(r0,(u32)(&djump_reg));		//3
		MOV32(r1,pc);							//2*
		MOV32(r2,(unat)rv);							//2*
		CMP(r0,r1);								//1*
		BX(r2,CC_EQ);							//1*
		CALL((unat)ngen_LinkBlock_Dynamic_2nd_stub);//1
		//6*4=36 bytes
	}
	CacheFlush((void*)patch, (void*)emit_ptr);
	emit_ptr=0;
	return (void*)rv;
}





const char *GetBT_Str(u32 bt)
{
    switch(bt) {
    case BET_Cond_0:        return "BET_Cond_0";
	case BET_Cond_1:        return "BET_Cond_1";
	case BET_DynamicCall:   return "BET_DynamicCall";
	case BET_DynamicJump:   return "BET_DynamicJump";
	case BET_DynamicRet:    return "BET_DynamicRet";
	case BET_StaticIntr:    return "BET_StaticIntr";
	case BET_DynamicIntr:   return "BET_DynamicIntr";
	case BET_StaticCall:    return "BET_StaticCall";
	case BET_StaticJump:    return "BET_StaticJump";
	default: break;
    }
    return "ERROR DEFAULT BlockType!";
}

void ngen_End(DecodedBlock* block)
{
	switch(block->BlockType)
	{
	case BET_Cond_0:
	case BET_Cond_1:
	{
	    u32 _to_r2 = (u32)(compile_state.has_jcond? &djump_cond : &sr.T);

        LoadImmBase(r2, (u32)_to_r2);
        CMP(r2,(block->BlockType&1));

        //ADR(r1,12,CC_NE);   // if(NE)   r1 := PC+12
        //BX(r1,CC_NE);

        MOV32(r0,(u32)block->BranchBlock);
		CALL((u32)ngen_LinkBlock_cond_EQ_stub,CC_EQ);

        MOV32(r0,(u32)block->NextBlock);
        CALL((u32)ngen_LinkBlock_cond_AL_stub);
		break;
    }


	case BET_DynamicCall:
	case BET_DynamicJump:
	case BET_DynamicRet:
    {
        LoadImmBase(r0,(u32)(&djump_reg));
        CALL((u32)ngen_LinkBlock_Dynamic_1st_stub);
		emit_Skip(6*4);
        break;
    }

	case BET_StaticIntr:
	case BET_DynamicIntr:
	{
		if (block->BlockType==BET_StaticIntr)
		{
            StoreImms(r1,r3,(u32)&next_pc,block->BranchBlock);
		}
		else
		{
            LoadImmBase(r0,(u32)(&djump_reg));
            StoreImmBase(r0,r3,(u32)&next_pc);
		}

        CALL((u32)UpdateINTC);
        LoadImmBase(r0,(u32)&next_pc);
        JUMP((u32)no_update);
        break;
	}

	case BET_StaticCall:
	case BET_StaticJump:
    {
        MOV32(r0, (u32)block->BranchBlock);
        CALL((u32)ngen_LinkBlock_Static_stub);
        break;
    }

	default:
        printf("Error, ngen_End() Block Type: %X\n", block->BlockType);
        verify(false);
        break;
	}

    MOV32(r0, (u32)block->cycles);
    BX(lr);

}







void ngen_Begin(DecodedBlock* block)
{
	compile_state.Reset();

	CMP(rfp,0);

	MOV32(r0,block->start);
	JUMP((u32)do_update_write, CC_LE);       // jump?  if jump then block will still get called?  DAMN IM TIRED

    SUB(rfp,rfp,block->cycles);
}



void ngen_Unary(shil_opcode* op, UnaryOP unop)
{
	LoadSh4Reg(r0,op->rs1);

    unop(r0);

	StoreSh4Reg(r0,op->rd);
}


void ngen_Binary(shil_opcode* op, BinaryOP dtop, bool has_imm=true)
{
	LoadSh4Reg(r0,op->rs1);

	if (has_imm && op->rs2.is_imm())    // **FIXME** ???
	{
		MOV32(r1,(u32)op->rs2._imm);
	}
	else if (op->rs2.is_r32i())  // **FIXME** Correct?
	{
        LoadSh4Reg(r1,op->rs2);
	}
	else
	{
		printf("ngen_Bin ??? %d \n",op->rs2.type);
		verify(false);
	}

	dtop(r0, r0, r1, CC_AL);
	StoreSh4Reg(r0,op->rd);
}

void ngen_fp_bin(shil_opcode* op, FPBinOP fpop, bool has_wb=true)
{
	verify(op->rs1.is_r32f());
	verify(op->rs2.is_r32f());

	//LoadSh4Reg
	//LoadSh4Reg(r0,op->rs1);
	//VMOV(d0,r0);
	//LoadSh4Reg(r0,op->rs2);
	//VMOV(d1,r0);

	VLDR_VFP(s0,r8,op->rs1.reg_ofs()/4);
	VLDR_VFP(s1,r8,op->rs2.reg_ofs()/4);

	//if(VDIV_VFP!=fpop)
	fpop(d0,d0,d1,false);

	if (has_wb) 
	{
		VSTR_VFP(s0,r8,op->rd.reg_ofs()/4);
       // VMOV(r0,d0);
       // StoreSh4Reg(r0,op->rd);
	}
}

void* _vmem_read_const(u32 addr,bool& ismem,u32 sz);
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
		case CPT_u32rv:
		case CPT_u64rvL:
			StoreSh4Reg(r0, *par);
			break;

		case CPT_u64rvH:
			StoreSh4Reg(r1, *par);
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
	u32 rd=r0;
	for (int i=CC_pars.size();i-->0;)
	{
		if (CC_pars[i].type==CPT_ptr)
			MOV32((eReg)rd, (u32)CC_pars[i].par->reg_ptr());
		else
		{
			if (CC_pars[i].par->is_reg())
			{
				LoadSh4Reg((eReg)rd,*CC_pars[i].par);
			}
			else
				MOV32((eReg)rd, CC_pars[i].par->_imm);
		}
		rd++;
	}
	//printf("used reg r0 to r%d, %d params, calling %08X\n",rd-1,CC_pars.size(),function);
	CALL((u32)function);
}
void ngen_CC_Finish(shil_opcode* op) 
{ 
	CC_pars.clear(); 
}

DynarecCodeEntry* ngen_CompileBlock(DecodedBlock* block)
{
    if (emit_FreeSpace()<16*1024)
        return 0;

    DynarecCodeEntry* code=(DynarecCodeEntry*)emit_GetCCPtr();


    ++blockno;
    ngen_Begin(block);


    shil_opcode* op;
	for (size_t i=0;i<block->oplist.size();i++)
	{
		op=&block->oplist[i];

		switch(op->op)
		{
		case shop_readm:
		{
		    void* fuct=0;
		    bool isram=false;


			if (op->rs1.is_imm())
		    {
				void* ptr=_vmem_read_const(op->rs1._imm,isram,op->flags);

		        if (isram) 
				{
		            if (1==op->flags) 
					{
		                //LoadImmBase8(r0, (u32)ptr,true);    // true for sx
		                verify(false);
		            } 
					else if (2==op->flags) 
					{
		                LoadImmBase16(r0,(u32)ptr,true);    // true for sx
		            } 
					else if (4==op->flags) 
					{
		                LoadImmBase(r0,(u32)ptr);
                    }
					else 
						die("Unsuported size");
                } 
				else 
				{
                    fuct=ptr;
                }
		    }
		    else
		    {
		        LoadSh4Reg(r0,op->rs1);

				if (op->rs3.is_imm())
		        {
					if(op->rs3.is_imm_u8())    // *FIXME* use Check8bRot instead ! // U8?
		            {
						ADD(r0,r0,op->rs3._imm);
		            }
					else 
					{
		                MOV32(r1,op->rs3._imm);
		                ADD(r0,r0,r1);
		            }
                }
				else if (op->rs3.is_r32i())
				{
				    LoadSh4Reg(r1,op->rs3);
				    ADD(r0,r0,r1);
				}
				else if (!op->rs3.is_null())
				{
					printf("rs3: %08X\n",op->rs3.type);
					die("invalid rs3");
				}
		    }
			
		    if(!isram)
		    {
                switch(op->flags)
                {
                case 1:
                    if(!fuct) { fuct=(void*)ReadMem8; }
                    CALL((u32)fuct);
                    SXTB(r0,r0);
                    break;

                case 2:
                    if(!fuct) { fuct=(void*)ReadMem16; }
                    CALL((u32)fuct);
                    SXTH(r0,r0);
                    break;

                case 4:
                    if(!fuct) { fuct=(void*)ReadMem32; }
                    CALL((u32)fuct);
                    break;

				case 8:
                    if(!fuct) { fuct=(void*)ReadMem64; }
                    CALL((u32)fuct);
                    break;

                default:
                    verify(false);
                    break;
                }
		    }

			if (op->flags!=8)
				StoreSh4Reg(r0,op->rd);
			else
				StoreSh4Reg64(r0,op->rd);
			break;
		}


		case shop_writem:
		{
		    LoadSh4Reg(r0, op->rs1);
			//possibly use ldm/ldrd ?
			if (op->flags!=8)
				LoadSh4Reg(r1, op->rs2);
			else
				LoadSh4Reg64(r2, op->rs2);

			if (op->rs3.is_imm())
			{
				if(op->rs3.is_imm_u8())    // *FIXME* use Check8bRot instead ! //u8 ?
			    {
			        ADD(r0,r0,op->rs3._imm);
                } 
				else 
				{
					MOV32(r4,op->rs3._imm);
                    ADD(r0,r0,r4);
                }
			}
			else if (op->rs3.is_r32i())
			{
			    LoadSh4Reg(r4,op->rs3);
			    ADD(r0,r0,r4);
			}
			else if (!op->rs3.is_null())
			{
				printf("rs3: %08X\n",op->rs3.type);
				die("invalid rs3");
			}

			switch(op->flags) 
			{
				case 1: CALL((u32)WriteMem8);   break;
				case 2: CALL((u32)WriteMem16);  break;
				case 4: CALL((u32)WriteMem32);  break;
				case 8: CALL((u32)WriteMem64);  break;
				default: die("invalid size!");
			}
			break;
		}

        //dynamic jump, r+imm32.This will be at the end of the block, but doesn't -have- to be the last opcode
		case shop_jdyn:
		{
		    LoadSh4Reg(r1, op->rs1);

			if (op->rs2.is_imm())
		    {
				MOV32(r2, (u32)op->rs2._imm);
		        ADD(r1,r1,r2);
            }
            StoreImmBase(r1,r2,(u32)&djump_reg);
            break;
		}

		case shop_mov32:
		{
			if (op->rs1.is_imm())
		    {
				StoreSh4RegImmVal(r3,op->rd._reg, op->rs1._imm);   // rs1 is a const here
            }
			else if (op->rs1.is_r32())
            {
		        LoadSh4Reg(r0,op->rs1);
		        StoreSh4Reg(r0,op->rd);
            }
            else {
                goto __default;
            }
            break;
        }
		case shop_mov64:
		{
			verify(op->rs1.is_r64() && op->rd.is_r64());
			LoadSh4Reg64(r0,op->rs1);
		    StoreSh4Reg64(r0,op->rd);
            break;
        }

		case shop_jcond:
		{
			compile_state.has_jcond=true;

	        LoadSh4Reg(r0,op->rs1);
	        StoreImmBase(r0,r2,(u32)&djump_cond);
			break;
		}

		case shop_ifb:
        {
			if (op->rs1._imm) {
                StoreImms(r3,r2,(u32)&next_pc,(u32)op->rs2._imm);
            }

            MOV32(r0, (u32)op->rs3._imm);
            CALL((u32)(OpPtr[op->rs3._imm]));
            break;
        }

#ifndef CANONICALTEST
		case shop_and:  ngen_Binary(op,AND);    break;
		case shop_or:   ngen_Binary(op,ORR);    break;
		case shop_xor:	ngen_Binary(op,EOR);    break;
		case shop_add:	ngen_Binary(op,ADD);    break;
		case shop_sub:	ngen_Binary(op,SUB);    break;

		case shop_neg:	ngen_Unary(op,NEG);     break;
		case shop_not:	ngen_Unary(op,NOT);     break;




		case shop_sync_sr:
        {
            CALL((u32)UpdateSR);
            break;
		}

		case shop_test:
		{
            LoadSh4Reg(r0,op->rs1);

			if (op->rs2.is_imm())
            {
            /*  if(0==(op->rs2 & ~0xFFF)) {
                    TST(r0,(u32)op->rs2);
                }
                else if(0==(op->rs2 & ~0xFFFF)) {
                    MOVW(r1,(u32)op->rs2);
                    TST(r0,r1);
                }
                else {*/
					MOV32(r1,(u32)op->rs2._imm);
                    TST(r0,r1);
                //}
            }
			else if (op->rs2.is_r32i())
            {
                LoadSh4Reg(r1,op->rs2);
                TST(r0,r1);
            }
            else
            {
                printf("ngen_Bin ??? %d \n",op->rs2.type);
                verify(false);
            }


		    MOVW(r0,0);
		    MOVW(r0,1,CC_EQ);
		    StoreSh4Reg(r0,reg_sr_T);
		    break;
        }

			//fpu
		case shop_fadd:
		case shop_fsub:
		case shop_fmul:
#if 0
		case shop_fdiv:	//VDIV_VFP is broken for now ..
#endif
		{
			const FPBinOP* opcds[]= { VADD_NFP,VSUB_NFP,VMUL_NFP,VDIV_VFP };
			ngen_fp_bin(op, opcds[op->op-shop_fadd]);
		}
		break;

#endif
		default:
			shil_chf[op->op](op);
			break;

__default:
            printf("@@\tError, Default case (0x%X) in ngen_CompileBlock!\n", op->op);
            verify(false);
            break;
		}
		
	}

    ngen_End(block);

    // Clear the area we've written to for cache
    u8* pEnd = (u8*)emit_GetCCPtr();
    CacheFlush((void*)code, pEnd);
	
	//void emit_WriteCodeCache();
	//emit_WriteCodeCache();

    return code;
}



DynarecCodeEntry* ngen_Compile(DecodedBlock* block,bool force_checks)
{
    return (DynarecCodeEntry*)ngen_CompileBlock(block);
}



void ngen_ResetBlocks()
{
    printf("@@\tngen_ResetBlocks()\n");
    Labels.clear();
}


extern "C" void ngen_FailedToFindBlock_();
void (*ngen_FailedToFindBlock)()=&ngen_FailedToFindBlock_;  // in asm

void ngen_GetFeatures(ngen_features* dst)
{
	dst->InterpreterFallback=false;
	dst->OnlyDynamicEnds=false;
}