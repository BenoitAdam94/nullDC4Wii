/*
**  armwreck.h
**
**  Its an arm rec .. written by me, so you know its a wreck
*/
#ifndef __armwreck_h
#define __armwreck_h

namespace ARM_Wreck     // to use generic names
{


    // if ! using armwreck compiler
    extern "C" u8* CodeCache;
    extern "C" u8* CodeIndex;


/*
    static void ClearCache()
    {
        asm("isb");
        asm("dsb");
        asm("dmb");

        syscall(2);

        __clear_cache(CodeCache, CodeCache+CODE_SIZE);
        //__ARM_NR_cacheflush();


        //asm("iciallu");
        //asm("MCR p15, 0, r0, c7, c1, 0");   // other encoding, r1=...

        //asm("swi 0x9f0002");    // @ sys_cacheflush
    }
*/




    struct t_label
    {
        u8* Addr;
        char ID[256];
    };


    extern vector<t_label> Labels;

    bool AddLabel(const char *ID)
    {
        if(0!=emit_ptr)
            return false;

        t_label label;
        strcpy(label.ID,ID);
        label.Addr=(u8*)emit_GetCCPtr();
        Labels.push_back(label);
        return true;
    }

    // Note:  this function is for general use, specific compiler must check the value
    //  returned to determine if it is suitable for use.  (ie: 12b for arm)

    snat Literal(const char *label)
    {
        u8* label_addr=NULL;

        for(u32 l=0; l<Labels.size(); l++)
        {
            if(0==strcmp(Labels[l].ID,label)) {
                label_addr = Labels[l].Addr;
                break;
            }
        }
        if(NULL==label_addr)
            return 0;

        u8* pc_addr = (u8*)emit_GetCCPtr();
        //return -(snat)((pc_addr+8)-(snat)label_addr);
        return (snat)((snat)label_addr - ((snat)pc_addr+8));
    }
    snat Literal(unat FnAddr)
    {
        u8* pc_addr = (u8*)emit_GetCCPtr();
        return (snat)((snat)FnAddr - ((snat)pc_addr+8));
        //return -(snat)((pc_addr+8)-(snat)FnAddr);
    }

    unat Absolute(const char *label)
    {
        for(u32 l=0; l<Labels.size(); l++)
        {
            if(0==strcmp(Labels[l].ID,label))
                return (unat)Labels[l].Addr;
        }
        return 0;
    }

#ifdef USE_ARMWRECK_COMPILER
class Buffer
{
public:

    Buffer()
    {
        Index=0;
        IsValid=false;
    }

    ~Buffer()
    {
        if(IsValid)
            munmap(CBlock,CBSize);
        IsValid=false;
    }

    bool Get()
    {
        if(IsValid)
            return false;

        CBSize=sysconf(_SC_PAGESIZE);
        if(CBSize < 512) {
            printf("bad page size: %d\n", sysconf(_SC_PAGESIZE));
            return false;
        }

        // use mmap so its on a page boundary
        // mprotect will make PROT_EXEC *LATER*
        CBlock = (u32*)mmap(NULL, CBSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        if(MAP_FAILED == CBlock) {
            printf("mmap() failed!\n");
            return false;
        }

        memset(CBlock,0xCD,512);
        IsValid=true;
        return IsValid;
    }

    bool Set(u32 Instruction)
    {
        if(!IsValid)
            return false;

        if(Index>=CBSize)
            return false;

        CBlock[Index++] = Instruction;
        return true;
    }

    bool Exec()
    {
        if(!IsValid || 0==Index)
            return false;

        /* Mark the buffer executable. */
        if (mprotect(CBlock, CBSize, PROT_EXEC)) {
            perror("Couldn’t mprotect");
            return false;
        }

        ClearCache();

        for(int i=0; i<Index; i++)
            printf("%08X\t%08X\n", (u32)CBlock+(i<<2), CBlock[i]);

        printf("Executing Code Block @ %p (%d Bytes)!\n", CBlock, Index<<2);
        ((void (*)(void))CBlock)();

        return true;
    }


    bool IsValid;

    u32 Index;
    u32 CBSize;
    u32 *CBlock __attribute__ ((aligned (8)));
};

#define MAX_BLOCKS  10

class Compiler
{
public:

    Compiler()
    {
        Index=0;
    }

    static u32 Index;
    static Buffer Block[MAX_BLOCKS];


    static bool SetBlock(u32 idx)
    {
        if(idx>MAX_BLOCKS)
            return false;

        Index=idx;

        if(!Block[Index].Get()) {
            printf("Error, Couldn't get buffer!\n");
            return false;
        }
        return true;
    }

    static bool RewindBlock(u32 idx)
    {
        // TODO,  or change SetBlock || Block::Get()
        //  issues:  block may not even be readable, it should be marked executable
        //  check how mprotect works,  if it OR's the flags in,  or whate
        //  cache:  this might play a huge role anyhow in rec.
        //  there could be issues on creation if used immediately after or who knows..
        //  but a quick modify might be sure to set it off ...

        //  best case scenario is make a rewind that will just

        //  mprotect( set to READ|WRITE ONLY )

        //  Block[idx].Index=0

        //  OR munmap the block and remap it !

        //  craziness:  try to gets the blocks address from mmap() even
        //      but this is sure to cause collisions with small blocks !
    }

    static bool Emit(u32 Instruction)
    {
        if(!Block[Index].Set(Instruction)) {
            printf("Error, Couldn't emit 0xe1a0f00eL!\n");
            return false;
        }
        return true;
    }

    static bool Exec(s32 CBlock=-1)
    {
        u32 RealIdx = (-1==CBlock) ? Index : CBlock ;

        if((RealIdx != Index) && (RealIdx>MAX_BLOCKS))
            return false;

        if(!Block[RealIdx].Exec()) {
            printf("Error, Couldn't execute block!\n");
            return false;
        }
        return true;
    }



    static bool Test()
    {
        if(!SetBlock(0)) {
            printf("Error, Couldn't set block!\n");
            return false;
        }

        if(!Emit(0xE1A0F00E)) {
            return false;
        }
        if(!Exec()) {
            return false;
        }
        return true;
    }
};

#endif // USE_ARMWRECK_COMPILER












enum eCC
{
    CC_EQ     =   0x00,   // 0000 EQ Equal Z set
    CC_NE     =   0x01,   // 0001 NE Not equal Z clear
    CC_CS     =   0x02,   // 0010 CS/HS Carry set/unsigned higher or same C set
    CC_CC     =   0x03,   // 0011 CC/LO Carry clear/unsigned lower C clear
    CC_MI     =   0x04,   // 0100 MI Minus/negative N set
    CC_PL     =   0x05,   // 0101 PL Plus/positive or zero N clear
    CC_VS     =   0x06,   // 0110 VS Overflow V set
    CC_VC     =   0x07,   // 0111 VC No overflow V clear
    CC_HI     =   0x08,   // 1000 HI Unsigned higher C set and Z clear
    CC_LS     =   0x09,   // 1001 LS Unsigned lower or same C clear or Z set
    CC_GE     =   0x0A,   // 1010 GE Signed greater than or equal N set and V set, or N clear and V clear (N == V)
    CC_LT     =   0x0B,   // 1011 LT Signed less than N set and V clear, or N clear and V set (N != V)
    CC_GT     =   0x0C,   // 1100 GT Signed greater than Z clear, and either N set and V set, or N clear and V clear (Z == 0,N == V)
    CC_LE     =   0x0D,   // 1101 LE Signed less than or equal Z set, or N set and V clear, or N clear and V set (Z == 1 or N != V)
    CC_AL     =   0x0E,   // 1110 AL Always (unconditional) -
    CC_B1     =   0x0F,   // 1111 - See Condition code 0b1111
};



/*
Reg.    Alt. Name  	Usage
r0      a1 	        First function argument     Integer function result     Scratch register
r1 	    a2 	        Second function argument    Scratch register
r2  	a3 	        Third function argument     Scratch register
r3  	a4 	        Fourth function argument    Scratch register
r4  	v1 	        Register variable
r5  	v2 	        Register variable
r6  	v3 	        Register variable
r7 	    v4      	Register variable
r8 	    v5 	        Register variable
r9 	    v6  rfp 	Register variable           Real frame pointer
r10 	sl 	        Stack limit
r11 	fp 	        Argument pointer
r12 	ip 	        Temporary workspace
r13 	sp 	        Stack pointer
r14 	lr 	        Link register Workspace
r15 	pc 	        Program counter
*/

enum eReg
{
    r0  = 0,
    r1, r2, r3, r4,
    r5, r6, r7, r8,
    r9, r10,r11,r12,
    r13,r14,r15,

