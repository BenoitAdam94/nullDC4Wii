@@

.arm
.align 8

.equ SH4_TIMESLICE, 448
.equ CPU_RATIO, 5



@@@@@@@@@@ some helpers @@@@@@@@@@

.global getSP
getSP:
    mov r0,sp
    bx lr

.global rotr
rotr:
    ror r0,r0,r1
    bx lr

@.global rotl
@rotl:
@    rol r0,r0,r1
@    bx lr


@@@@@@@@@@ some helpers @@@@@@@@@@





@@@@@@@@@@ ngen_LinkBlock_*****_stub @@@@@@@@@@

.global ngen_LinkBlock_Static_stub
ngen_LinkBlock_Static_stub:

    mov r1,lr
    sub r1,#12                  @go before the mov32
    bl ngen_LinkBlock_Static
    bx r0
    nop


.global ngen_LinkBlock_cond_EQ_stub
ngen_LinkBlock_cond_EQ_stub:

    mov r1,lr
    sub r1,#12                  @go before the mov32
    mov r2,#0
    bl ngen_LinkBlock_condX
    bx r0
    nop
    
.global ngen_LinkBlock_cond_AL_stub    
ngen_LinkBlock_cond_AL_stub:

    mov r1,lr
    sub r1,#12                  @go before the mov32
    mov r2,#1
    bl ngen_LinkBlock_condX
    bx r0
    nop

@@@@@@@@@@ ngen_LinkBlock_*****_stub @@@@@@@@@@

@@@@@@@@@@ void ngen_LinkBlock_Dynamic_1st_stub() @@@@@@@@@@
.global ngen_LinkBlock_Dynamic_1st_stub
ngen_LinkBlock_Dynamic_1st_stub:

	mov r1,lr
    sub r1,#16                  @go before the loadimmbase
    bl ngen_LinkBlock_Dynamic_1st
    bx r0
    nop

@@@@@@@@@@ void ngen_LinkBlock_Dynamic_2nd_stub() @@@@@@@@@@
.global ngen_LinkBlock_Dynamic_2nd_stub
ngen_LinkBlock_Dynamic_2nd_stub:
	
	mov r1,lr
    sub r1,#40                  @go before the loadimmbase
    bl ngen_LinkBlock_Dynamic_2nd
    bx r0
    nop
    
@@@@@@@@@@ ngen_FailedToFindBlock_ @@@@@@@@@@


.global ngen_FailedToFindBlock_
ngen_FailedToFindBlock_:

    bl rdv_FailedToFindBlock
    bx r0

@@@@@@@@@@ ngen_FailedToFindBlock_ @@@@@@@@@@







@@@@@@@@@@ ngen_mainloop @@@@@@@@@@

@ you can load the address of the sh4 reg struct on the mainloop init
@ using (u8*)regptr-(u8*)Sh4cntx
@ all registers are < 1024 bytes from that
@ so you can use reg+imm forms for it



.global ngen_mainloop
ngen_mainloop:

nop
push { r3-r9,lr }


ldr r1, =_next_pc   @ _next_pc is u32* -> Sh4cntx.pc (next_pc)
ldr r1, [r1]        @ now we have the actual address to next_pc
ldr r0, [r1]        @ now we (hopefully) have the value of next_pc

mov r9, #SH4_TIMESLICE


    ldr r8, =Sh4cntx    @ wheeeee


.global no_update
no_update:              @ next_pc _MUST_ be on r0


    bl bm_GetCode
    bx r0

.global do_update_write
do_update_write:        @ next_pc _MUST_ be on ram


    ldr r1, =_next_pc   @ _next_pc is u32* -> Sh4cntx.pc (next_pc)
    ldr r1, [r1]        @ now we have the actual address to next_pc
    str r0, [r1]


