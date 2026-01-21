#include "types.h"
#include "dc\sh4\sh4_opcode_list.h"
#include "dc\sh4\ccn.h"

#include "dc\sh4\sh4_registers.h"
#include "dc\sh4\rec_v2\ngen.h"
#include "dc\mem\sh4_mem.h"

#include "emitter\emitter\x86_emitter.h"

x86_block* x86e;

u32 djump_reg;
u32 djump_cond;
void* loop_no_update;
void* loop_do_update;
void* loop_do_update_write;

bool sse_1=true;
bool sse_2=true;
bool sse_3=true;
bool ssse_3=true;
bool mmx=true;

void DetectCpuFeatures()
{
	static bool detected=false;
	if (detected) return;
	detected=true;

	__try
	{
		__asm addps xmm0,xmm0
	}
	__except(1) 
	{
		sse_1=false;
	}

	__try
	{
		__asm addpd xmm0,xmm0
	}
	__except(1) 
	{
		sse_2=false;
	}

	__try
	{
		__asm addsubpd xmm0,xmm0
	}
	__except(1) 
	{
		sse_3=false;
	}

	__try
	{
		__asm phaddw xmm0,xmm0
	}
	__except(1) 
	{
		ssse_3=false;
	}

	
	__try
	{
		__asm paddd mm0,mm1
		__asm emms;
	}
	__except(1) 
	{
		mmx=false;
	}
}
struct
{
	bool has_jcond;

	void Reset()
	{
		has_jcond=false;
	}
} compile_state;

struct CompileListItem
{
	u32 addr;
	u8* patchptr;
	bool optional;
};
vector<CompileListItem> ngen_CompileList;
void ngen_CompileListAdd(u32 addr,u8* patchptr,bool optional)
{
	CompileListItem t = {addr,patchptr,optional};
	ngen_CompileList.push_back(t);
}
naked void FASTCALL ngen_blockcheckfail(u32 addr)
{
	__asm
	{
		call rdv_BlockCheckFail;
		jmp eax;
	}
}
void ngen_Begin(DecodedBlock* block,bool force_checks)
{
	compile_state.Reset();

	x86e->Emit(op_mov32,ECX,block->start);
	if (force_checks)
	{
		u32* ptr=(u32*)GetMemPtr(block->start,4);
		x86e->Emit(op_cmp32,ptr,*ptr);
		x86e->Emit(op_jne,x86_ptr_imm(ngen_blockcheckfail));
	}
	x86e->Emit(op_test32,ESI,ESI);
	x86e->Emit(op_js,x86_ptr_imm(loop_do_update_write));
	x86e->Emit(op_sub32,ESI,block->cycles);
}

void* FASTCALL ngen_LinkBlock_Static(u32 pc,u8* patch)
{
	next_pc=pc;
	bool do_link=true;

	if (0==rdv_FindCode() && patch==((u8*)emit_GetCCPtr()-5))
	{
		emit_Skip(-10);
		do_link=false;
	}
	DynarecCodeEntry* rv=rdv_FindOrCompile();

	if (do_link)
	{
		x86_block* x86e = new x86_block();
		x86e->Init(0,0);
		x86e->x86_buff=patch;
		x86e->x86_size=512;
		x86e->do_realloc=false;

		x86e->Emit(op_jmp,x86_ptr_imm(rv));

		x86e->Generate();
		delete x86e;
	}
	return rv;
}
naked void ngen_LinkBlock_Static_stub()
{
	__asm
	{
		pop edx;
		sub edx,5;
		call ngen_LinkBlock_Static;
		jmp eax;
	}
}
naked void ngen_LinkBlock_condX_stub()
{
	__asm
	{
		pop edx;
		sub edx,5;
		call ngen_LinkBlock_Static;
		jmp eax;
	}
}


