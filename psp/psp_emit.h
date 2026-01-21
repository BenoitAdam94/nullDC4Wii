/*
* Copyright (C) 2008, MBCE4ALL
* MBC4ALL author : hlide (christophe avoinne, hlide@free.fr)
*
* See the file "LICENSE" for information on usage and
* redistribution of this file, and for a DISCLAIMER OF ALL
* WARRANTIES.
*
* $HeadURL: $
* $Id: $
*/

#pragma once

#include <psptypes.h>

typedef union
{
	u32 word;

	struct { u32 fun : 6, sa  : 5, rd :  5, rt : 5, rs :  5, op : 6; } rtype; // register type
	struct { u32 imm                  : 16, rt : 5, rs :  5, op : 6; } itype; // immediate type
	struct { u32 imm                                   : 26, op : 6; } jtype; // jump type
	struct { u32 fun : 6, fmt : 5, fd :  5, ft : 5, fs :  5, op : 6; } ftype; // register type

	struct { u32 vd : 7, one : 1, vs : 7, two : 1,  vt :  7, sub : 3, op : 6; } vtype3; // register type

} psp_insn_t;

typedef union
{
	u32 word;

	struct { u32 : 16, imp : 8, rev : 8; } fcr0;
	struct { u32 : 7, fs : 1, c : 1, : 5, ce : 1, cvzoui : 5, evzoui : 5, fvzoui : 5, rm : 2; } fcr31;

} psp_fcr_bitfields_t;

enum
{
	psp_round_to_nearest,
	psp_round_to_zero,
	psp_round_to_posinf,
	psp_round_to_neginf,
	psp_round_mask = 3
};

/* Registers */
typedef enum
{
	psp_gr0,  psp_r0  = psp_gr0,  psp_zr = psp_gr0, psp_zero = psp_gr0, psp_gpr = psp_gr0,
	psp_gr1,  psp_r1  = psp_gr1,  psp_at = psp_gr1,
	psp_gr2,  psp_r2  = psp_gr2,  psp_v0 = psp_gr2,
	psp_gr3,  psp_r3  = psp_gr3,  psp_v1 = psp_gr3,
	psp_gr4,  psp_r4  = psp_gr4,  psp_a0 = psp_gr4,
	psp_gr5,  psp_r5  = psp_gr5,  psp_a1 = psp_gr5,
	psp_gr6,  psp_r6  = psp_gr6,  psp_a2 = psp_gr6,
	psp_gr7,  psp_r7  = psp_gr7,  psp_a3 = psp_gr7,
	psp_gr8,  psp_r8  = psp_gr8,  psp_t0 = psp_gr8,
	psp_gr9,  psp_r9  = psp_gr9,  psp_t1 = psp_gr9,
	psp_gr10, psp_r10 = psp_gr10, psp_t2 = psp_gr10,
	psp_gr11, psp_r11 = psp_gr11, psp_t3 = psp_gr11,
	psp_gr12, psp_r12 = psp_gr12, psp_t4 = psp_gr12,
	psp_gr13, psp_r13 = psp_gr13, psp_t5 = psp_gr13,
	psp_gr14, psp_r14 = psp_gr14, psp_t6 = psp_gr14,
	psp_gr15, psp_r15 = psp_gr15, psp_t7 = psp_gr15,
	psp_gr16, psp_r16 = psp_gr16, psp_s0 = psp_gr16,
	psp_gr17, psp_r17 = psp_gr17, psp_s1 = psp_gr17,
	psp_gr18, psp_r18 = psp_gr18, psp_s2 = psp_gr18,
	psp_gr19, psp_r19 = psp_gr19, psp_s3 = psp_gr19,
	psp_gr20, psp_r20 = psp_gr20, psp_s4 = psp_gr20,
	psp_gr21, psp_r21 = psp_gr21, psp_s5 = psp_gr21,
	psp_gr22, psp_r22 = psp_gr22, psp_s6 = psp_gr22,
	psp_gr23, psp_r23 = psp_gr23, psp_s7 = psp_gr23,
	psp_gr24, psp_r24 = psp_gr24, psp_t8 = psp_gr24,
	psp_gr25, psp_r25 = psp_gr25, psp_t9 = psp_gr25,
	psp_gr26, psp_r26 = psp_gr26, psp_k0 = psp_gr26,
	psp_gr27, psp_r27 = psp_gr27, psp_k1 = psp_gr27,
	psp_gr28, psp_r28 = psp_gr28, psp_gp = psp_gr28,
	psp_gr29, psp_r29 = psp_gr29, psp_sp = psp_gr29,
	psp_gr30, psp_r30 = psp_gr30, psp_fp = psp_gr30, psp_s8 = psp_gr30,
	psp_gr31, psp_r31 = psp_gr31, psp_ra = psp_gr31,
} psp_gpr_t;

typedef enum
{
	psp_c0dr0, psp_c0dr = psp_c0dr0,
	psp_c0dr1,
	psp_c0dr2,
	psp_c0dr3,
	psp_c0dr4,
	psp_c0dr5,
	psp_c0dr6,
	psp_c0dr7,
	psp_c0dr8,
	psp_c0dr9,
	psp_c0dr10,
	psp_c0dr11,
	psp_c0dr12,
	psp_c0dr13,
	psp_c0dr14,
	psp_c0dr15,
	psp_c0dr16,
	psp_c0dr17,
	psp_c0dr18,
	psp_c0dr19,
	psp_c0dr20,
	psp_c0dr21,
	psp_c0dr22,
	psp_c0dr23,
	psp_c0dr24,
	psp_c0dr25,
	psp_c0dr26,
	psp_c0dr27,
	psp_c0dr28,
	psp_c0dr29,
	psp_c0dr30,
	psp_c0dr31,
} psp_c0dr_t;

