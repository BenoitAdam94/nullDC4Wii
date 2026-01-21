//Helper file for the emitter, includes varius handy register #defines and
//opcode shortcuts :)

//include the emit functions ..
#include "ppc_emit.h"


/*
Sadly this is empty for now.Can you fill it ?!
*/

/*
* Register numbers.
*/
enum ppc_ireg
{
	ppc_r0   = 0,
	ppc_r1   = 1, ppc_sp = ppc_r1,
	ppc_r2   = 2,
	ppc_r3   = 3, ppc_rrv0 = ppc_r3, ppc_rarg0 = ppc_r3,
	ppc_r4   = 4, ppc_rrv1 = ppc_r4, ppc_rarg1 = ppc_r4,
	ppc_r5   = 5, ppc_rarg2 = ppc_r5,
	ppc_r6   = 6, ppc_rarg3 = ppc_r6,
	ppc_r7   = 7, ppc_rarg4 = ppc_r7,
	ppc_r8   = 8, ppc_rarg5 = ppc_r8,
	ppc_r9   = 9, ppc_rarg6 = ppc_r9,
	ppc_r10  = 10, ppc_rarg7 = ppc_r10,
	ppc_r11  = 11,
	ppc_r12  = 12,
	ppc_r13  = 13,
	ppc_r14  = 14,
	ppc_r15  = 15,
	ppc_r16  = 16,
	ppc_r17  = 17,
	ppc_r18  = 18,
	ppc_r19  = 19,
	ppc_r20  = 20,
	ppc_r21  = 21,
	ppc_r22  = 22,
	ppc_r23  = 23,
	ppc_r24  = 24,
	ppc_r25  = 25,
	ppc_r26  = 26,
	ppc_r27  = 27,
	ppc_r28  = 28,
	ppc_r29  = 29,
	ppc_r30  = 30,
	ppc_r31  = 31,

	/* PPC Special registers */
	ppc_spr_xer  = 32,
	ppc_spr_lr   = 256,
	ppc_spr_ctr  = 288,

};

enum ppc_cr_reg
{
	ppc_cr0 = 0,
	ppc_cr1 = 1,
	ppc_cr2 = 2,
	ppc_cr3 = 3,
	ppc_cr4 = 4,
	ppc_cr5 = 5,
	ppc_cr6 = 6,
	ppc_cr7 = 7,
};

/*
* Floating-point register numbers.
*/
enum ppc_freg
{
	ppc_f0   = 0, 
	ppc_f1   = 1, ppc_frv0 = ppc_f1, ppc_farg0 = ppc_f1,
	ppc_f2   = 2, ppc_frv1 = ppc_f2, ppc_farg1 = ppc_f2,
	ppc_f3   = 3, ppc_frv2 = ppc_f3, ppc_farg2 = ppc_f3,
	ppc_f4   = 4, ppc_frv3 = ppc_f4, ppc_farg3 = ppc_f4,
	ppc_f5   = 5, ppc_farg4 = ppc_f5,
	ppc_f6   = 6, ppc_farg5 = ppc_f6,
	ppc_f7   = 7, ppc_farg6 = ppc_f7,
	ppc_f8   = 8, ppc_farg7 = ppc_f8,
	ppc_f9   = 9, ppc_farg8 = ppc_f9,
	ppc_f10  = 10, ppc_farg9 = ppc_f10,
	ppc_f11  = 11, ppc_farg10 = ppc_f11,
	ppc_f12  = 12, ppc_farg11 = ppc_f12,
	ppc_f13  = 13, ppc_farg12 = ppc_f13,
	ppc_f14  = 14,
	ppc_f15  = 15,
	ppc_f16  = 16,
	ppc_f17  = 17,
	ppc_f18  = 18,
	ppc_f19  = 19,
	ppc_f20  = 20,
	ppc_f21  = 21,
	ppc_f22  = 22,
	ppc_f23  = 23,
	ppc_f24  = 24,
	ppc_f25  = 25,
	ppc_f26  = 26,
	ppc_f27  = 27,
	ppc_f28  = 28,
	ppc_f29  = 29,
	ppc_f30  = 30,
	ppc_f31  = 31,

};

#define BO_DEC_NZ_FALSE		0x00
#define BO_DEC_Z_FALSE		0x02
#define BO_FALSE			0x04
#define BO_DEC_NZ_TRUE		0x08
#define BO_DEC_Z_TRUE		0x0A
#define BO_TRUE				0x0C
#define BO_DEC_NZ			0x10
#define BO_DEC_Z			0x12
#define BO_ALWAYS			0x14

#define BO_HINT_TAKEN		0x01

#define BI_CR0_LT   0
#define BI_CR0_GT   1
#define BI_CR0_EQ   2
#define BI_CR0_SO   3
#define BI_CR0_UN   3

#define BI_CR1_LT   4
#define BI_CR1_GT   5
#define BI_CR1_EQ   6
#define BI_CR1_SO   7
#define BI_CR1_UN   7

#define BI_CR2_LT   8
#define BI_CR2_GT   9
#define BI_CR2_EQ   10
#define BI_CR2_SO   11
#define BI_CR2_UN   11

#define BI_CR3_LT   12
#define BI_CR3_GT   13
#define BI_CR3_EQ   14
#define BI_CR3_SO   15
#define BI_CR3_UN   15

#define BI_CR4_LT   16
#define BI_CR4_GT   17
#define BI_CR4_EQ   18
#define BI_CR4_SO   19
#define BI_CR4_UN   19

#define BI_CR5_LT   20
#define BI_CR5_GT   21
#define BI_CR5_EQ   22
#define BI_CR5_SO   23
#define BI_CR5_UN   23

#define BI_CR6_LT   24
#define BI_CR6_GT   25
#define BI_CR6_EQ   26
#define BI_CR6_SO   27
#define BI_CR6_UN   27

#define BI_CR7_LT   28
#define BI_CR7_GT   29
#define BI_CR7_EQ   30
#define BI_CR7_SO   31
#define BI_CR7_UN   31

//move to link register
#define  ppc_mtlr(S)     ppc_mtspr(ppc_spr_lr,S)

//move to counter register
#define  ppc_mtctr(S)    ppc_mtspr(ppc_spr_ctr,S)

//move to xer register
#define  ppc_mtxer(S)    ppc_mtspr(ppc_spr_xer,S)

//branch to link register
#define ppc_blr() ppc_bclrx(BO_ALWAYS,BI_CR0_EQ,0)

//branch to counter register
#define ppc_bctr() ppc_bcctrx(BO_ALWAYS,BI_CR0_EQ,0)

//mov register to register
#define ppc_mov(D,S) ppc_ori(D,S,0)


struct ppc_label
{
	void MarkLabel()
	{
		//get the patch address
		u32* pt=(u32*)this;

		//16 bit disp
		u32 offs=(u8*)emit_GetCCPtr()-(u8*)pt;
		offs&=0xFFFF;
		offs>>=2;

		*pt|=offs<<2;
	}
};

//this actually allocates no memory !
static inline ppc_label* ppc_CreateLabel() { return (ppc_label*)emit_GetCCPtr(); }