void ngen_End(DecodedBlock* block)
{
	switch(block->BlockType)
	{
	case BET_Cond_0:
	case BET_Cond_1:
		{
			//printf("COND %d\n",block->BlockType&1);
			x86e->Emit(op_cmp32,compile_state.has_jcond? &djump_cond : &sr.T,block->BlockType&1);
			
			x86_Label* noBranch=x86e->CreateLabel(0,8);

			x86e->Emit(op_jne,noBranch);
			{
				//do branch
				x86e->Emit(op_mov32,ECX,block->BranchBlock);
				x86e->Emit(op_call,x86_ptr_imm(ngen_LinkBlock_condX_stub));
			}
			x86e->MarkLabel(noBranch);
			{
				//no branch
				x86e->Emit(op_mov32,ECX,block->NextBlock);
				x86e->Emit(op_call,x86_ptr_imm(ngen_LinkBlock_condX_stub));
			}

			ngen_CompileListAdd(block->NextBlock,0,false);
			ngen_CompileListAdd(block->BranchBlock,0,false);
		}
		break;

	case BET_DynamicCall:
	case BET_DynamicJump:
	case BET_DynamicRet:
		//printf("Dynamic !\n");
		x86e->Emit(op_mov32,ECX,&djump_reg);
		x86e->Emit(op_jmp,x86_ptr_imm(loop_no_update));
		break;

	case BET_StaticIntr:
	case BET_DynamicIntr:
		//printf("Interrupt !\n");
		if (block->BlockType==BET_StaticIntr)
		{
			x86e->Emit(op_mov32,&next_pc,block->BranchBlock);
		}
		else
		{
			x86e->Emit(op_mov32,EAX,&djump_reg);
			x86e->Emit(op_mov32,&next_pc,EAX);
		}
		x86e->Emit(op_call,x86_ptr_imm(UpdateINTC));
		x86e->Emit(op_mov32,ECX,&next_pc);
		x86e->Emit(op_jmp,x86_ptr_imm(loop_no_update));
		break;

	case BET_StaticCall:
	case BET_StaticJump:
		//printf("Static 0x%08X!\n",block->BranchBlock);
		x86e->Emit(op_mov32,ECX,block->BranchBlock);
		x86e->Emit(op_call,x86_ptr_imm(ngen_LinkBlock_Static_stub));
		ngen_CompileListAdd(block->BranchBlock,0,false);
		break;
	}
}
void __fastcall PrintBlock(u32 pc)
{
	printf("block: 0x%08X\n",pc);
	for (int i=0;i<16;i++)
		printf("%08X ",r[i]);
	printf("\n");
}
u32* GetRegPtr(u32 reg)
{
	return Sh4_int_GetRegisterPtr((Sh4RegType)reg);
}

void ngen_Bin(shil_opcode* op,x86_opcode_class natop,bool has_imm=true,bool has_wb=true)
{
	x86e->Emit(op_mov32,EAX,op->rs1.reg_ptr());

	if (has_imm && op->rs2.is_imm())
	{
		x86e->Emit(natop,EAX,op->rs2._imm);
	}
	else if (op->rs2.is_r32i())
	{
		x86e->Emit(natop,EAX,op->rs2.reg_ptr());
	}
	else
	{
		printf("%d \n",op->rs1.type);
		verify(false);
	}

	if (has_wb)
		x86e->Emit(op_mov32,op->rd.reg_ptr(),EAX);
}
void ngen_fp_bin(shil_opcode* op,x86_opcode_class natop,bool has_wb=true)
{
	x86e->Emit(op_movss,XMM0,op->rs1.reg_ptr());

	if (op->rs2.is_r32f())
	{
		x86e->Emit(natop,XMM0,op->rs2.reg_ptr());
	}
	else
	{
		printf("%d \n",op->rs2.type);
		verify(false);
	}
	if (has_wb)
		x86e->Emit(op_movss,op->rd.reg_ptr(),XMM0);
}
void ngen_Unary(shil_opcode* op,x86_opcode_class natop)
{
	x86e->Emit(op_mov32,EAX,op->rs1.reg_ptr());

	x86e->Emit(natop,EAX);
	
	x86e->Emit(op_mov32,op->rd.reg_ptr(),EAX);
}

void* _vmem_read_const(u32 addr,bool& ismem,u32 sz);
void FASTCALL do_sqw_mmu(u32 dst);
void FASTCALL do_sqw_nommu(u32 dst);