typedef enum
{
	psp_c0cr0, psp_c0cr = psp_c0cr0,
	psp_c0cr1,
	psp_c0cr2,
	psp_c0cr3,
	psp_c0cr4,
	psp_c0cr5,
	psp_c0cr6,
	psp_c0cr7,
	psp_c0cr8,
	psp_c0cr9,
	psp_c0cr10,
	psp_c0cr11,
	psp_c0cr12,
	psp_c0cr13,
	psp_c0cr14,
	psp_c0cr15,
	psp_c0cr16,
	psp_c0cr17,
	psp_c0cr18,
	psp_c0cr19,
	psp_c0cr20,
	psp_c0cr21,
	psp_c0cr22,
	psp_c0cr23,
	psp_c0cr24,
	psp_c0cr25,
	psp_c0cr26,
	psp_c0cr27,
	psp_c0cr28,
	psp_c0cr29,
	psp_c0cr30,
	psp_c0cr31,
} psp_c0cr_t;

typedef enum
{
	psp_fr0, psp_fpr = psp_fr0,
	psp_fr1,
	psp_fr2,
	psp_fr3,
	psp_fr4,
	psp_fr5,
	psp_fr6,
	psp_fr7,
	psp_fr8,
	psp_fr9,
	psp_fr10,
	psp_fr11,
	psp_fr12,
	psp_fr13,
	psp_fr14,
	psp_fr15,
	psp_fr16,
	psp_fr17,
	psp_fr18,
	psp_fr19,
	psp_fr20,
	psp_fr21,
	psp_fr22,
	psp_fr23,
	psp_fr24,
	psp_fr25,
	psp_fr26,
	psp_fr27,
	psp_fr28,
	psp_fr29,
	psp_fr30,
	psp_fr31,
} psp_fpr_t;

typedef enum
{
	psp_c1cr0, psp_c1cr = psp_c1cr0,
	psp_c1cr1,
	psp_c1cr2,
	psp_c1cr3,
	psp_c1cr4,
	psp_c1cr5,
	psp_c1cr6,
	psp_c1cr7,
	psp_c1cr8,
	psp_c1cr9,
	psp_c1cr10,
	psp_c1cr11,
	psp_c1cr12,
	psp_c1cr13,
	psp_c1cr14,
	psp_c1cr15,
	psp_c1cr16,
	psp_c1cr17,
	psp_c1cr18,
	psp_c1cr19,
	psp_c1cr20,
	psp_c1cr21,
	psp_c1cr22,
	psp_c1cr23,
	psp_c1cr24,
	psp_c1cr25,
	psp_c1cr26,
	psp_c1cr27,
	psp_c1cr28,
	psp_c1cr29,
	psp_c1cr30,
	psp_c1cr31,
} psp_c1cr_t;

typedef enum
{
	psp_vr0, psp_vfpr = psp_vr0,
	psp_vr1,
	psp_vr2,
	psp_vr3,
	psp_vr4,
	psp_vr5,
	psp_vr6,
	psp_vr7,
	psp_vr8,
	psp_vr9,
	psp_vr10,
	psp_vr11,
	psp_vr12,
	psp_vr13,
	psp_vr14,
	psp_vr15,
	psp_vr16,
	psp_vr17,
	psp_vr18,
	psp_vr19,
	psp_vr20,
	psp_vr21,
	psp_vr22,
	psp_vr23,
	psp_vr24,
	psp_vr25,
	psp_vr26,
	psp_vr27,
	psp_vr28,
	psp_vr29,
	psp_vr30,
	psp_vr31,
	psp_vr32,
	psp_vr33,
	psp_vr34,
	psp_vr35,
	psp_vr36,
	psp_vr37,
	psp_vr38,
	psp_vr39,
	psp_vr40,
	psp_vr41,
	psp_vr42,
	psp_vr43,
	psp_vr44,
	psp_vr45,
	psp_vr46,
	psp_vr47,
	psp_vr48,
	psp_vr49,
	psp_vr50,
	psp_vr51,
	psp_vr52,
	psp_vr53,
	psp_vr54,
	psp_vr55,
	psp_vr56,
	psp_vr57,
	psp_vr58,
	psp_vr59,
	psp_vr60,
	psp_vr61,
	psp_vr62,
	psp_vr63,
	psp_vr64,
	psp_vr65,
	psp_vr66,
	psp_vr67,
	psp_vr68,
	psp_vr69,
	psp_vr70,
	psp_vr71,
	psp_vr72,
	psp_vr73,
	psp_vr74,
	psp_vr75,
	psp_vr76,
	psp_vr77,
	psp_vr78,
	psp_vr79,
	psp_vr80,
	psp_vr81,
	psp_vr82,
	psp_vr83,
	psp_vr84,
	psp_vr85,
	psp_vr86,
	psp_vr87,
	psp_vr88,
	psp_vr89,
	psp_vr90,
	psp_vr91,
	psp_vr92,
	psp_vr93,
	psp_vr94,
	psp_vr95,
	psp_vr96,
	psp_vr97,
	psp_vr98,
	psp_vr99,
	psp_vr100,
	psp_vr101,
	psp_vr102,
	psp_vr103,
	psp_vr104,
	psp_vr105,
	psp_vr106,
	psp_vr107,
	psp_vr108,
	psp_vr109,
	psp_vr110,
	psp_vr111,
	psp_vr112,
	psp_vr113,
	psp_vr114,
	psp_vr115,
	psp_vr116,
	psp_vr117,
	psp_vr118,
	psp_vr119,
	psp_vr120,
	psp_vr121,
	psp_vr122,
	psp_vr123,
	psp_vr124,
	psp_vr125,
	psp_vr126,
	psp_vr127,
	psp_vr128,
} psp_vfpr_t;

typedef enum
{
	psp_lo, psp_hilo = psp_lo,
	psp_hi,
} psp_hilo_t;

typedef enum
{
	reg_gpr = 0,
	reg_fpr = reg_gpr + 32,
	reg_vpr = reg_fpr + 32,
	reg_mdr = reg_vpr + 128
} psp_regs_t;

enum
{
	psp_fcond_f,
	psp_fcond_un,
	psp_fcond_eq,
	psp_fcond_ueq,
	psp_fcond_olt,
	psp_fcond_ult,
	psp_fcond_ole,
	psp_fcond_ule,
	psp_fcond_sf,
	psp_fcond_ngle,
	psp_fcond_seq,
	psp_fcond_ngl,
	psp_fcond_lt,
	psp_fcond_nge,
	psp_fcond_le,
	psp_fcond_ngt
};