    a1  = r0,    a2  = r1,    a3  = r2,    a4  = r3,
    v1  = r4,    v2  = r5,    v3  = r6,    v4  = r7,
    v5  = r8,    v6  = r9,    rfp = r9,    sl  = r10,
    fp  = r11,   ip  = r12,   sp  = r13,   lr  = r14,
    pc  = r15,
};

enum eFPR
{
    q0=0,q1,  q2,  q3,
    q4,  q5,  q6,  q7,
    q8,  q9,  q10, q11,
    q12, q13, q14, q15,

    d0=0,d1,  d2,  d3,
    d4,  d5,  d6,  d7,
    d8,  d9,  d10, d11,
    d12, d13, d14, d15,
    d16, d17, d18, d19,
    d20, d21, d22, d23,
    d24, d25, d26, d27,
    d28, d29, d30, d31,

    s0=0,s1,  s2,  s3,
    s4,  s5,  s6,  s7,
    s8,  s9,  s10, s11,
    s12, s13, s14, s15,
    s16, s17, s18, s19,
    s20, s21, s22, s23,
    s24, s25, s26, s27,
    s28, s29, s30, s31
};


enum ePushPopReg
{
    _r0 =0x0001,    _r1 =0x0002,    _r2 =0x0004,    _r3 =0x0008,
    _r4 =0x0010,    _r5 =0x0020,    _r6 =0x0040,    _r7 =0x0080,
    _r8 =0x0100,    _r9 =0x0200,    _r10=0x0400,    _r11=0x0800,
    _r12=0x1000,    _r13=0x2000,    _r14=0x4000,    _r15=0x8000,

    _a1  = _r0,    _a2  = _r1,    _a3  = _r2,    _a4  = _r3,
    _v1  = _r4,    _v2  = _r5,    _v3  = _r6,    _v4  = _r7,
    _v5  = _r8,    _v6  = _r9,    _rfp = _r9,    _sl  = _r10,
    _fp  = _r11,   _ip  = _r12,   _sp  = _r13,   _lr  = _r14,
    _pc  = _r15,

    _push_all   = 0xFFFF,       // Save All 15
    _push_call  = _lr|_rfp,     // Save lr && _rfb(cycle count)

    _push_eabi  = _lr|_v1|_v2|_v3|_v4|_v5|_v6|_sl|_fp|_ip,  // this is guesswork ..
};



//////////// ARM Encoding methods for the emitter, decided not to trust gcc with constants
///////////////// so using DECL_Id and manually OR'ing the shit ;)~ (so probably xtra bug its 3:33am!)

///////////////////////////////////////////////////////////////////////////////////
#ifndef RELEASE
///////////////////////////////////////////////////////////////////////////////////


#define WreckAPI void

#define DECL_I   \
    u32 Instruction=0

#define DECL_Id(d)   \
    u32 Instruction=(d)





///////////////////////////////////////////////////////////////////////////////////
#else
///////////////////////////////////////////////////////////////////////////////////

#define _inlineExSVoidA __extension__ static __inline void __attribute__ ((__always_inline__))

#define WreckAPI \
    inline static void

#define DECL_I   \
    static u32 Instruction; \
        Instruction=0

#define DECL_Id(d)   \
    static u32 Instruction; \
        Instruction=(d)

///////////////////////////////////////////////////////////////////////////////////
#endif //_DEBUG
///////////////////////////////////////////////////////////////////////////////////





#define I   \
    (Instruction)

#define SET_CC  \
    I |= (((u32)CC&15)<<28)



#define USE_NDC_EMITTER
#ifdef  USE_NDC_EMITTER



#define EMIT_I              \
        emit_Write32((I));  \


#else

#define EMIT_I  \
    printf("Error, Add emitter!\n")

#endif




/*
        ** Branches **
*/





WreckAPI B(u32 sImm24, eCC CC=CC_AL)
{
    DECL_Id(0x0A000000);

    SET_CC;
    I |= ((sImm24>>2)&0xFFFFFF);
    EMIT_I;
}

WreckAPI BL(u32 sImm24, eCC CC=CC_AL)
{
    DECL_Id(0x0B000000);

    SET_CC;
    I |= ((sImm24>>2)&0xFFFFFF);
    EMIT_I;
}


// This is broken !! encoding looks correct,  it segfaults,  the pc val is align(pc,4) but this should be right in ARM
// *FIXME*
#if 0
WreckAPI BLX(u32 sImm24)   // Form I     *FIXME* H is derived so not needed, fixup sImm24 so one can just pass a real addr
{
    DECL_Id(0xFA000000);

    //if(sImm24&1)
    //    I |= 1<<24; // SET_H

    I |= ((sImm24>>2)&0xFFFFFF);
    EMIT_I;
}
#endif

WreckAPI BLX(eReg Rm, eCC CC=CC_AL)   // Form II
{
    DECL_Id(0x012FFF30);

    SET_CC;
    I |= (Rm&15);
    EMIT_I;
}




// Either X variant will switch to THUMB* if bit0 of addr is 1

WreckAPI BX(eReg Rm, eCC CC=CC_AL)
{
    DECL_Id(0x012FFF10);

    SET_CC;
    I |= (Rm&15);
    EMIT_I;
}


WreckAPI BXJ(eReg Rm, eCC CC=CC_AL)
{
    DECL_Id(0x012FFF20);

    SET_CC;
    I |= (Rm&15);
    EMIT_I;
}









/*
        ** Data Processing **
*/










enum DP_OPCODE
{
    DP_AND, // 0000 AND Logical AND             Rd := Rn AND shifter_operand
    DP_EOR, // 0001 EOR Logical Exclusive OR    Rd := Rn EOR shifter_operand
    DP_SUB, // 0010 SUB Subtract                Rd := Rn - shifter_operand
    DP_RSB, // 0011 RSB Reverse Subtract        Rd := shifter_operand - Rn
    DP_ADD, // 0100 ADD Add                     Rd := Rn + shifter_operand
    DP_ADC, // 0101 ADC Add with Carry          Rd := Rn + shifter_operand + Carry Flag
    DP_SBC, // 0110 SBC Subtract with Carry     Rd := Rn - shifter_operand - NOT(Carry Flag)
    DP_RSC, // 0111 RSC Reverse Subtract with Carry Rd := shifter_operand - Rn - NOT(Carry Flag)
    DP_TST, // 1000 TST Test Update flags after Rn AND shifter_operand
    DP_TEQ, // 1001 TEQ Test Equivalence Update flags after Rn EOR shifter_operand
    DP_CMP, // 1010 CMP Compare Update flags after Rn - shifter_operand
    DP_CMN, // 1011 CMN Compare Negated Update flags after Rn + shifter_operand
    DP_ORR, // 1100 ORR Logical (inclusive) OR  Rd := Rn OR shifter_operand
    DP_MOV, // 1101 MOV Move                    Rd := shifter_operand (no first operand)
    DP_BIC, // 1110 BIC Bit Clear               Rd := Rn AND NOT(shifter_operand)
    DP_MVN  // 1111 MVN Move Not                Rd := NOT shifter_operand (no first operand)
};