u32 ngen_CC_BytesPushed;
void ngen_CC_Start(shil_opcode* op)
{
	ngen_CC_BytesPushed=0;
}
void ngen_CC_Param(shil_opcode* op,shil_param* par,CanonicalParamType tp)
{
	switch(tp)
	{
		//push the contents
		case CPT_u32:
		case CPT_f32:
			if (par->is_reg())
				x86e->Emit(op_push32,x86_ptr(par->reg_ptr()));
			else if (par->is_imm())
				x86e->Emit(op_push,par->_imm);
			else
				die("invalid combination");
			ngen_CC_BytesPushed+=4;
			break;
		//push the ptr itself
		case CPT_ptr:
			x86e->Emit(op_push,(unat)par->reg_ptr());
			ngen_CC_BytesPushed+=4;
			break;

		//store from EAX
		case CPT_u64rvL:
		case CPT_u32rv:
			x86e->Emit(op_mov32,par->reg_ptr(),EAX);
			break;

		case CPT_u64rvH:
			x86e->Emit(op_mov32,par->reg_ptr(),EDX);
			break;

		//Store from ST(0)
		case CPT_f32rv:
			x86e->Emit(op_fstp32f,x86_ptr(par->reg_ptr()));
			break;
		
	}
}
void ngen_CC_Call(shil_opcode*op,void* function)
{
	x86e->Emit(op_call,x86_ptr_imm(function));
}
void ngen_CC_Finish(shil_opcode* op)
{
	x86e->Emit(op_add32,ESP,ngen_CC_BytesPushed);
}