typedef enum /* OPCODE list */
{
	//     |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
	// 000 |   *1  |   *2  |   J   |  JAL  |  BEQ  |  BNE  | BLEZ  | BGTZ  |
	// 001 | ADDI  | ADDIU | SLTI  | SLTIU | ANDI  |  ORI  | XORI  |  LUI  |
	// 010 |   *3  |   *4  | VFPU2 |  ---  | BEQL  | BNEL  | BLEZL | BGTZL |
	// 011 | VFPU0 | VFPU1 |  ---  | VFPU3 |   *5  |  ---  |  ---  |   *6  |
	// 100 |   LB  |   LH  |  LWL  |   LW  |  LBU  |  LHU  |  LWR  |  ---  |
	// 101 |   SB  |   SH  |  SWL  |   SW  |  ---  |  ---  |  SWR  | CACHE |
	// 110 |   LL  | LWC1  |  LVS  |  ---  | VFPU4 | ULVQ  |  LVQ  | VFPU5 |
	// 111 |   SC  | SWC1  |  SVS  |  ---  | VFPU6 | USVQ  |  SVQ  | VFPU7 |
	//  hi |-------|-------|-------|-------|-------|-------|-------|-------|
	//  
	//      *1 = SPECIAL, see SPECIAL list    *2 = REGIMM, see REGIMM list
	//      *3 = COP0                         *4 = COP1  
	//      *5 = SPECIAL2 , see SPECIAL2      *6 = SPECAIL3 , see SPECIAL3
	//      *ULVQ is buggy on PSP1000 PSP 
	//      *VFPU0 check VFPU0 table
	//      *VFPU1 check VFPU1 table
	//      *VFPU2 check VFPU2 table
	//      *VFPU3 check VFPU3 table
	//      *VFPU4 check VFPU4 table
	//      *VFPU5 check VFPU5 table
	//      *VFPU6 check VFPU6 table
	//      *VFPU7 check VFPU7 table
	psp_special1 = 0x00,
	psp_regimm   = 0x01,
	psp_j        = 0x02,
	psp_jal      = 0x03,
	psp_beq      = 0x04,
	psp_bne      = 0x05,
	psp_blez     = 0x06,
	psp_bgtz     = 0x07,
	psp_addi     = 0x08,
	psp_addiu    = 0x09,
	psp_slti     = 0x0a,
	psp_sltiu    = 0x0b,
	psp_andi     = 0x0c,
	psp_ori      = 0x0d,
	psp_xori     = 0x0e,
	psp_lui      = 0x0f,
	psp_cop0     = 0x10,
	psp_cop1     = 0x11,
	psp_cop2     = 0x12,
	psp_cop3     = 0x13,
	psp_beql     = 0x14,
	psp_bnel     = 0x15,
	psp_blezl    = 0x16,
	psp_bgtzl    = 0x17,
	psp_vfpu0    = 0x18,
	psp_vfpu1    = 0x19,
	psp_vfpu3    = 0x1b,
	psp_special2 = 0x1c,
	psp_special3 = 0x1f,
	psp_lb       = 0x20,
	psp_lh       = 0x21,
	psp_lwl      = 0x22,
	psp_lw       = 0x23,
	psp_lbu      = 0x24,
	psp_lhu      = 0x25,
	psp_lwr      = 0x26,
	psp_sb       = 0x28,
	psp_sh       = 0x29,
	psp_swl      = 0x2a,
	psp_sw       = 0x2b,
	psp_swr      = 0x2e,
	psp_cache    = 0x2f,
	psp_ll       = 0x30,
	psp_lwc1     = 0x31,
	psp_lvs      = 0x32,
	psp_vfpu4    = 0x34,
	psp_ulvq     = 0x35,
	psp_lvq      = 0x36,
	psp_vfpu5    = 0x37,
	psp_sc       = 0x38,
	psp_swc1     = 0x39,
	psp_svs      = 0x3a,
	psp_vfpu6    = 0x3c,
	psp_usvq     = 0x3d,
	psp_svq      = 0x3e,
	psp_vfpu7    = 0x3f,

	psp_opcode_eot
} psp_opcode_t;

typedef enum /* SPECIAL list */
{
	//     |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
	// 000 |  SLL  |  ---  |SRLROR |  SRA  | SLLV  |  ---  |SRLRORV| SRAV  |
	// 001 |   JR  | JALR  | MOVZ  | MOVN  |SYSCALL| BREAK |  ---  | SYNC  |
	// 010 | MFHI  | MTHI  | MFLO  | MTLO  |  ---  |  ---  |  CLZ  |  CLO  |
	// 011 | MULT  | MULTU |  DIV  | DIVU  | MADD  | MADDU |  ---  |  ---  |
	// 100 |  ADD  | ADDU  |  SUB  | SUBU  |  AND  |   OR  |  XOR  |  NOR  |
	// 101 |  ---  |  ---  |  SLT  | SLTU  |  MAX  |  MIN  | MSUB  | MSUBU |
	// 110 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 111 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	//  hi |-------|-------|-------|-------|-------|-------|-------|-------|
	psp_sll        = 0x00,
	psp_srl_rotr   = 0x02,
	psp_sra        = 0x03,
	psp_sllv       = 0x04,
	psp_srlv_rotrv = 0x06,
	psp_srav       = 0x07,
	psp_jr         = 0x08,
	psp_jalr       = 0x09,
	psp_movz       = 0x0a,
	psp_movn       = 0x0b,
	psp_syscall    = 0x0c,
	psp_break      = 0x0d,
	psp_sync       = 0x0f,
	psp_mfhi       = 0x10,
	psp_mthi       = 0x11,
	psp_mflo       = 0x12,
	psp_mtlo       = 0x13,
	psp_clz        = 0x16,
	psp_clo        = 0x17,
	psp_mult       = 0x18,
	psp_multu      = 0x19,
	psp_div        = 0x1a,
	psp_divu       = 0x1b,
	psp_madd       = 0x1c,
	psp_maddu      = 0x1d,
	psp_add        = 0x20,
	psp_addu       = 0x21,
	psp_sub        = 0x22,
	psp_subu       = 0x23,
	psp_and        = 0x24,
	psp_or         = 0x25,
	psp_xor        = 0x26,
	psp_nor        = 0x27,
	psp_slt        = 0x2a,
	psp_sltu       = 0x2b,
	psp_max        = 0x2c,
	psp_min        = 0x2d,
	psp_msub       = 0x2e,
	psp_msubu      = 0x2f,

	psp_opcode_special_eot
} psp_opcode_special_t;