        // IMPORTANT NOTE,  the I bit 0x0200 0000  NOT being set
        // and bit[7] && bit[4] of Shifter ARE set
        // The instruction is NOT one of these data processing instructions !

        // ALSO We aren't handling the S bit, (Set flags)
        //   *FIXME* *TODO*  -- can we simply set the I bit ????  NOOOOO

    // --->   DO NOT TRY TO USE THESE - UNLESS YOU MANUALLY CALC SHIFTER
    //          EVEN THEN YOU CAN'T USE MANY FORMS B/C of I Bit
    //  ARM ENCODING IS ________FUBAR++++++++


///////////////////// Lets try one (zillion) more time(s)

// ffs at least by now  we know i didn't cheat and use someone elses shit,
// prob would have been done hours ago




/////////////////////////// shifters - high 20b are always similar ///////////////

    // *FIXME* use some damn macro header .def---- damn its 4:52am and this is going sloooowly
    // brain on drugs... no .. brain on lack of sleep && arm encoding is MUCH worse ... dont let your kids ..

#if 0
1. #<immediate>             See Data-processing operands - Immediate on page A5-6.
2. <Rm>                     See Data-processing operands - Register on page A5-8.
3. <Rm>, LSL #<shift_imm>   See Data-processing operands - Logical shift left by immediate on page A5-9.
4. <Rm>, LSL <Rs>           See Data-processing operands - Logical shift left by register on page A5-10.
5. <Rm>, LSR #<shift_imm>   See Data-processing operands - Logical shift right by immediate on page A5-11.
6. <Rm>, LSR <Rs>           See Data-processing operands - Logical shift right by register on page A5-12.
7. <Rm>, ASR #<shift_imm>   See Data-processing operands - Arithmetic shift right by immediate on page A5-13.
8. <Rm>, ASR <Rs>           See Data-processing operands - Arithmetic shift right by register on page A5-14.
9. <Rm>, ROR #<shift_imm>   See Data-processing operands - Rotate right by immediate on page A5-15.
10. <Rm>, ROR <Rs>          See Data-processing operands - Rotate right by register on page A5-16.
11. <Rm>, RRX               See Data-processing operands - Rotate right with extend on page A5-17.
#endif



struct Shifter          // yep just leave it TODO *FIXME*
{
    union   // go fuck a goat
    {
        struct {
            //u32 lowbitsfirst: 16;
            u32 sc:  16;
        } IMM;/*
        struct {
            u32 sc:  16;
        } REG;
        struct {
            u32 sc:  16;
        } LSL_IMM;
        struct {
            u32 sc:  16;
        } LSL_REG;
        struct {
            u32 sc:  16;
        } LSR_IMM;
        struct {
            u32 sc:  16;
        } LSR_REG;
        struct {
            u32 sc:  16;
        } ASR_IMM;
        struct {
            u32 sc:  16;
        } ASR_REG;
        struct {
            u32 sc:  16;
        } ROR_IMM;
        struct {
            u32 sc:  16;
        } ROR_REG;
        struct {
            u32 sc:  16;
        } RRX;*/