DynarecCodeEntry* ngen_CompileBlock(DecodedBlock* block,bool force_checks)
{
	if (emit_FreeSpace()<16*1024)
		return 0;
	
	x86e = new x86_block();
	x86e->Init(0,0);
	x86e->x86_buff=(u8*)emit_GetCCPtr();
	x86e->x86_size=emit_FreeSpace();
	x86e->do_realloc=false;

	//x86e->Emit(op_mov32,ECX,block->start);
	//x86e->Emit(op_call,x86_ptr_imm(&PrintBlock));

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
							x86e->Emit(op_movsx8to32,EAX,ptr);
						else if (op->flags==2)
							x86e->Emit(op_movsx16to32,EAX,ptr);
						else if (op->flags==4)
							x86e->Emit(op_mov32,EAX,ptr);
						else
						{
							die("Invalid mem read size");
						}
					}
					else
					{
						x86e->Emit(op_mov32,ECX,op->rs1._imm);
						fuct=ptr;
					}
				}
				else
				{
					x86e->Emit(op_mov32,ECX,op->rs1.reg_ptr());
					if (op->rs3.is_imm())
					{
						x86e->Emit(op_add32,ECX,op->rs3._imm);
					}
					else if (op->rs3.is_r32i())
					{
						x86e->Emit(op_add32,ECX,op->rs3.reg_ptr());
					}
					else if (!op->rs3.is_null())
					{
						die("invalid rs3");
					}

					if (op->flags!=8)
					{
						/*
						this is not working for now
						*/
						x86_Label* DoCall=x86e->CreateLabel(false,8);
						x86_Label* OpEnd=x86e->CreateLabel(false,8);

						void** vmap,** funct;
						_vmem_get_ptrs(op->flags,false,&vmap,&funct);
						x86e->Emit(op_mov32,EAX,ECX);
						x86e->Emit(op_shr32,EAX,24);
						x86e->Emit(op_mov32,EAX,x86_mrm(EAX,sib_scale_4,vmap));

						x86e->Emit(op_test32,EAX,~0x7F);
						x86e->Emit(op_jz,DoCall);
						x86e->Emit(op_xchg32,ECX,EAX);
						x86e->Emit(op_shl32,EAX,ECX);
						x86e->Emit(op_shr32,EAX,ECX);
						x86e->Emit(op_and32,ECX,~0x7F);

						x86_opcode_class opcls;
						if (op->flags==1)
							opcls=op_movsx8to32;
						else if (op->flags==2)
							opcls=op_movsx16to32;
						else if (op->flags==4)
							opcls=op_mov32;

						x86e->Emit(opcls,EAX,x86_mrm(EAX,ECX));
						x86e->Emit(op_jmp,OpEnd);
						x86e->MarkLabel(DoCall);
						x86e->Emit(op_call32,x86_mrm(EAX,funct));
						if (op->flags<4)
							x86e->Emit(opcls,EAX,EAX);
						x86e->MarkLabel(OpEnd);

						isram=true;

					}
				}

				if (!isram)
				{
					switch(op->flags)
					{
					case 1:
						if (!fuct) fuct=ReadMem8;
						x86e->Emit(op_call,x86_ptr_imm(fuct));
						x86e->Emit(op_movsx8to32,EAX,EAX);
						break;
					case 2:
						if (!fuct) fuct=ReadMem16;
						x86e->Emit(op_call,x86_ptr_imm(fuct));
						x86e->Emit(op_movsx16to32,EAX,EAX);
						break;
					case 4:
						if (!fuct) fuct=ReadMem32;
						x86e->Emit(op_call,x86_ptr_imm(fuct));
						break;
					case 8:
						if (!fuct) fuct=ReadMem64;
						x86e->Emit(op_call,x86_ptr_imm(fuct));
						break;
					default:
						verify(false);
					}
				}
				if (op->flags!=8)
				{
					x86e->Emit(op_mov32,op->rd.reg_ptr(),EAX);
				}
				else
				{
					x86e->Emit(op_mov32,op->rd.reg_ptr(),EAX);
					x86e->Emit(op_mov32,op->rd.reg_ptr()+1,EDX);
				}
			}
			break;

		case shop_writem:
			{
				x86e->Emit(op_mov32,ECX,op->rs1.reg_ptr());
				if (op->flags!=8)
				{
					x86e->Emit(op_mov32,EDX,op->rs2.reg_ptr());
				}
				else
				{
					x86e->Emit(op_push32,x86_ptr(op->rs2.reg_ptr()+1));
					x86e->Emit(op_push32,x86_ptr(op->rs2.reg_ptr()));
				}

				if (op->rs3.is_imm())
				{
					x86e->Emit(op_add32,ECX,op->rs3._imm);
				}
				else if (op->rs3.is_r32i())
				{
					x86e->Emit(op_add32,ECX,op->rs3.reg_ptr());
				}
				else if (!op->rs3.is_null())
				{
					printf("rs3: %08X\n",op->rs3.type);
					die("invalid rs3");
				}

				switch(op->flags)
				{
					case 1:
						x86e->Emit(op_call,x86_ptr_imm(&WriteMem8));
						break;
					case 2:
						x86e->Emit(op_call,x86_ptr_imm(&WriteMem16));
						break;
					case 4:
						x86e->Emit(op_call,x86_ptr_imm(&WriteMem32));
						break;
					case 8:
						x86e->Emit(op_call,x86_ptr_imm(&WriteMem64));
						break;
					default:
						verify(false);
				}
			}
			break;

		case shop_ifb:
			{
				if (op->rs1._imm)
				{
					x86e->Emit(op_mov32,&next_pc,op->rs2._imm);
				}
				x86e->Emit(op_mov32,ECX,op->rs3._imm);
				x86e->Emit(op_add32,&OpDesc[op->rs3._imm]->fallbacks,1);
				x86e->Emit(op_adc32,((u8*)&OpDesc[op->rs3._imm]->fallbacks)+4,0);
				x86e->Emit(op_call,x86_ptr_imm(OpDesc[op->rs3._imm]->oph));
			}
			break;

		case shop_jdyn:
			{
				x86e->Emit(op_mov32,EAX,op->rs1.reg_ptr());
				if (op->rs2.is_imm())
				{
					x86e->Emit(op_add32,EAX,op->rs2._imm);
				}
				x86e->Emit(op_mov32,&djump_reg,EAX);
			}
			break;

		case shop_jcond:
			{
				compile_state.has_jcond=true;
				x86e->Emit(op_mov32,EAX,op->rs1.reg_ptr());
				x86e->Emit(op_mov32,&djump_cond,EAX);
			}
			break;
			
		case shop_mov64:
			{
				verify(op->rd.is_r64());
				verify(op->rs1.is_r64());
				x86e->Emit(op_movlps,XMM0,op->rs1.reg_ptr());
				x86e->Emit(op_movlps,op->rd.reg_ptr(),XMM0);
			}
			break;
		case shop_mov32:
			{
				verify(op->rd.is_r32());

				if (op->rs1.is_imm())
				{
					x86e->Emit(op_mov32,op->rd.reg_ptr(),op->rs1._imm);
				}
				else if (op->rs1.is_r32())
				{
					x86e->Emit(op_mov32,EAX,op->rs1.reg_ptr());
					x86e->Emit(op_mov32,op->rd.reg_ptr(),EAX);
				}
				else
				{
					die("Invalid mov32 size");
				}
				
			}
			break;