typedef enum
{
	//     |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
	// 000 | SRL   | ROTR  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 001 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 010 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 011 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	//  hi |-------|-------|-------|-------|-------|-------|-------|-------|
	psp_srl_bit  = 0x00,
	psp_rotr_bit = 0x01,

	psp_opcode_special_srl_rotr_eot
} psp_opcode_special_srl_rotr_t;

typedef enum
{
	//     |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
	// 000 | SRLV  | ROTRV |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 001 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 010 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 011 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	//  hi |-------|-------|-------|-------|-------|-------|-------|-------|
	psp_srlv_bit  = 0x00,
	psp_rotrv_bit = 0x01,

	psp_opcode_special_srlv_rotrv_eot
} psp_opcode_special_srlv_rotrv_t;

typedef enum /* REGIMM list */
{
	//     |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
	//  00 | BLTZ  | BGEZ  | BLTZL | BGEZL |  ---  |  ---  |  ---  |  ---  |
	//  01 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	//  10 | BLTZAL| BGEZAL|BLTZALL|BGEZALL|  ---  |  ---  |  ---  |  ---  |
	//  11 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	//  hi |-------|-------|-------|-------|-------|-------|-------|-------|
	psp_bltz    = 0x00,
	psp_bgez    = 0x01,
	psp_bltzl   = 0x02,
	psp_bgezl   = 0x03,
	psp_bltzal  = 0x10,
	psp_bgezal  = 0x11,
	psp_bltzall = 0x12,
	psp_bgezall = 0x13,

	psp_opcode_regimm_eot
} psp_opcode_regimm_t;

typedef enum /* SPECIAL3 list */
{
	//     |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
	// 000 |  EXT  |  ---  |  ---  |  ---  |  INS  |  ---  |  ---  |  ---  |
	// 001 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 010 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 011 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 100 |  *1   |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 101 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 110 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 111 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	//  hi |-------|-------|-------|-------|-------|-------|-------|-------|
	//       * 1 BSHFL encoding based on sa field
	psp_ext   = 0x00,
	psp_ins   = 0x04,
	psp_bshfl = 0x20,

	psp_opcode_special3_eot
} psp_opcode_special3_t;


typedef enum
{
	//     |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
	//  00 |  ---  |  ---  | WSBH  | WSBW  |  ---  |  ---  |  ---  |  ---  |
	//  01 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	//  10 |  SEB  |  ---  |  ---  |  ---  |BITREV |  ---  |  ---  |  ---  |
	//  11 |  SEH  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	//  hi |-------|-------|-------|-------|-------|-------|-------|-------|
	psp_wsbh   = 0x02,
	psp_wsbw   = 0x03,
	psp_seb    = 0x10,
	psp_bitrev = 0x14,
	psp_seh    = 0x18,

	psp_opcode_special3_bshfl_eot
} psp_opcode_special3_bshfl_t;

typedef enum
{
	//     |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
	//  00 |  MFC1 |  ---  |  CFC1 |  ---  |  MTC1 |  ---  |  CTC1 |  ---  |
	//  01 |  *1   |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	//  10 |  *2   |  ---  |  ---  |  ---  |  *3   |  ---  |  ---  |  ---  |
	//  11 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	//  hi |-------|-------|-------|-------|-------|-------|-------|-------|
	//    *1 check COP1BC table
	//    *2 check COP1S table;
	//    *2 check COP1W table;
	psp_mfc1    = 0x00,
	psp_cfc1    = 0x02,
	psp_mtc1    = 0x04,
	psp_ctc1    = 0x06,

	psp_cop1bc	= 0x08,
	psp_cop1s	= 0x10,
	psp_cop1w	= 0x14,

	psp_opcode_cop1_eot
} psp_opcode_cop1_t;

typedef enum /* COP1BC list */
{
	//     |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
	//  00 |  BC1F | BC1T  | BC1FL | BC1TL |  ---  |  ---  |  ---  |  ---  |
	//  01 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	//  10 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	//  11 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	//  hi |-------|-------|-------|-------|-------|-------|-------|-------|
	psp_bc1f	= 0x00,
	psp_bc1t	= 0x01,
	psp_bc1fl	= 0x02,
	psp_bc1tl	= 0x03,

	psp_opcode_cop1bc_eot
} psp_opcode_cop1bc_t;

typedef enum /* COP1S list */
{
	//     |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
	// 000 | add.s | sub.s | mul.s | div.s |sqrt.s | abs.s | mov.s | neg.s |
	// 001 |  ---  |  ---  |  ---  |  ---  |            <*1>.w.s           |
	// 010 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 011 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 100 |  ---  |  ---  |  ---  |  ---  |cvt.w.s|  ---  |  ---  |  ---  |
	// 101 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 110 |                            c.<*2>.s                           |
	// 110 |                            c.<*3>.s                           |
	//  hi |-------|-------|-------|-------|-------|-------|-------|-------|   
	//
	// *1 : round.w.s | trunc.w.s | ceil.w.s | floor.w.s
	// *2 : c.f.s | c.un.s | c.eq.s | c.ueq.s | c.olt.s | c.ult.s | c.ole.s | c.ule.s
	// *3 : c.sf.s | c.ngle.s | c.seq.s | c.ngl.s | c.lt.s | c.nge.s | c.le.s  | c.ngt.s
	//
	psp_adds		= 0x00,
	psp_subs		= 0x01,
	psp_muls		= 0x02,
	psp_divs		= 0x03,
	psp_sqrts		= 0x04,
	psp_abss		= 0x05,
	psp_movs		= 0x06,
	psp_negs		= 0x07,
	psp_roundws		= 0x0c,
	psp_truncws		= 0x0d,
	psp_ceilws		= 0x0e,
	psp_floorws		= 0x0f,
	psp_cvtws		= 0x24,
	psp_cconds		= 0x30,
	//     |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
	// 000 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 001 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 010 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 011 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 100 |cvt.s.w|  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 101 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 110 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	// 110 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
	//  hi |-------|-------|-------|-------|-------|-------|-------|-------|   
	psp_cvtsw		= 0x20,

	psp_opcode_cop1s_eot
} psp_opcode_cop1s_t;