        u32 Raw;        // Really only u12 !
    };
};


#define DP_PARAMS   (eReg Rd, eReg Rn, Shifter Sh, eCC CC=CC_AL)
#define DP_RPARAMS  (eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)

#define DP_COMMON           \
    DECL_I;                 \
\
    SET_CC;                 \
    I |= (Rn&15)<<16;       \
    I |= (Rd&15)<<12;       \
    I |= Sh.Raw&0xFFF

#define DP_RCOMMON           \
    DECL_I;                 \
\
    SET_CC;                 \
    I |= (Rn&15)<<16;       \
    I |= (Rd&15)<<12;       \
    I |= (Rm&15)

#define DP_OPCODE(opcode)   \
    I |= (opcode)<<21


WreckAPI AND DP_PARAMS { DP_COMMON; DP_OPCODE(DP_AND); EMIT_I; }
WreckAPI EOR DP_PARAMS { DP_COMMON; DP_OPCODE(DP_EOR); EMIT_I; }
WreckAPI SUB DP_PARAMS { DP_COMMON; DP_OPCODE(DP_SUB); EMIT_I; }
WreckAPI RSB DP_PARAMS { DP_COMMON; DP_OPCODE(DP_RSB); EMIT_I; }
WreckAPI ADD DP_PARAMS { DP_COMMON; DP_OPCODE(DP_ADD); EMIT_I; }
WreckAPI ADC DP_PARAMS { DP_COMMON; DP_OPCODE(DP_ADC); EMIT_I; }
WreckAPI SBC DP_PARAMS { DP_COMMON; DP_OPCODE(DP_SBC); EMIT_I; }
WreckAPI RSC DP_PARAMS { DP_COMMON; DP_OPCODE(DP_RSC); EMIT_I; }
WreckAPI TST DP_PARAMS { DP_COMMON; DP_OPCODE(DP_TST); EMIT_I; }
WreckAPI TEQ DP_PARAMS { DP_COMMON; DP_OPCODE(DP_TEQ); EMIT_I; }
WreckAPI CMP DP_PARAMS { DP_COMMON; DP_OPCODE(DP_CMP); EMIT_I; }
WreckAPI CMN DP_PARAMS { DP_COMMON; DP_OPCODE(DP_CMN); EMIT_I; }
WreckAPI ORR DP_PARAMS { DP_COMMON; DP_OPCODE(DP_ORR); EMIT_I; }
WreckAPI MOV DP_PARAMS { DP_COMMON; DP_OPCODE(DP_MOV); EMIT_I; }
WreckAPI BIC DP_PARAMS { DP_COMMON; DP_OPCODE(DP_BIC); EMIT_I; }
WreckAPI MVN DP_PARAMS { DP_COMMON; DP_OPCODE(DP_MVN); EMIT_I; }



#if 0   // THESE ARE DEFF OUT OF ORDER //   USE AT OWN RISK  CMP/MOV Shifter(Reg)???????? fmt broken?
// Simple third reg type w/ no shifter
WreckAPI AND DP_RPARAMS { DP_RCOMMON; DP_OPCODE(DP_AND); EMIT_I; }
WreckAPI EOR DP_RPARAMS { DP_RCOMMON; DP_OPCODE(DP_EOR); EMIT_I; }
WreckAPI SUB DP_RPARAMS { DP_RCOMMON; DP_OPCODE(DP_SUB); EMIT_I; }
WreckAPI RSB DP_RPARAMS { DP_RCOMMON; DP_OPCODE(DP_RSB); EMIT_I; }
WreckAPI ADD DP_RPARAMS { DP_RCOMMON; DP_OPCODE(DP_ADD); EMIT_I; }
WreckAPI ADC DP_RPARAMS { DP_RCOMMON; DP_OPCODE(DP_ADC); EMIT_I; }
WreckAPI SBC DP_RPARAMS { DP_RCOMMON; DP_OPCODE(DP_SBC); EMIT_I; }
WreckAPI RSC DP_RPARAMS { DP_RCOMMON; DP_OPCODE(DP_RSC); EMIT_I; }
WreckAPI TST DP_RPARAMS { DP_RCOMMON; DP_OPCODE(DP_TST); EMIT_I; }
WreckAPI TEQ DP_RPARAMS { DP_RCOMMON; DP_OPCODE(DP_TEQ); EMIT_I; }
WreckAPI CMP DP_RPARAMS { DP_RCOMMON; DP_OPCODE(DP_CMP); EMIT_I; }
WreckAPI CMN DP_RPARAMS { DP_RCOMMON; DP_OPCODE(DP_CMN); EMIT_I; }
WreckAPI ORR DP_RPARAMS { DP_RCOMMON; DP_OPCODE(DP_ORR); EMIT_I; }
WreckAPI MOV DP_RPARAMS { DP_RCOMMON; DP_OPCODE(DP_MOV); EMIT_I; }
WreckAPI BIC DP_RPARAMS { DP_RCOMMON; DP_OPCODE(DP_BIC); EMIT_I; }
WreckAPI MVN DP_RPARAMS { DP_RCOMMON; DP_OPCODE(DP_MVN); EMIT_I; }
#endif



/*
    **  Multiply
*/

#if 0
MLA Multiply Accumulate. See MLA on page A4-66.
MUL Multiply. See MUL on page A4-80.
SMLA<x><y> Signed halfword Multiply Accumulate. See SMLA<x><y> on page A4-141.
SMLAD Signed halfword Multiply Accumulate, Dual. See SMLAD on page A4-144.
SMLAL Signed Multiply Accumulate Long. See SMLAL on page A4-146.
SMLAL<x><y> Signed halfword Multiply Accumulate Long. See SMLAL<x><y> on page A4-148.
SMLALD Signed halfword Multiply Accumulate Long, Dual. See SMLALD on page A4-150.
SMLAW<y> Signed halfword by word Multiply Accumulate. See SMLAW<y> on page A4-152.
SMLSD Signed halfword Multiply Subtract, Dual. See SMLAD on page A4-144.
SMLSLD Signed halfword Multiply Subtract Long Dual. See SMLALD on page A4-150.
SMMLA Signed Most significant word Multiply Accumulate. See SMMLA on page A4-158.
SMMLS Signed Most significant word Multiply Subtract. See SMMLA on page A4-158.
SMMUL Signed Most significant word Multiply. See SMMUL on page A4-162.
SMUAD Signed halfword Multiply, Add, Dual. See SMUAD on page A4-164.
SMUL<x><y> Signed halfword Multiply. See SMUL<x><y> on page A4-166.
SMULL Signed Multiply Long. See SMULL on page A4-168.
SMULW<y> Signed halfword by word Multiply. See SMULW<y> on page A4-170.
SMUSD Signed halfword Multiply, Subtract, Dual. See SMUSD on page A4-172.
UMAAL Unsigned Multiply Accumulate significant Long. See UMAAL on page A4-247.
UMLAL Unsigned Multiply Accumulate Long. See UMLAL on page A4-249.
UMULL Unsigned Multiply Long. See UMULL on page A4-251.
#endif



WreckAPI MLA(eReg Rd, eReg Rn, eReg Rs, eReg Rm, eCC CC=CC_AL)   //     *FIXME* S
{
    DECL_Id(0x00200090);

    SET_CC;
    I |= (Rd&15)<<16;
    I |= (Rn&15)<<12;
    I |= (Rs&15)<<8;
    I |= (Rm&15);
    EMIT_I;
}

WreckAPI MUL(eReg Rd, eReg Rs, eReg Rm, eCC CC=CC_AL)   //     *FIXME* S
{
    DECL_Id(0x00000090);

    SET_CC;
    I |= (Rd&15)<<16;
    I |= (Rs&15)<<8;
    I |= (Rm&15);
    EMIT_I;
}

// uh you dont need the rest right now do you ??

// SMUL UMUL you all MUL your mom






/*
    **  Parallel Add/Sub
*/


// {Q,S,U,US,UQ, USQUsubmarinne yea right i'm not adding these right now either






/*
    **  Extend (and add?)
*/

#if 0
SXTAB16 Sign extend bytes to halfwords, add halfwords.
SXTAB   Sign extend byte to word, add.
SXTAH   Sign extend halfword to word, add.
SXTB16  Sign extend bytes to halfwords.
SXTB    Sign extend byte to word.
SXTH    Sign extend halfword to word.
UXTAB16 Zero extend bytes to halfwords, add halfwords.
UXTAB   Zero extend byte to word, add.
UXTAH   Zero extend halfword to word, add.
UXTB16  Zero extend bytes to halfwords.
UXTB    Zero extend byte to word.
UXTH    Zero extend halfword to word.
#endif





WreckAPI SXTAB(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)   //     *FIXME* you think you get rotate? HAH sit on my thumb and rotate
{
    DECL_Id(0x06A00070);

    SET_CC;
    I |= (Rn&15)<<16;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}

WreckAPI SXTAB16(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)   //     *FIXME* you think you get rotate? Good joke
{
    DECL_Id(0x06800070);

    SET_CC;
    I |= (Rn&15)<<16;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}

WreckAPI SXTAH(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)   //     *FIXME* you think you get rotate? Good joke
{
    DECL_Id(0x06B00070);

    SET_CC;
    I |= (Rn&15)<<16;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}

WreckAPI SXTB(eReg Rd, eReg Rm, eCC CC=CC_AL)   //     *FIXME* still rot
{
    DECL_Id(0x06AF0070);

    SET_CC;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}

WreckAPI SXTB16(eReg Rd, eReg Rm, eCC CC=CC_AL)   //     *FIXME* rot
{
    DECL_Id(0x068F0070);

    SET_CC;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}

WreckAPI SXTH(eReg Rd, eReg Rm, eCC CC=CC_AL)   //     *FIXME* rot
{
    DECL_Id(0x06BF0070);

    SET_CC;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}




WreckAPI UXTAB(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)   //     *FIXME* rot
{
    DECL_Id(0x06E00070);

    SET_CC;
    I |= (Rn&15)<<16;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}

WreckAPI UXTAB16(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)   //     *FIXME* you think you get rotate? Good joke
{
    DECL_Id(0x06C00070);

    SET_CC;
    I |= (Rn&15)<<16;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}

WreckAPI UXTAH(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)   //     *FIXME* you think you get rotate? Good joke
{
    DECL_Id(0x06F00070);

    SET_CC;
    I |= (Rn&15)<<16;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}

WreckAPI UXTB(eReg Rd, eReg Rm, eCC CC=CC_AL)   //     *FIXME* rot
{
    DECL_Id(0x06EF0070);

    SET_CC;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}

WreckAPI UXTB16(eReg Rd, eReg Rm, eCC CC=CC_AL)   //     *FIXME* rot
{
    DECL_Id(0x06CF0070);

    SET_CC;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}

WreckAPI UXTH(eReg Rd, eReg Rm, eCC CC=CC_AL)   //     *FIXME* rot
{
    DECL_Id(0x06FF0070);

    SET_CC;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}



/*
    **  Misc. Arithmetic
*/


WreckAPI CLZ(eReg Rd, eReg Rm, eCC CC=CC_AL)
{
    DECL_Id(0x016F0F10);

    SET_CC;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}



WreckAPI USAD8(eReg Rd, eReg Rm, eReg Rs, eCC CC=CC_AL)
{
    DECL_Id(0x0780F010);

    SET_CC;
    I |= (Rd&15)<<16;
    I |= (Rs&15)<<8;
    I |= (Rm&15);
    EMIT_I;
}

WreckAPI USADA8(eReg Rd, eReg Rm, eReg Rs, eReg Rn, eCC CC=CC_AL)
{
    DECL_Id(0x07800010);

    SET_CC;
    I |= (Rd&15)<<16;
    I |= (Rn&15)<<12;
    I |= (Rs&15)<<8;
    I |= (Rm&15);
    EMIT_I;
}


/*
    ** Misc.
*/


#if 0
PKHBT (Pack Halfword Bottom Top)
PKHTB (Pack Halfword Top Bottom)
REV (Byte-Reverse Word)
REV16 (Byte-Reverse Packed Halfword)
REVSH (Byte-Reverse Signed Halfword) r
SEL (Select
SSAT (Signed Saturate)
SSAT16 Saturates two 16-bit signed values to a signed range.
USAT (Unsigned Saturate)
USAT16 Saturates two signed 16-bit
#endif

WreckAPI PKHBT(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)    // *FIXME* shift_imm .. nope
{
    DECL_Id(0x06800010);

    SET_CC;
    I |= (Rn&15)<<16;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}


WreckAPI PKHTB(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)    // *FIXME* shift_imm .. nope
{
    DECL_Id(0x06800050);

    SET_CC;
    I |= (Rn&15)<<16;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}


WreckAPI REV(eReg Rd, eReg Rm, eCC CC=CC_AL)
{
    DECL_Id(0x06BF0F30);

    SET_CC;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}


WreckAPI REV16(eReg Rd, eReg Rm, eCC CC=CC_AL)
{
    DECL_Id(0x06BF0FB0);

    SET_CC;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}

WreckAPI REVSH(eReg Rd, eReg Rm, eCC CC=CC_AL)
{
    DECL_Id(0x06FF0F30);

    SET_CC;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}


WreckAPI SEL(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)
{
    DECL_Id(0x06800FB0);

    SET_CC;
    I |= (Rn&15)<<16;
    I |= (Rd&15)<<12;
    I |= (Rm&15);
    EMIT_I;
}

#if 0
WreckAPI SSAT(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)
WreckAPI SSAT16(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)
WreckAPI USAT(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)
WreckAPI USAT16(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)
#endif










/*
    **  Status Reg. Instructions
*/

#if 0
MRS Move PSR to General-purpose Register. See MRS on page A4-74.
MSR Move General-purpose Register to PSR. See MSR on page A4-76.
CPS Change Processor State. Changes one or more of the processor mode and interrupt enable bits of the CPSR, without changing the other CPSR bits. See CPS on page A4-29.
SETEND Modifies the CPSR endianness, E, bit, without changing any other bits in the CPSR. See
SETEND on page A4-129.
#endif









/*
    ** Load / Store
*/


#if 0
LDR     Load Word.
LDRB    Load Byte.
LDRBT   Load Byte with User Mode Privilege.
LDRD    Load Doubleword.
LDREX   Load Exclusive.
LDRH    Load Unsigned Halfword.
LDRSB   Load Signed Byte.
LDRSH   Load Signed Halfword.
LDRT    Load Word with User Mode Privilege.
STR     Store Word.
STRB    Store Byte.
STRBT   Store Byte with User Mode Privilege.
STRD    Store Doubleword.
STREX   Store Exclusive.
STRH    Store Halfword.
STRT    Store Word with User Mode Privilege.
#endif






        ///////////// NEW ; MOVE / REORDER //////////////
/*
        WreckAPI MOV(eReg Rd, u32 Rm, eCC CC=CC_AL)
        {
            DECL_Id(0x01A000000);

            SET_CC;
            I |= (Rd&15)<<12;
            I |= (Rm&15);

            EMIT_I;
        }
*/
        WreckAPI MOVW(eReg Rd, u32 Imm16, eCC CC=CC_AL)
        {
            DECL_Id(0x03000000);

            SET_CC;
            I |= (Imm16&0xF000)<<4;
            I |= (Rd&15)<<12;
            I |= (Imm16&0x0FFF);
            EMIT_I;
        }

        WreckAPI MOVT(eReg Rd, u32 Imm16, eCC CC=CC_AL)
        {
            DECL_Id(0x03400000);

            SET_CC;
            I |= (Imm16&0xF000)<<4;
            I |= (Rd&15)<<12;
            I |= (Imm16&0x0FFF);
            EMIT_I;
        }


    // Load / Store //
/*
        WreckAPI LDR(eReg Rt, eReg Rn, eReg Rm, eCC CC=CC_AL)
        {
            DECL_Id(0x06100000);

            I |= (1<<23);   // SET_U (add)
            I |= (1<<24);   // SET_P (index)


            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rt&15)<<12;
            I |= (Rm&15);
            EMIT_I;
        }
*/


        WreckAPI LDR(eReg Rt, eReg Rn, s32 Imm12, eCC CC=CC_AL)
        {
            DECL_Id(0x04100000);

			I |= (1<<24);   // SET_P (index) // what ?

            if(0 != Imm12)
            {

                if(Imm12 > 0)
                    I |= (1<<23);   // SET_U (add)

                Imm12 = abs(Imm12);
                I |= ((u32)Imm12)&0xFFF;
            }

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rt&15)<<12;
            EMIT_I;
        }