//if CANONICAL_TEST is defined all opcodes use the C-based  canonical implementation !
//#define CANONICAL_TEST
#ifndef CANONICAL_TEST
		case shop_shl:
		case shop_shr:
		case shop_sar:
			{
				x86_opcode_class opcd[]={op_shl32,op_shr32,op_sar32};
				ngen_Bin(op,opcd[op->op-shop_shl]);
			}
			break;

			//rd=rs1<<rs2
		case shop_shad:
		case shop_shld:
			{
				x86_opcode_class sl32=op->op==shop_shad?op_sal32:op_shl32;
				x86_opcode_class sr32=op->op==shop_shad?op_sar32:op_shr32;

				x86e->Emit(op_mov32,EAX,op->rs1.reg_ptr());
				x86e->Emit(op_mov32,ECX,op->rs2.reg_ptr());
				
				x86_Label* _exit=x86e->CreateLabel(false,8);
				x86_Label* _neg=x86e->CreateLabel(false,8);
				x86_Label* _nz=x86e->CreateLabel(false,8);

				x86e->Emit(op_cmp32,ECX,0);
				x86e->Emit(op_js,_neg);
				{
					//>=0
					//r[n]<<=sf;
					x86e->Emit(sl32,EAX,ECX);
					x86e->Emit(op_jmp,_exit);
				}
				x86e->MarkLabel(_neg);
				x86e->Emit(op_test32,ECX,0x1f);
				x86e->Emit(op_jnz,_nz);
				{
					//1fh==0
					if (op->op!=shop_shad)
					{
						//r[n]=0;
						x86e->Emit(op_mov32,EAX,0);
					}
					else
					{
						//r[n]>>=31;
						x86e->Emit(op_sar32,EAX,31);
					}
					x86e->Emit(op_jmp,_exit);
				}
				x86e->MarkLabel(_nz);
				{
					//<0
					//r[n]>>=(-sf);
					x86e->Emit(op_neg32,ECX);
					x86e->Emit(sr32,EAX,ECX);
				}
				x86e->MarkLabel(_exit);

				x86e->Emit(op_mov32,op->rd.reg_ptr(),EAX);
			}
			break;

		case shop_and:
				ngen_Bin(op,op_and32);
			break;

		case shop_or:
				ngen_Bin(op,op_or32);
			break;

		case shop_xor:
				ngen_Bin(op,op_xor32);
			break;

		case shop_add:
				ngen_Bin(op,op_add32);
			break;

		case shop_sub:
				ngen_Bin(op,op_sub32);
			break;

		case shop_neg:
				ngen_Unary(op,op_neg32);
			break;

		case shop_not:
				ngen_Unary(op,op_not32);
			break;

		case shop_ror:
				ngen_Bin(op,op_ror32);
			break;
			
		case shop_test:
		case shop_seteq:
		case shop_setge:
		case shop_setgt:
		case shop_setae:
		case shop_setab:
			{
				x86_opcode_class opcls1=op->op==shop_test?op_test32:op_cmp32;
				x86_opcode_class opcls2[]={op_setz,op_sete,op_setge,op_setg,op_setae,op_seta };
				ngen_Bin(op,opcls1,true,false);
				x86e->Emit(opcls2[op->op-shop_test],AL);
				x86e->Emit(op_movzx8to32,EAX,AL);
				x86e->Emit(op_mov32,op->rd.reg_ptr(),EAX);
			}
			break;

		case shop_sync_sr:
			{
				x86e->Emit(op_call,x86_ptr_imm(UpdateSR));
			}
			break;

		case shop_mul_u16:
		case shop_mul_s16:
		case shop_mul_i32:
		case shop_mul_u64:
		case shop_mul_s64:
			{
				x86_opcode_class opdt[]={op_movzx16to32,op_movsx16to32,op_mov32,op_mov32,op_mov32};
				x86_opcode_class opmt[]={op_mul32,op_mul32,op_mul32,op_mul32,op_imul32};
				//only the top 32 bits are different on signed vs unsigned

				u32 opofs=op->op-shop_mul_u16;

				x86e->Emit(opdt[opofs],EAX,op->rs1.reg_ptr());
				x86e->Emit(opdt[opofs],EDX,op->rs2.reg_ptr());
				
				x86e->Emit(opmt[opofs],EDX);
				x86e->Emit(op_mov32,op->rd.reg_ptr(),EAX);

				if (op->op>=shop_mul_u64)
					x86e->Emit(op_mov32,op->rd2.reg_ptr(),EDX);
			}
			break;


			//fpu
		case shop_fadd:
		case shop_fsub:
		case shop_fmul:
		case shop_fdiv:
			{
				const x86_opcode_class opcds[]= { op_addss, op_subss, op_mulss, op_divss };
				ngen_fp_bin(op,opcds[op->op-shop_fadd]);
			}
			break;

		case shop_fabs:
			{
				verify(op->rd._reg==op->rs1._reg);
				x86e->Emit(op_and32,op->rd.reg_ptr(),0x7fffffff);
			}
			break;

		case shop_fneg:
			{
				verify(op->rd._reg==op->rs1._reg);
				x86e->Emit(op_xor32,op->rd.reg_ptr(),0x80000000);
			}
			break;

		case shop_fsca:
			{
				#define pi (3.14159265f)
				static float fsca_fpul_adj=(2*pi)/65536.0f;
				verify(op->rs1.is_r32i());
				//verify(op->rd.is_); //double ? vector(2) ?

				//sin/cos
				x86e->Emit(op_fild32i,x86_ptr(op->rs1.reg_ptr()));		//st(0)=(s32)fpul
				x86e->Emit(op_fmul32f,x86_ptr(&fsca_fpul_adj));			//st(0)=(s32)fpul * ((2*pi)/65536.0f)
				x86e->Emit(op_fsincos);						//st(0)=sin , st(1)=cos
		
				u32* rd=op->rd.reg_ptr();
				x86e->Emit(op_fstp32f,x86_ptr(rd+1));	//Store cos to reg[1]
				x86e->Emit(op_fstp32f,x86_ptr(rd));		//store sin to reg[0]
			}
			break;

		case shop_fipr:
			{
				//rd=rs1*rs2 (vectors)
				verify(op->rs1.is_r32fv()==4);
				verify(op->rs2.is_r32fv()==4);
				verify(op->rd.is_r32());

				if (sse_3)
				{
					x86e->Emit(op_movaps ,XMM0,op->rs1.reg_ptr());
					x86e->Emit(op_mulps ,XMM0,op->rs2.reg_ptr());
														//xmm0={a0				,a1				,a2				,a3}
					x86e->Emit(op_haddps,XMM0,XMM0);	//xmm0={a0+a1			,a2+a3			,a0+a1			,a2+a3}
					x86e->Emit(op_haddps,XMM0,XMM0);	//xmm0={(a0+a1)+(a2+a3) ,(a0+a1)+(a2+a3),(a0+a1)+(a2+a3),(a0+a1)+(a2+a3)}

					x86e->Emit(op_movss,op->rd.reg_ptr(),XMM0);
				}
				else
				{
					x86e->Emit(op_movaps ,XMM0,op->rs1.reg_ptr());
					x86e->Emit(op_mulps ,XMM0,op->rs2.reg_ptr());
					x86e->Emit(op_movhlps ,XMM1,XMM0);
					x86e->Emit(op_addps ,XMM0,XMM1);
					x86e->Emit(op_movaps ,XMM1,XMM0);
					x86e->Emit(op_shufps ,XMM1,XMM1,1);
					x86e->Emit(op_addss ,XMM0,XMM1);
					x86e->Emit(op_movss,op->rd.reg_ptr(),XMM0);
				}
			}
			break;
		case shop_fsqrt:
			{
				//rd=sqrt(rs1)
				x86e->Emit(op_sqrtss ,XMM0,op->rs1.reg_ptr());
				x86e->Emit(op_movss ,op->rd.reg_ptr(),XMM0);
			}
			break;

		case shop_ftrv:
			{
				//rd(vector)=rs1(vector)*rs2(matrix)
				verify(op->rd.is_r32fv()==4);
				verify(op->rs1.is_r32fv()==4);
				verify(op->rs2.is_r32fv()==16);

				//load the vector ..
				if (sse_2)
				{
					x86e->Emit(op_movaps ,XMM3,op->rs1.reg_ptr());	//xmm0=vector
					x86e->Emit(op_pshufd ,XMM0,XMM3,0);					//xmm0={v0}
					x86e->Emit(op_pshufd ,XMM1,XMM3,0x55);				//xmm1={v1}	
					x86e->Emit(op_pshufd ,XMM2,XMM3,0xaa);				//xmm2={v2}
					x86e->Emit(op_pshufd ,XMM3,XMM3,0xff);				//xmm3={v3}
				}
				else
				{
					x86e->Emit(op_movaps ,XMM0,op->rs1.reg_ptr());	//xmm0=vector

					x86e->Emit(op_movaps ,XMM3,XMM0);					//xmm3=vector
					x86e->Emit(op_shufps ,XMM0,XMM0,0);					//xmm0={v0}
					x86e->Emit(op_movaps ,XMM1,XMM3);					//xmm1=vector
					x86e->Emit(op_movaps ,XMM2,XMM3);					//xmm2=vector
					x86e->Emit(op_shufps ,XMM3,XMM3,0xff);				//xmm3={v3}
					x86e->Emit(op_shufps ,XMM1,XMM1,0x55);				//xmm1={v1}	
					x86e->Emit(op_shufps ,XMM2,XMM2,0xaa);				//xmm2={v2}
				}

				//do the matrix mult !
				x86e->Emit(op_mulps ,XMM0,op->rs2.reg_ptr() + 0);		//v0*=vm0
				x86e->Emit(op_mulps ,XMM1,op->rs2.reg_ptr() + 4);		//v1*=vm1
				x86e->Emit(op_mulps ,XMM2,op->rs2.reg_ptr() + 8);		//v2*=vm2
				x86e->Emit(op_mulps ,XMM3,op->rs2.reg_ptr() + 12);		//v3*=vm3

				x86e->Emit(op_addps ,XMM0,XMM1);					//sum it all up
				x86e->Emit(op_addps ,XMM2,XMM3);
				x86e->Emit(op_addps ,XMM0,XMM2);

				x86e->Emit(op_movaps ,op->rd.reg_ptr(),XMM0);
			}
			break;

		case shop_fmac:
			{
				//rd=rs1+rs2*rs3
				x86e->Emit(op_movss ,XMM0,op->rs2.reg_ptr());
				x86e->Emit(op_mulss ,XMM0,op->rs3.reg_ptr());
				x86e->Emit(op_addss ,XMM0,op->rs1.reg_ptr());
				x86e->Emit(op_movss ,op->rd.reg_ptr(),XMM0);
			}
			break;

		case shop_fsrra:
			{
				//rd=1/sqrt(rs1)
				static float one=1.0f;
				x86e->Emit(op_sqrtss ,XMM1,op->rs1.reg_ptr());
				x86e->Emit(op_movss ,XMM0,&one);
				x86e->Emit(op_divss ,XMM0,XMM1);
				x86e->Emit(op_movss ,op->rd.reg_ptr(),XMM0);
			}
			break;

		case shop_fseteq:
		case shop_fsetgt:
			{
				x86e->Emit(op_movss,XMM0,op->rs1.reg_ptr());
				x86e->Emit(op_ucomiss,XMM0,op->rs2.reg_ptr());

				if (op->op==shop_fseteq)
				{
					//special case
					//We want to take in account the 'unordered' case on the fpu
					x86e->Emit(op_lahf);
					x86e->Emit(op_test8,AH,0x44);
					x86e->Emit(op_setnp,AL);
				}
				else
				{
					x86e->Emit(op_seta,AL);
				}

				x86e->Emit(op_movzx8to32,EAX,AL);
				x86e->Emit(op_mov32,op->rd.reg_ptr(),EAX);
			}
			break;

		case shop_pref:
			{
				verify(op->rs1.is_r32i());
				x86e->Emit(op_mov32 ,ECX,op->rs1.reg_ptr());
				x86e->Emit(op_mov32 ,EDX,ECX);
				x86e->Emit(op_shr32 ,EDX,26);

				x86_Label* nosq=x86e->CreateLabel(false,8);

				x86e->Emit(op_cmp32,EDX,0x38);
				x86e->Emit(op_jne,nosq);
				{
					void (fastcall * handl)(u32);
					
					if (CCN_MMUCR.AT)
						handl=&do_sqw_mmu;
					else
						handl=&do_sqw_nommu;

					x86e->Emit(op_call,x86_ptr_imm(handl));
				}
				x86e->MarkLabel(nosq);
			}
			break;

		case shop_ext_s8:
		case shop_ext_s16:
			{
				verify(op->rd.is_r32i());
				verify(op->rs1.is_r32i());
				
				if (op->op==shop_ext_s8)
					x86e->Emit(op_movsx8to32,EAX,op->rs1.reg_ptr());
				else
					x86e->Emit(op_movsx16to32,EAX,op->rs1.reg_ptr());
				
				x86e->Emit(op_mov32,op->rd.reg_ptr(),EAX);
			}
			break;

		case shop_cvt_f2i_t:
			verify(op->rd.is_r32i());
			verify(op->rs1.is_r32f());

			x86e->Emit(op_cvttss2si,EAX,op->rs1.reg_ptr());
			x86e->Emit(op_mov32,op->rd.reg_ptr(),EAX);
			break;

			//i hope that the round mode bit is set properly here :p
		case shop_cvt_i2f_n:
		case shop_cvt_i2f_z:
			verify(op->rd.is_r32f());
			verify(op->rs1.is_r32i());

			x86e->Emit(op_cvtsi2ss,XMM0,op->rs1.reg_ptr());
			x86e->Emit(op_movss,op->rd.reg_ptr(),XMM0);
			break;