.global do_update
do_update:


    add r9,r9,#SH4_TIMESLICE
    mov r0,sp
    bl UpdateSystem

    ldr r1, =sh4_int_bCpuRun
    ldr r1, [r1]

    mov r2, #0
    cmp r1, r2

    ldr r1, =_next_pc   @ _next_pc is u32* -> Sh4cntx.pc (next_pc)
    ldr r1, [r1]        @ now we have the actual address to next_pc
    ldr r0, [r1]        @ now we (hopefully) have the value of next_pc


    bne no_update


pop {r3-r9,lr}
bx lr

end_ngen_mainloop:

@@@@@@@@@@ ngen_mainloop @@@@@@@@@@










        @=======================================================@
            @ Start of FPU Instructions -ARM VFP/NEON Impl. @
        @=======================================================@

@Note: emulating single precision only! no check, just FAIL!

@Fixme: use some damn macros for GetN/M and loading base of fpu regs!

@#define GetN(str) ((str>>8) & 0xf)
@#define GetM(str) ((str>>4) & 0xf)
@#define GetImm4(str) ((str>>0) & 0xf)
@#define GetImm8(str) ((str>>0) & 0xff)
@#define GetSImm8(str) ((s8)((str>>0) & 0xff))
@#define GetImm12(str) ((str>>0) & 0xfff)
@#define GetSImm12(str) (((s16)((GetImm12(str))<<4))>>4)

@		u32 n = GetN(op);
@		u32 m = GetM(op);
@		fr[n] += fr[m];





.global aa1111_nnnn_mmmm_0000
aa1111_nnnn_mmmm_0000:

ldr r1,=_fr         @ r0=&Sh4cntx.fr
ldr r1,[r1]

ror r2,r0,#8        @ GetN
and r2,r2,#15
lsl r2,r2,#2
add r2,r1,r2

ror r3,r0,#4        @ GetM
and r3,r3,#15
lsl r3,r4,#2
add r3,r1,r3

vldr d0,[r2]        @ d0=fr[n]
vldr d2,[r3]        @ d2=fr[m]
vadd.f32 d0,d0,d2   @ d0*=d2
vstr d0,[r2]        @ fr[n]=d0


bx lr

@@@@@@
@matrix mul
.global ftrv_asm
ftrv_asm:

@r0=dst,r1=vec,r2=mtx

vldm r2,{d8,d9,d10,d11,d12,d13,d14,d15}

@ =not working for some reason=
@ vld4.32 {d0[],d2[],d4[],d6[]},r1@128
@ vld4.32 {d1[],d3[],d5[],d7[]},r1@128
vldr d0,[r1,#+0]
vldr d2,[r1,#+4]
vldr d4,[r1,#+8]
vldr d6,[r1,#+12]

vdup.32 q0,d0[0]
vdup.32 q1,d2[0]
vdup.32 q2,d4[0]
vdup.32 q3,d6[0]

@ vstm r0,{d0,d1,d2,d3,d4,d5,d6,d7} @{d8,d9,d10,d11,d12,d13,d14,d15} @
@ bx lr

VMUL.F32 Q0,Q0,Q4
VMLA.F32 Q0,Q1,Q5
VMLA.F32 Q0,Q2,Q6
VMLA.F32 Q0,Q3,Q7

vstm r0,{d0,d1}

bx lr

.global fipr_asm
fipr_asm:

@ vdot
@		idp=fr[n+0]*fr[m+0];
@		idp+=fr[n+1]*fr[m+1];
@		idp+=fr[n+2]*fr[m+2];
@		idp+=fr[n+3]*fr[m+3];

vldr s0,[r0,#+0]
vldr s2,[r0,#+4]
vldr s4,[r0,#+8]
vldr s6,[r0,#+12]

vldr s8,[r1,#+0]
vldr s10,[r1,#+4]
vldr s12,[r1,#+8]
vldr s14,[r1,#+12]

vmul.f32 d0,d0,d4
vmla.f32 d0,d1,d5
vmla.f32 d0,d2,d6
vmla.f32 d0,d3,d7

@store to ret ..
vmov r0,s0
bx lr