        // Literal / PC Relative
/*

    *FIXME* TEST **

        WreckAPI LDR(eReg Rt, s32 Imm12, eCC CC=CC_AL)
        {
            DECL_Id(0x051F0000);

            // *FIXME* i'll have to fix all the imms and make them signed on input and check sign to set this bit and then save unsigned value&0xFFF
            if(Imm12 > 0)
                I |= (1<<23);

            SET_CC;
            I |= (Rt&15)<<12;
            I |= ((u32)Imm12)&0xFFF;
            EMIT_I;
        }
*/






/*
        WreckAPI LDRH(eReg Rt, eReg Rn, eReg Rm, eCC CC=CC_AL)
        {
            DECL_Id(0x001000B0);

            I |= (1<<23);   // SET_U (add)
            I |= (1<<24);   // SET_P (index)

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rt&15)<<12;
            I |= (Rm&15);
            EMIT_I;
        }
*/


        WreckAPI LDRH(eReg Rt, eReg Rn, s32 Imm8, eCC CC=CC_AL)
        {
            DECL_Id(0x005000B0);

			I |= (1<<24);   // SET_P (index)

            if(0 != Imm8)
            {
                if(Imm8 > 0)
                    I |= (1<<23);   // SET_U (add)

                Imm8 = abs(Imm8);
                I |= ((u32)Imm8 &0x0F);
                I |= ((u32)Imm8 &0xF0)<<4;
            }

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rt&15)<<12;
            EMIT_I;
        }








/*
        WreckAPI LDRB(eReg Rt, eReg Rn, eReg Rm, eCC CC=CC_AL)
        {
            DECL_Id(0x041000B0);

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rt&15)<<12;
            I |= (Rm&15);
            EMIT_I;
        }



        WreckAPI LDRB(eReg Rt, eReg Rn, s32 Imm12, eCC CC=CC_AL)
        {
            DECL_Id(0x045000B0);

            if(0 != Imm12)
            {
                I |= (1<<24);   // SET_P (index)

                if(Imm12 > 0)
                    I |= (1<<23);   // SET_U (add)

                Imm12 = abs(Imm12);
                I |= ((u32)Imm12)&0xFFF;
            }

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rt&15)<<12;
            EMIT_I;
        }
*/








/*
        WreckAPI STR(eReg Rt, eReg Rn, eReg Rm, eCC CC=CC_AL)
        {
            DECL_Id(0x06000000);

            I |= (1<<23);   // SET_U (add)
            I |= (1<<24);   // SET_P (index)

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rt&15)<<12;
            I |= (Rm&15);
            EMIT_I;
        }
*/
        WreckAPI STR(eReg Rt, eReg Rn, s32 Imm12, eCC CC=CC_AL)
        {
            DECL_Id(0x04000000);

            // *FIXME* i'll have to fix all the imms and make them signed on input and check sign to set this bit and then save unsigned value&0xFFF

			I |= (1<<24);   // SET_P (index)

            if(0 != Imm12)
            {

                if(Imm12 > 0)
                    I |= (1<<23);   // SET_U (add)

                Imm12 = abs(Imm12);
                I |= (Imm12&0xFFF);
            }

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rt&15)<<12;
            EMIT_I;
        }





        WreckAPI MOV(eReg Rd, eReg Rm, eCC CC=CC_AL)
        {
            DECL_Id(0x01A00000);

            SET_CC;
            I |= (Rd&15)<<12;
            I |= (Rm&15);
            EMIT_I;
        }