typedef enum
{
	psp_mfc2    = 0x00,
	psp_cfc2    = 0x02,
	psp_mtc2    = 0x04,
	psp_ctc2    = 0x06,

	psp_opcode_cop2_eot
} psp_opcode_cop2_t;

///////////////////////////////
void emit_insn(psp_insn_t insn);
///////////////////////////////

#define emit_rtype(_op, _rs, _rt, _rd, _sa, _func) \
	do { psp_insn_t insn; insn.rtype.op = _op; insn.rtype.rs = _rs; insn.rtype.rt = _rt; insn.rtype.rd = _rd; insn.rtype.sa = _sa; insn.rtype.fun = _func; emit_insn(insn); } while(0)

#define emit_itype(_op, _rs, _rt, _imm) \
	do { psp_insn_t insn; insn.itype.op = _op; insn.itype.rs = _rs; insn.itype.rt = _rt; insn.itype.imm = _imm; emit_insn(insn); } while(0)

#define emit_jtype(_op, _target) \
	do { psp_insn_t insn; insn.jtype.op = _op; insn.jtype.imm = _target; emit_insn(insn); } while(0)

#define emit_stype(_op, _code, _fun) \
	do { psp_insn_t insn; insn.stype.op = _op; insn.stype.imm = _code; insn.stype.fun = _func; emit_insn(insn); } while(0)

#define emit_ftype(_op, _fs, _ft, _fd, _fmt, _func) \
	do { psp_insn_t insn; insn.ftype.op = _op; insn.ftype.fs = _fs; insn.ftype.ft = _ft; insn.ftype.fd = _fd; insn.ftype.fmt = _fmt; insn.ftype.fun = _func; emit_insn(insn); } while(0)