#endif
		default:
			shil_chf[op->op](op);
			break;
defaulty:
			printf("OH CRAP %d\n",op->op);
			verify(false);
		}
	}

	ngen_End(block);
	DynarecCodeEntry* rv=(DynarecCodeEntry*)x86e->Generate();
	emit_Skip(x86e->x86_indx);
	delete x86e;
	x86e=0;

	return rv;
}


DynarecCodeEntry* ngen_Compile(DecodedBlock* block,bool force_checks)
{
	DetectCpuFeatures();
	DynarecCodeEntry* rv = ngen_CompileBlock(block,force_checks);
	ngen_CompileList.clear();
	return rv;
}
void ngen_ResetBlocks()
{
}

naked void ngen_FailedToFindBlock_()
{
	__asm
	{
		call rdv_FailedToFindBlock;
		jmp eax;
	}
}
void (*ngen_FailedToFindBlock)()=&ngen_FailedToFindBlock_;
naked void ngen_mainloop()
{
	__asm
	{
		push esi;
		push edi;
		push ebp;
		push ebx;

		mov ecx,next_pc;
		mov esi,SH4_TIMESLICE;

		mov [loop_no_update],offset no_update;
		mov [loop_do_update],offset do_update;
		mov [loop_do_update_write],offset do_update_write;
		
		//next_pc _MUST_ be on ecx
no_update:
		call bm_GetCode
		jmp eax;

do_update_write:
		mov [next_pc],ECX;
		//next_pc _MUST_ be on ram
do_update:
		add esi,SH4_TIMESLICE;
		call UpdateSystem;

		cmp byte ptr [sh4_int_bCpuRun],0;
		mov ecx,[next_pc];
		jne  no_update;

cleanup:
		pop ebx;
		pop ebp;
		pop edi;
		pop esi;

		ret;
	}
}

void ngen_GetFeatures(ngen_features* dst)
{
	dst->InterpreterFallback=false;
	dst->OnlyDynamicEnds=false;
}