#if 0
        WreckAPI MOV(eReg Rd, u32 Imm32, eCC CC=CC_AL)  // dont use anyhow, uses expanded 8b (w/ C?) MOVW
        {
            DECL_Id(0x03A00000);

            SET_CC;
            I |= (Rd&15)<<12;
            I |= (Rm&15);
            EMIT_I;
        }
#endif




        WreckAPI CMP(eReg Rn, eReg Rm, eCC CC=CC_AL)
        {
            DECL_Id(0x01500000);

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rm&15);
            EMIT_I;
        }


        WreckAPI CMP(eReg Rn, s32 Imm8, eCC CC=CC_AL)
        {
            DECL_Id(0x03500000);

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Imm8&0xFF);  // *FIXME* 12b imm is 8b imm 4b rot. spec, add rot support!
            EMIT_I;
        }










        WreckAPI ADD(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)
        {
            DECL_Id(0x00800000);

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rd&15)<<12;
            I |= (Rm&15);
            EMIT_I;
        }

        WreckAPI ADD(eReg Rd, eReg Rn, s32 Imm8, eCC CC=CC_AL)
        {
            DECL_Id(0x02800000);

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rd&15)<<12;
            I |= (Imm8&0xFF);  // *FIXME* 12b imm is 8b imm 4b rot. spec, add rot support!
            EMIT_I;
        }



        WreckAPI ADR(eReg Rd, s32 Imm8, eCC CC=CC_AL)
        {
            DECL_Id(0x028F0000);

            SET_CC;
            I |= (Rd&15)<<12;
            I |= (Imm8&0xFF);  // *FIXME* 12b imm is 8b imm 4b rot. spec, add rot support!
            EMIT_I;
        }

        WreckAPI ADR_Zero(eReg Rd, s32 Imm8, eCC CC=CC_AL) // Special case for subtraction of 0
        {
            DECL_Id(0x024F0000);

            SET_CC;
            I |= (Rd&15)<<12;
            I |= (Imm8&0xFF);  // *FIXME* 12b imm is 8b imm 4b rot. spec, add rot support!
            EMIT_I;
        }


        WreckAPI ORR(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)
        {
            DECL_Id(0x01800000);

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rd&15)<<12;
            I |= (Rm&15);
            EMIT_I;
        }

        WreckAPI ORR(eReg Rd, eReg Rn, s32 Imm8, eCC CC=CC_AL)
        {
            DECL_Id(0x03800000);

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rd&15)<<12;
            I |= (Imm8&0xFF);  // *FIXME* 12b imm is 8b imm 4b rot. spec, add rot support!
            EMIT_I;
        }

        WreckAPI AND(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)
        {
            DECL_Id(0x00000000);

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rd&15)<<12;
            I |= (Rm&15);
            EMIT_I;
        }

        WreckAPI AND(eReg Rd, eReg Rn, s32 Imm8, eCC CC=CC_AL)
        {
            DECL_Id(0x02000000);

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rd&15)<<12;
            I |= (Imm8&0xFF);  // *FIXME* 12b imm is 8b imm 4b rot. spec, add rot support!
            EMIT_I;
        }


        WreckAPI EOR(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)
        {
            DECL_Id(0x00200000);

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rd&15)<<12;
            I |= (Rm&15);
            EMIT_I;
        }

        WreckAPI EOR(eReg Rd, eReg Rn, s32 Imm8, eCC CC=CC_AL)
        {
            DECL_Id(0x02200000);

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rd&15)<<12;
            I |= (Imm8&0xFF);  // *FIXME* 12b imm is 8b imm 4b rot. spec, add rot support!
            EMIT_I;
        }



        WreckAPI SUB(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)
        {
            DECL_Id(0x00400000);

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rd&15)<<12;
            I |= (Rm&15);
            EMIT_I;
        }

        WreckAPI SUB(eReg Rd, eReg Rn, s32 Imm8, eCC CC=CC_AL)
        {
            DECL_Id(0x02400000);

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rd&15)<<12;
            I |= (Imm8&0xFF);  // *FIXME* 12b imm is 8b imm 4b rot. spec, add rot support!
            EMIT_I;
        }


        WreckAPI RSB(eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL)
        {
            DECL_Id(0x00600000);

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rd&15)<<12;
            I |= (Rm&15);
            EMIT_I;
        }


        WreckAPI RSB(eReg Rd, eReg Rn, s32 Imm8, eCC CC=CC_AL)
        {
            DECL_Id(0x02600000);

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rd&15)<<12;
            I |= (Imm8&0xFF);  // *FIXME* 12b imm is 8b imm 4b rot. spec, add rot support!
            EMIT_I;
        }


        WreckAPI MVN(eReg Rd, eReg Rm, eCC CC=CC_AL)
        {
            DECL_Id(0x01E00000);

            SET_CC;
            I |= (Rd&15)<<12;
            I |= (Rm&15);
            EMIT_I;
        }


        WreckAPI MVN(eReg Rd, s32 Imm8, eCC CC=CC_AL)
        {
            DECL_Id(0x03E00000);

            SET_CC;
            I |= (Rd&15)<<12;
            I |= (Imm8&0xFF);  // *FIXME* 12b imm is 8b imm 4b rot. spec, add rot support!
            EMIT_I;
        }


        WreckAPI TST(eReg Rn, eReg Rm, eCC CC=CC_AL)
        {
            DECL_Id(0x01100000);

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Rm&15);
            EMIT_I;
        }


        WreckAPI TST(eReg Rn, u32 Imm12, eCC CC=CC_AL)
        {
            DECL_Id(0x03100000);

            SET_CC;
            I |= (Rn&15)<<16;
            I |= (Imm12&0xFFF);
            EMIT_I;
        }




#define REC_NO_STACK

#ifndef REC_NO_STACK

        // Must use _Reg format
        WreckAPI PUSH(ePushPopReg RegList, eCC CC=CC_AL)
        {
            DECL_Id(0x092D0000);

            SET_CC;
            I |= (RegList&0xFFFF);
            EMIT_I;
        }


        WreckAPI POP(ePushPopReg RegList, eCC CC=CC_AL)
        {
            DECL_Id(0x08BD0000);

            SET_CC;
            I |= (RegList&0xFFFF);
            EMIT_I;
        }

#endif





        WreckAPI DSB()
        {
            DECL_Id(0xF57FF04F);
            EMIT_I;
        }

        WreckAPI DMB()
        {
            DECL_Id(0xF57FF05F);
            EMIT_I;
        }

        WreckAPI ISB()
        {
            DECL_Id(0xF57FF06F);
            EMIT_I;
        }



























WreckAPI ERROR()
{
    DECL_Id(0xFFFFFFFF);
    EMIT_I;
}