#define emit_reg(_op, _rs, _rt, _rd, _sa, _func) \
	emit_rtype(psp_##_op, _rs, _rt, _rd, _sa, psp_##_func)

#define emit_special1(_func, _rs, _rt, _rd, _sa) \
	emit_rtype(psp_special1, _rs, _rt, _rd, _sa, psp_##_func)

#define emit_special3(_func, _rs, _rt, _imm1, _imm2) \
	emit_rtype(psp_special3, _rs, _rt, _imm1, _imm2, psp_##_func)

#define emit_special3_bshfl(_func, _rt, _rd) \
	emit_rtype(psp_special3, 0, _rt, _rd, psp_##_func, psp_bshfl)

#define emit_imm(_op, _rs, _rt, _imm) \
	emit_itype(psp_##_op, _rs, _rt, _imm)

#define emit_regimm(_func, _rs, _imm) \
	emit_itype(psp_regimm, _rs, psp_##_func, _imm)

#define emit_jump(_op, _target) \
	emit_jtype(psp_##_op, _target)

#define psp_relative_target_2(source, target) ((((u32)target - ((u32)source + 4)))>>2)
#define psp_relative_target(target) psp_relative_target_2(emit_GetCCPtr(),target)
#define psp_absolute_target(target) (((u32)target)>>2)

// psp_insn
//  = (add|addu|sub|subu) rd, rs, rt
#define emit_add(_rd, _rs, _rt) emit_special1(add, _rs, _rt, _rd, 0)
#define emit_addu(_rd, _rs, _rt) emit_special1(addu, _rs, _rt, _rd, 0)
#define emit_sub(_rd, _rs, _rt) emit_special1(sub, _rs, _rt, _rd, 0)
#define emit_subu(_rd, _rs, _rt) emit_special1(subu, _rs, _rt, _rd, 0)
#define emit_negu(_rd, _rt) emit_subu(_rd,psp_zero,_rt)
//  | (and|or|xor|nor) rd, rs, rt
#define emit_and(_rd, _rs, _rt) emit_special1(and, _rs, _rt, _rd, 0)
#define emit_or(_rd, _rs, _rt) emit_special1(or, _rs, _rt, _rd, 0)
#define emit_xor(_rd, _rs, _rt) emit_special1(xor, _rs, _rt, _rd, 0)
#define emit_nor(_rd, _rs, _rt) emit_special1(nor, _rs, _rt, _rd, 0)
#define emit_not(_rd, _rs) emit_nor(_rd,_rs,psp_zero)
//  | (slt|sltu|movn|movz|max|min) rd, rs, rt
#define emit_slt(_rd, _rs, _rt) emit_special1(slt, _rs, _rt, _rd, 0)
#define emit_sltu(_rd, _rs, _rt) emit_special1(sltu, _rs, _rt, _rd, 0)
#define emit_movn(_rd, _rs, _rt) emit_special1(movn, _rs, _rt, _rd, 0)
#define emit_movz(_rd, _rs, _rt) emit_special1(movz, _rs, _rt, _rd, 0)
#define emit_max(_rd, _rs, _rt) emit_special1(max, _rs, _rt, _rd, 0)
#define emit_min(_rd, _rs, _rt) emit_special1(min, _rs, _rt, _rd, 0)
//  | (sllv|srlv|rotrv|srav) rd, rt, rs
#define emit_sllv(_rd, _rt, _rs) emit_special1(sllv, _rs, _rt, _rd, 0)
#define emit_srlv(_rd, _rt, _rs) emit_special1(srlv_rotrv, _rs, _rt, _rd, 0)
#define emit_rotrv(_rd, _rt, _rs) emit_special1(srlv_rotrv, _rs, _rt, _rd, 1)
#define emit_srav(_rd, _rt, _rs) emit_special1(srav, _rs, _rt, _rd, 0)
//  | (sll|srl|sra) rd, rt, sa
#define emit_sll(_rd, _rt, _sa) emit_special1(sll, 0, _rt, _rd, _sa)
#define emit_srl(_rd, _rt, _sa) emit_special1(srl_rotr, 0, _rt, _rd, _sa)
#define emit_rotr(_rd, _rt, _sa) emit_special1(srl_rotr, 1, _rt, _rd, _sa)
#define emit_sra(_rd, _rt, _sa) emit_special1(sra, 0, _rt, _rd, _sa)
//  | (mult|multu|madd|maddu|msub|msubu) rs, rt
//  | (div|divu) rs, rt
#define emit_mult(_rs, _rt) emit_special1(mult, _rs, _rt, 0, 0)
#define emit_multu(_rs, _rt) emit_special1(multu, _rs, _rt, 0, 0)
#define emit_madd(_rs, _rt) emit_special1(madd, _rs, _rt, 0, 0)
#define emit_maddu(_rs, _rt) emit_special1(maddu, _rs, _rt, 0, 0)
#define emit_msub(_rs, _rt) emit_special1(msub, _rs, _rt, 0, 0)
#define emit_msubu(_rs, _rt) emit_special1(msubu, _rs, _rt, 0, 0)
#define emit_div(_rs, _rt) emit_special1(div, _rs, _rt, 0, 0)
#define emit_divu(_rs, _rt) emit_special1(divu, _rs, _rt, 0, 0)
//  | (mthi|mtlo) rs
#define emit_mfhi(_rd) emit_special1(mfhi, 0, 0, _rd, 0)
#define emit_mflo(_rd) emit_special1(mflo, 0, 0, _rd, 0)
//  | (mfhi|mflo) rd
#define emit_mthi(_rs) emit_special1(mthi, _rs, 0, 0, 0)
#define emit_mtlo(_rs) emit_special1(mtlo, _rs, 0, 0, 0)
//  | jr rs
#define emit_jr(_rs) emit_special1(jr, _rs, 0, 0, 0)
#define emit_jra() emit_jr(psp_ra)
//  | jalr rd, rs
#define emit_jalr(_rd, _rs) emit_special1(jalr, _rs, 0, _rd, 0)
//  | (break|syscall)

//  | (clz|clo) rd, rs
#define emit_clz(_rd, _rs) emit_special1(clz, _rs, 0, _rd, 0)
#define emit_clo(_rd, _rs) emit_special1(clo, _rs, 0, _rd, 0)
//  | (addi|addiu|slti|sltiu|andi|ori|xori) rt, rs, immediate
#define emit_addi(_rt, _rs, _imm) emit_imm(addi, _rs, _rt, _imm)
#define emit_addiu(_rt, _rs, _imm) emit_imm(addiu, _rs, _rt, _imm)
#define emit_slti(_rt, _rs, _imm) emit_imm(slti, _rs, _rt, _imm)
#define emit_sltiu(_rt, _rs, _imm) emit_imm(sltiu, _rs, _rt, _imm)
#define emit_andi(_rt, _rs, _imm) emit_imm(andi, _rs, _rt, _imm)
#define emit_ori(_rt, _rs, _imm) emit_imm(ori, _rs, _rt, _imm)
#define emit_xori(_rt, _rs, _imm) emit_imm(xori, _rs, _rt, _imm)
//  | (beq|bne) rs, rt, label
#define emit_beq(_rs, _rt, _offset) emit_imm(beq, _rs, _rt, psp_relative_target(_offset))
#define emit_bne(_rs, _rt, _offset) emit_imm(bne, _rs, _rt, psp_relative_target(_offset))
//  | (beql|bnel) rs, rt, label
#define emit_beql(_rs, _rt, _offset) emit_imm(beql, _rs, _rt, psp_relative_target(_offset))
#define emit_bnel(_rs, _rt, _offset) emit_imm(bnel, _rs, _rt, psp_relative_target(_offset))
//  | (blez|bgtz) rs, label
#define emit_blez(_rs, _offset) emit_imm(blez, _rs, 0, psp_relative_target(_offset))
#define emit_bgtz(_rs, _offset) emit_imm(bgtz, _rs, 0, psp_relative_target(_offset))
//  | (blezl|bgtzl) rs, label
#define emit_blezl(_rs, _offset) emit_imm(blezl, _rs, 0, psp_relative_target(_offset))
#define emit_bgtzl(_rs, _offset) emit_imm(bgtzl, _rs, 0, psp_relative_target(_offset))
//  | (bltz|bgez) rs, label
#define emit_bltz(_rs, _offset) emit_regimm(bltz, _rs, psp_relative_target(_offset))
#define emit_bgez(_rs, _offset) emit_regimm(bgez, _rs, psp_relative_target(_offset))
//  | (bltzl|bgezl) rs, label
#define emit_bltzl(_rs, _offset) emit_regimm(bltzl, _rs, psp_relative_target(_offset))
#define emit_bgezl(_rs, _offset) emit_regimm(bgezl, _rs, psp_relative_target(_offset))
//  | (bltzal|bgezal) rs, label
#define emit_bltzal(_rs, _offset) emit_regimm(bltzal, _rs,psp_relative_target(_offset))
#define emit_bgezal(_rs, _offset) emit_regimm(bltzal, _rs, psp_relative_target(_offset))
//  | (lb|lh/|lw|lbu|lhu|lwl|lwr) rt, immediate(rs)
#define emit_lb(_rt, _rs, _offset) emit_imm(lb, _rs, _rt, _offset)
#define emit_lbu(_rt, _rs, _offset) emit_imm(lbu, _rs, _rt, _offset)
#define emit_lh(_rt, _rs, _offset) emit_imm(lh, _rs, _rt, _offset)
#define emit_lhu(_rt, _rs, _offset) emit_imm(lhu, _rs, _rt, _offset)
#define emit_lw(_rt, _rs, _offset) emit_imm(lw, _rs, _rt, _offset)
#define emit_lwl(_rt, _rs, _offset) emit_imm(lwl, _rs, _rt, _offset)
#define emit_lwr(_rt, _rs, _offset) emit_imm(lwr, _rs, _rt, _offset)
#define emit_lwc1(_ft, _rs, _offset) emit_imm(lwc1, _rs, _ft, _offset)
//  | (sb|sh|sw|swl|swr) rt, immediate(rs)
#define emit_sb(_rt, _rs, _offset) emit_imm(sb, _rs, _rt, _offset)
#define emit_sh(_rt, _rs, _offset) emit_imm(sh, _rs, _rt, _offset)
#define emit_sw(_rt, _rs, _offset) emit_imm(sw, _rs, _rt, _offset)
#define emit_swl(_rt, _rs, _offset) emit_imm(swl, _rs, _rt, _offset)
#define emit_swr(_rt, _rs, _offset) emit_imm(swr, _rs, _rt, _offset)
#define emit_swc1(_ft, _rs, _offset) emit_imm(swc1, _rs, _ft, _offset)
//  | lui rt, immediate
#define emit_lui(_rt, _imm) emit_imm(lui, 0, _rt, _imm)
//  | j label
#define emit_j(_target) emit_jump(j, psp_absolute_target(_target))
//  | jal label
#define emit_jal(_target) emit_jump(jal, psp_absolute_target(_target))
//  | (ext|ins) rt, rs, msb, lsb
#define emit_ext(_rt, _rs, _msb, _lsb) emit_special3(ext, _rs, _rt, _msb-_lsb, _lsb)
#define emit_ins(_rt, _rs, _msb, _lsb) emit_special3(ins, _rs, _rt, _msb, _lsb)
//  | (seb|seh|bitrev|wsbh) rd, rt
#define emit_seb(_rd, _rt) emit_special3_bshfl(seb, _rt, _rd)
#define emit_seh(_rd, _rt) emit_special3_bshfl(seh, _rt, _rd)
#define emit_bitrev(_rd, _rt) emit_special3_bshfl(bitrev, _rt, _rd)
#define emit_wsbh(_rd, _rt) emit_special3_bshfl(wsbh, _rt, _rd)

#define emit_nop() emit_sll(psp_zero, psp_zero, 0)
#define emit_move(_rt, _rs) emit_addiu(_rt, _rs, 0)

#define emit_cop1bc(_func, _imm) \
	emit_itype(psp_cop1, psp_##_func, 0, _imm)

#define emit_cop1s(_func, _fs, _ft, _fd) \
	emit_ftype(psp_cop1, psp_cop1s, _fs, _ft, _fd, psp_##_func)

#define emit_cop1w(_func, _fs, _ft, _fd) \
	emit_ftype(psp_cop1, psp_cop1w, _fs, _ft, _fd, psp_##_func)

//  | (bc1f|bc1t|bc1fl|bc1tl) imm
#define emit_bc1f(_imm) emit_cop1bc(bc1f, ((u32)(_imm))>>2)
#define emit_bc1t(_imm) emit_cop1bc(bc1t, ((u32)(_imm))>>2)
#define emit_bc1fl(_imm) emit_cop1bc(bc1fl, ((u32)(_imm))>>2)
#define emit_bc1tl(_imm) emit_cop1bc(bc1tl, ((u32)(_imm))>>2)
//  | (add.s|sub.s|mul.s|div.s) fd, fs, ft
#define emit_adds(_fd, _fs, _ft) emit_cop1s(adds, _fs, _ft, _fd)
#define emit_subs(_fd, _fs, _ft) emit_cop1s(subs, _fs, _ft, _fd)
#define emit_muls(_fd, _fs, _ft) emit_cop1s(muls, _fs, _ft, _fd)
#define emit_divs(_fd, _fs, _ft) emit_cop1s(divs, _fs, _ft, _fd)
//  | (sqrt.s|abs.s|mov.s|neg.s) fd, fs
#define emit_sqrts(_fd, _fs) emit_cop1s(sqrts, _fs, 0, _fd)
#define emit_abss(_fd, _fs) emit_cop1s(abss, _fs, 0, _fd)
#define emit_movs(_fd, _fs) emit_cop1s(movs, _fs, 0, _fd)
#define emit_negs(_fd, _fs) emit_cop1s(negs, _fs, 0, _fd)
//  | (round.w.s|trunc.w.s|ceil.w.s|floor.w.s|cvt.w.s) fd, fs
#define emit_roundws(_fd, _fs) emit_cop1s(roundws, _fs, 0, _fd)
#define emit_truncws(_fd, _fs) emit_cop1s(truncws, _fs, 0, _fd)
#define emit_ceilws(_fd, _fs) emit_cop1s(ceilws, _fs, 0, _fd)
#define emit_floorws(_fd, _fs) emit_cop1s(floorws, _fs, 0, _fd)
#define emit_cvtws(_fd, _fs) emit_cop1s(cvtws, _fs, 0, _fd)
//  | (c.f.s|c.un.s|c.eq.s|c.ueq.s|c.olt.s|c.ult.s|c.ole.s|c.ule.s) fd, fs
//  | (c.sf.s|c.ngle.s|c.seq.s|c.ngl.s|c.lt.s|c.nge.s|c.le.s|c.ngt.s) fd, fs
#define emit_cconds(_fcond, _fs, _ft) emit_cop1s(cvtws, _fs, _ft, 0, (psp_cconds + _fcond))
#define emit_cfs(_fs, _ft) emit_cconds(psp_fcond_f, _fs, _ft)
#define emit_cuns(_fs, _ft) emit_cconds(psp_fcond_un, _fs, _ft)
#define emit_ceqs(_fs, _ft) emit_cconds(psp_fcond_eq, _fs, _ft)
#define emit_cueqs(_fs, _ft) emit_cconds(psp_fcond_ueq, _fs, _ft)
#define emit_colts(_fs, _ft) emit_cconds(psp_fcond_olt, _fs, _ft)
#define emit_cults(_fs, _ft) emit_cconds(psp_fcond_ult, _fs, _ft)
#define emit_coles(_fs, _ft) emit_cconds(psp_fcond_ole, _fs, _ft)
#define emit_cules(_fs, _ft) emit_cconds(psp_fcond_ule, _fs, _ft)
#define emit_csfs(_fs, _ft) emit_cconds(psp_fcond_sf, _fs, _ft)
#define emit_cngles(_fs, _ft) emit_cconds(psp_fcond_ngle, _fs, _ft)
#define emit_cseqs(_fs, _ft) emit_cconds(psp_fcond_seq, _fs, _ft)
#define emit_cngls(_fs, _ft) emit_cconds(psp_fcond_ngl, _fs, _ft)
#define emit_clts(_fs, _ft) emit_cconds(psp_fcond_lt, _fs, _ft)
#define emit_cnges(_fs, _ft) emit_cconds(psp_fcond_nge, _fs, _ft)
#define emit_cles(_fs, _ft) emit_cconds(psp_fcond_le, _fs, _ft)
#define emit_cngts(_fs, _ft) emit_cconds(psp_fcond_ngt, _fs, _ft)
//  | cvt.s.w fd, fs
#define emit_cvtsw(_fd, _fs) emit_cop1w(cvtsw, _fs, 0, _fd)
//  | (mfc1|mtc1) rt, fs
#define emit_mfc1(_rt, _fs) emit_ftype(psp_cop1, psp_mfc1, _fs, _rt, 0, 0)
#define emit_mtc1(_rt, _fs) emit_ftype(psp_cop1, psp_mtc1, _fs, _rt, 0, 0)


#define emit_vtype_vd_vs_vt(_op, _sub, _vsize, _vs, _vt, _vd) \
	do { psp_insn_t insn; insn.vtype3.op = _op; insn.vtype3.sub = _sub; insn.vtype3.one = 1&(u32(_vsize-1)>>0); insn.vtype3.two = 1&(u32(_vsize-1)>>1); insn.vtype3.vs = _vs; insn.vtype3.vt = _vt; insn.vtype3.vd = _vd; emit_insn(insn); } while(0)
#define emit_vtype_vd_vs(_op, _sub, _vsize, _vs, _vt, _vd) \
	do { psp_insn_t insn; insn.vtype3.op = _op; insn.vtype3.sub = _sub; insn.vtype3.one = 1&(u32(_vsize-1)>>0); insn.vtype3.two = 1&(u32(_vsize-1)>>1); insn.vtype3.vs = _vs; insn.vtype3.vt = _vt; insn.vtype3.vd = _vd; emit_insn(insn); } while(0)

#define emit_vadds(_vs, _vt, _vd) emit_vtype_vd_vs_vt(psp_vfpu0, 0, 1, _vs, _vt, _vd)
#define emit_vsubs(_vs, _vt, _vd) emit_vtype_vd_vs_vt(psp_vfpu0, 1, 1, _vs, _vt, _vd)

#define emit_vmuls(_vs, _vt, _vd) emit_vtype_vd_vs_vt(psp_vfpu1, 0, 1, _vs, _vt, _vd)
#define emit_vdotq(_vs, _vt, _vd) emit_vtype_vd_vs_vt(psp_vfpu1, 1, 4, _vs, _vt, _vd)
#define emit_vhdpp(_vs, _vt, _vd) emit_vtype_vd_vs_vt(psp_vfpu1, 4, 2, _vs, _vt, _vd)
//#define emit_vdivs(_vs, _vt, _vd) emit_vtype_vd_vs_vt(psp_vfpu1, 3, 1, _vs, _vt, _vd)

#define emit_vmovs(_vs, _vd) emit_vtype_vd_vs_vt(psp_vfpu4, 0, 1, _vs,  0, _vd)
#define emit_vabss(_vs, _vd) emit_vtype_vd_vs_vt(psp_vfpu4, 0, 1, _vs,  1, _vd)
#define emit_vnegs(_vs, _vd) emit_vtype_vd_vs_vt(psp_vfpu4, 0, 1, _vs,  2, _vd)
#define emit_vsqrts(_vs, _vd) emit_vtype_vd_vs_vt(psp_vfpu4, 0, 1, _vs, 13, _vd)
#define emit_vrcps(_vs, _vd) emit_vtype_vd_vs_vt(psp_vfpu4, 0, 1, _vs, 16, _vd)
#define emit_vrsqs(_vs, _vd) emit_vtype_vd_vs_vt(psp_vfpu4, 0, 1, _vs, 17, _vd)

#define emit_vrotp(_type, _vs, _vd) emit_vtype_vd_vs_vt(psp_vfpu6, 7, 2, _vs, (32|_type), _vd)
#define emit_vtfm4q(_vs, _vt, _vd) emit_vtype_vd_vs_vt(psp_vfpu6, 3, 4, _vs, _vt, _vd)

#define emit_vrotp_sc(_vs, _vd) emit_vrotp(1, _vs, _vd)

#define emit_vdivs(_vs, _vt, _vd) vrcps(_vs, _vd); emit_vmuls(_vs, _vt, _vd)

#define emit_lvs(_vt, _rs, _offset) emit_imm(lvs, _rs, (_vt)&15, (_offset&-4) | ((_vt>>4)&3))
#define emit_svs(_vt, _rs, _offset) emit_imm(svs, _rs, (_vt)&15, (_offset&-4) | ((_vt>>4)&3))

#define emit_lvq(_vt, _rs, _offset) emit_imm(lvq, _rs, (_vt)&15, (_offset&-4) | ((_vt>>4)&1))
#define emit_svq(_vt, _rs, _offset) emit_imm(svq, _rs, (_vt)&15, (_offset&-4) | ((_vt>>4)&1))

#define emit_mtv(_rt, _vd) emit_itype(psp_cop2, 3, _rt, _vd)

#define emit_vi2fs(_vs, _vd, _shift) emit_vtype_vd_vs_vt(psp_vfpu4, 5, 1, _vs,  (_shift&31), _vd)
#define emit_vf2izs(_vs, _vd, _shift) emit_vtype_vd_vs_vt(psp_vfpu4, 4, 1, _vs,  64|(_shift&31), _vd)

#define emit_vscmps(_vs, _vt, _vd) emit_vtype_vd_vs_vt(psp_vfpu3, 5, 1, _vs,  _vt, _vd)
#define emit_vsges(_vs, _vt, _vd) emit_vtype_vd_vs_vt(psp_vfpu3, 6, 1, _vs,  _vt, _vd)
#define emit_vslts(_vs, _vt, _vd) emit_vtype_vd_vs_vt(psp_vfpu3, 7, 1, _vs,  _vt, _vd)