WreckAPI NOP()
{
    DECL_Id(0xE1A00000);
    EMIT_I;
}

WreckAPI SVC(u32 code)
{
    DECL_Id(0x0F000000);
    I |= code&0xFFFFFF;
    EMIT_I;
}
#define SWI SVC













    // PSEUDO OPCODES //



        WreckAPI MOV32(eReg Rd, u32 Imm32, eCC CC=CC_AL)
        {
            MOVW(Rd,((Imm32)&0xFFFF),CC);
            MOVT(Rd,((Imm32>>16)&0xFFFF),CC);
        }



    // Helpers //

        WreckAPI CALL(unat FnAddr, eCC CC=CC_AL)
        {
            snat lit = Literal(FnAddr);

            if(0==lit) {
                printf("Error, Compiler caught NULL literal, CALL(%08X)\n", FnAddr);
                verify(false);
                return;
            }
            if( (lit<-33554432) || (lit>33554428) )     // ..28 for BL ..30 for BLX
            {
                printf("Warning, CALL(%08X) is out of range for literal(%08X)\n", FnAddr, lit);
               // verify(false);

                MOV32(v1,FnAddr,CC);
                BLX(v1, CC);
                return;
            }

            BL(lit,CC);
        }

        WreckAPI CALL(const char *Label, eCC CC=CC_AL)
        {
            snat lit = Literal(Label);

            printf("Call(%s) literal: %d > absolute: %08X\n", Label, lit, Absolute(Label));
            verify(false);

            if(0==lit) {
                printf("Error, Compiler caught NULL literal: %s\n", Label);
                verify(false);
                return;
            }
            if( (lit<-33554432) || (lit>33554428) )     // ..28 for BL ..30 for BLX
            {
                printf("Warning, Label: %s is out of range for imm call! \n", Label);
                verify(false);

                u32 abs = Absolute(Label);
                MOV32(v1,abs,CC);
                BL(v1, CC);
                return;
            }

            BL(lit,CC);
        }



        WreckAPI JUMP(unat FnAddr, eCC CC=CC_AL)
        {
            snat lit = Literal(FnAddr);

            if(0==lit) {
                printf("Error, Compiler caught NULL literal, JUMP(%08X)\n", FnAddr);
                verify(false);
                return;
            }
            if( (lit<-33554432) || (lit>33554428) )     // ..28 for BL ..30 for BLX
            {
                printf("Warning, %X is out of range for imm jump! \n", FnAddr);
                //verify(false);

                MOV32(v1,FnAddr,CC);
                BX(v1, CC);
                return;
            }

            B(lit,CC);     // Note, wont work for THUMB*,  have to use bx which is reg only !
        }

        WreckAPI JUMP(const char *Label, eCC CC=CC_AL)
        {
            snat lit = Literal(Label);

            if(0==lit) {
                printf("Error, Compiler caught NULL literal: JUMP(%s)\n", Label);
                verify(false);
                return;
            }
            if( (lit<-33554432) || (lit>33554428) )     // ..28 for BL ..30 for BLX
            {
                printf("Warning, Label: %s is out of range for imm jump! \n", Label);
                verify(false);

                u32 abs = Absolute(Label);
                MOV32(v1,abs,CC);
                BX(v1, CC);
                return;
            }

            B(lit,CC);
        }





    // Reg Helpers //

#define SHREG_OPT
#ifndef SHREG_OPT


        // you pick reg, loads Base with reg addr, no reg. mapping yet !
        WreckAPI LoadSh4Reg(eReg Rt, u32 Sh4_Reg, eCC CC=CC_AL)
        {
			MOV32(Rt, (u32)GetRegPtr(Sh4_Reg), CC);
			LDR(Rt,Rt,0, CC);
        }


        // you pick regs, loads Base with reg addr, no reg. mapping yet !
        // data should already exist for Rt !
        WreckAPI StoreSh4Reg(eReg Rt, eReg Rn, u32 Sh4_Reg, eCC CC=CC_AL)
        {
			MOV32(Rn, (u32)GetRegPtr(Sh4_Reg), CC);
			STR(Rt,Rn,0, CC);
        }

        WreckAPI StoreSh4RegImmVal(eReg Rt, eReg Rn, u32 Sh4_Reg, u32 Val, eCC CC=CC_AL)
        {
			MOV32(Rn, (u32)GetRegPtr(Sh4_Reg), CC);
			MOV32(Rt, Val, CC);
			STR(Rt,Rn,0, CC);
        }

#else

        // you pick reg, loads Base with reg addr, no reg. mapping yet !
        WreckAPI LoadSh4Reg(eReg Rt, u32 Sh4_Reg, eCC CC=CC_AL)
        {
            const u32 shRegOffs = (u8*)GetRegPtr(Sh4_Reg)-(u8*)&Sh4cntx ;

			LDR(Rt,r8,shRegOffs, CC);
        }


        // you pick regs, loads Base with reg addr, no reg. mapping yet !
        // data should already exist for Rt !
        WreckAPI StoreSh4Reg(eReg Rt,u32 Sh4_Reg, eCC CC=CC_AL)
        {
            const u32 shRegOffs = (u8*)GetRegPtr(Sh4_Reg)-(u8*)&Sh4cntx ;

			STR(Rt,r8,shRegOffs, CC);
        }

        WreckAPI StoreSh4RegImmVal(eReg Rt, u32 Sh4_Reg, u32 Val, eCC CC=CC_AL)
        {
            const u32 shRegOffs = (u8*)GetRegPtr(Sh4_Reg)-(u8*)&Sh4cntx ;

			MOV32(Rt, Val, CC);
			STR(Rt,r8,shRegOffs, CC);
        }

#endif



    // Load Helpers //  *FIXME* add possibility of literal calcs later  //


        WreckAPI LoadImmBase(eReg Rt, u32 Base, eCC CC=CC_AL)
        {
			MOV32(Rt, Base, CC);
			LDR(Rt,Rt,0, CC);
        }

        WreckAPI LoadImmBase(eReg Rt, eReg Rn, u32 Base, eCC CC=CC_AL)
        {
			MOV32(Rn, Base, CC);
			LDR(Rt,Rn,0, CC);
        }

        WreckAPI LoadImmBase16(eReg Rt, u32 Base, bool Extend=false, eCC CC=CC_AL)
        {
            MOV32(Rt, Base, CC);
            LDRH(Rt,Rt,0, CC);

			if(Extend)
                SXTH(Rt,Rt);
        }

        WreckAPI LoadImmBase16(eReg Rt, eReg Rn, u32 Base, bool Extend=false, eCC CC=CC_AL)
        {
			MOV32(Rn, Base, CC);
			LDRH(Rt,Rn,0, CC);

			if(Extend)
                SXTH(Rt,Rt);
        }
/*
            // *CHECK* *FIXME*
        WreckAPI LoadImmBase8(eReg Rt, u32 Base, bool Extend=false, eCC CC=CC_AL)
        {
			MOV32(Rt, Base, CC);
			LDRB(Rt,Rt,0, CC);

			if(Extend)
                SXTB(Rt,Rt);
        }

        WreckAPI LoadImmBase8(eReg Rt, eReg Rn, u32 Base, bool Extend=false, eCC CC=CC_AL)
        {
			MOV32(Rn, Base, CC);
			LDRB(Rt,Rn,0, CC);

			if(Extend)
                SXTB(Rt,Rt);
        }
*/

    // Store Helpers //


        // you pick regs, loads Base with reg addr, you supply data in Rt
        WreckAPI StoreImmBase(eReg Rt, eReg Rn, u32 Base, eCC CC=CC_AL)
        {
			MOV32(Rn, Base, CC);
			STR(Rt,Rn,0, CC);
        }

        // you pick regs, loads Rt with const val, you supply base for Rn
        WreckAPI StoreImmVal(eReg Rt, eReg Rn, u32 Val, eCC CC=CC_AL)
        {
			MOV32(Rt, Val, CC);
			STR(Rt,Rn,0, CC);
        }

        // you pick regs, loads Base with reg addr, loads Rt with const val
        WreckAPI StoreImms(eReg Rt, eReg Rn, u32 Base, u32 Val, eCC CC=CC_AL)
        {
			MOV32(Rn, Base, CC);
			MOV32(Rt, Val, CC);
			STR(Rt,Rn,0, CC);
        }











/*
    **  NEON Instructions
*/



WreckAPI NEON()
{
    DECL_Id(0xF2000000);
    EMIT_I;
}






WreckAPI VLDR(eFPR Dd, eReg Rn, s32 Imm8, eCC CC=CC_AL)   //  VFPv2, VFPv3, A.SIMD
{
    DECL_Id(0x0D100B00);

    //if(Imm8>=0)
    //  I |= (1<<23);           // U (add)
    //I |= (1<<22);             // D

    SET_CC;
    I |= (Rn&15)<<16;
    I |= (Dd&15)<<12;
    I |= (Imm8&0xFF);
    EMIT_I;
}

WreckAPI VLDR_VFP(eFPR Dd, eReg Rn, s32 Imm8, eCC CC=CC_AL)   //  VFPv2, VFPv3
{
	DECL_Id(0x0D100A00);

	if(Imm8>=0)
		I |= (1<<23);           // U (add)
	else
		Imm8=-Imm8;

	SET_CC;
	I |= (Rn&15)<<16;
	I |= (Dd&15)<<12;
	I |= (Imm8&0xFF);
	EMIT_I;
}

WreckAPI VSTR_VFP(eFPR Dd, eReg Rn, s32 Imm8, eCC CC=CC_AL)   //  VFPv2, VFPv3
{
	DECL_Id(0x0D000A00);

	if(Imm8>=0)
		I |= (1<<23);           // U (add)
	else
		Imm8=-Imm8;
	//I |= (1<<22);             // D

	SET_CC;
	I |= (Rn&15)<<16;
	I |= (Dd&15)<<12;
	I |= (Imm8&0xFF);
	EMIT_I;
}


WreckAPI VMOV(eFPR Dd, eReg Rt, eCC CC=CC_AL)   //  VFPv2, VFPv3, A.SIMD
{
    DECL_Id(0x0E000B10);

    SET_CC;
    I |= (Dd&15)<<16;
    I |= (Rt&15)<<12;
    EMIT_I;
}

WreckAPI VMOV(eReg Rt, eFPR Dd, eCC CC=CC_AL)   //  VFPv2, VFPv3, A.SIMD
{
    DECL_Id(0x0E100B10);

    SET_CC;
    I |= (Dd&15)<<16;
    I |= (Rt&15)<<12;
    EMIT_I;
}



/*
WreckAPI VMOV(eReg Rt, eReg Rt2, eFPR Sm, eFPR Sm1, eCC CC=CC_AL)   //  VFP
{
    DECL_Id(0x0C400A10);

    SET_CC;
    I |= (Rt2&15)<<16;
    I |= (Rt&15)<<12;

    //I |= (Imm8&0xFF);

    // *FIXME*

    EMIT_I;
}*/





// VADD (NEON) (Floating Point)
void VADD_NFP(eFPR Dd, eFPR Dn, eFPR Dm, bool dpr=false)
{
    DECL_Id(0xF2000D00);

    I |= (dpr)?(1<<6):(0);    // Q or do i set sz??

    I |= (Dn&15)<<16;
    I |= (Dd&15)<<12;
    I |= (Dm&15);
    EMIT_I;
}

// VSUB (NEON) (Floating Point)
void VSUB_NFP(eFPR Dd, eFPR Dn, eFPR Dm, bool dpr=false)
{
    DECL_Id(0xF2200D00);

    I |= (dpr)?(1<<6):(0);    // Q or do i set sz??

    I |= (Dn&15)<<16;
    I |= (Dd&15)<<12;
    I |= (Dm&15);
    EMIT_I;
}

// VMUL (NEON) (Floating Point)
void VMUL_NFP(eFPR Dd, eFPR Dn, eFPR Dm, bool dpr=false)
{
    DECL_Id(0xF3000D10);

    I |= (dpr)?(1<<6):(0);    // Q or do i set sz??

    I |= (Dn&15)<<16;
    I |= (Dd&15)<<12;
    I |= (Dm&15);
    EMIT_I;
}

// VDIV (VFP3) this is different from the others !
void VDIV_VFP(eFPR Dd, eFPR Dn, eFPR Dm, bool dpr=false)
{
    DECL_Id(0x0E800A00);

    //I |= (1<<8);            // sz=1
    //I |= (dpr)?(1<<8):(0);

    I |= (Dn&15)<<16;
    I |= (Dd&15)<<12;
    I |= (Dm&15);
    EMIT_I;
}










typedef void FPBinOP    (eFPR Dd, eFPR Dn, eFPR Dm, bool dpr=false);
typedef void BinaryOP   (eReg Rd, eReg Rn, eReg Rm, eCC CC=CC_AL);
typedef void UnaryOP    (eReg Rd);      // *FIXME*





// Unary OP
    WreckAPI NEG(eReg Rd)
    {
        RSB(Rd,Rd,0);
    }

    WreckAPI NOT(eReg Rd)
    {
        MVN(Rd,Rd);
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////
    //  New instruction format - begin //
    ////////////////////////////////////////////////////////////////////////////////////////////////

    extern "C" u32 rotr(u32,u32);
/*
    class ARM_Instruction
    {
    public:
*/
        typedef u32 __Imm12;      // This is typed so you'll know its really an 8b with rotate!

        s32 Check8bRot(u32 v)
        {
            if(0==(v & ~0x000000FF)) return 0x00;
            if(0==(v & ~0x8000007F)) return 0x01;
            if(0==(v & ~0xC000003F)) return 0x02;
            if(0==(v & ~0xE000001F)) return 0x03;
            if(0==(v & ~0xF000000F)) return 0x04;
            if(0==(v & ~0xF8000007)) return 0x05;
            if(0==(v & ~0xFC000003)) return 0x06;
            if(0==(v & ~0xFE000001)) return 0x07;
            if(0==(v & ~0xFF000000)) return 0x08;
            if(0==(v & ~0x7F800000)) return 0x09;
            if(0==(v & ~0x3FC00000)) return 0x0A;
            if(0==(v & ~0x1FE00000)) return 0x0B;
            if(0==(v & ~0x0FF00000)) return 0x0C;
            if(0==(v & ~0x07F80000)) return 0x0D;
            if(0==(v & ~0x03FC0000)) return 0x0E;
            return -1;
        }

        //pseudo
        void fake_inst_2546()
        {
            s32 IVal = Check8bRot(0x3f000001);
            if(-1 != IVal)
            {
                IVal = (IVal<<8) | rotr(IVal,IVal);
                printf("IVal: 0x%08X\n",IVal);
            }
            else
            {
                printf("IVal: %d\n",IVal);
            }
        }


    //};





















};

#endif //__armwreck_h


/*  Pandora donations for readme page:

    Gruso               $15 USD
    Raj Takhar          $14 USD

    Pandorasloveflaps   $20 GBP
    Kyosys              $4 ?USD

    fischju2000         $50 USD
*/
