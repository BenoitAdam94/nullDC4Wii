// SH4 Move Instructions - Fixed to work with existing memory macros

//=============================================================================
// CONTROL REGISTER MOVES (stc/sts family)
//=============================================================================

// stc GBR,<REG_N>
sh4op(i0000_nnnn_0001_0010)
{
	const u32 n = GetN(op);
	r[n] = gbr;
}

// stc VBR,<REG_N>
sh4op(i0000_nnnn_0010_0010)
{
	const u32 n = GetN(op);
	r[n] = vbr;
}

// stc SSR,<REG_N>
sh4op(i0000_nnnn_0011_0010)
{
	const u32 n = GetN(op);
	r[n] = ssr;
}

// stc SGR,<REG_N>
sh4op(i0000_nnnn_0011_1010)
{
	const u32 n = GetN(op);
	r[n] = sgr;
}

// stc SPC,<REG_N>
sh4op(i0000_nnnn_0100_0010)
{
	const u32 n = GetN(op);
	r[n] = spc;
}

// stc DBR,<REG_N>
sh4op(i0000_nnnn_1111_1010)
{
	const u32 n = GetN(op);
	r[n] = dbr;
}

// sts FPUL,<REG_N>
sh4op(i0000_nnnn_0101_1010)
{
	const u32 n = GetN(op);
	r[n] = fpul;
}

// sts MACH,<REG_N>
sh4op(i0000_nnnn_0000_1010)
{
	const u32 n = GetN(op);
	r[n] = mach;
}

// sts MACL,<REG_N>
sh4op(i0000_nnnn_0001_1010)
{
	const u32 n = GetN(op);
	r[n] = macl;
}

// sts PR,<REG_N>
sh4op(i0000_nnnn_0010_1010)
{
	const u32 n = GetN(op);
	r[n] = pr;
}

// stc RM_BANK,<REG_N>
sh4op(i0000_nnnn_1mmm_0010)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op) & 0x7;
	r[n] = r_bank[m];
}

//=============================================================================
// INDEXED ADDRESSING (R0 + Rm)
//=============================================================================

// mov.b @(R0,<REG_M>),<REG_N>
sh4op(i0000_nnnn_mmmm_1100)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	ReadMemBOS8(r[n], r[0], r[m]);
}

// mov.w @(R0,<REG_M>),<REG_N>
sh4op(i0000_nnnn_mmmm_1101)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	ReadMemBOS16(r[n], r[0], r[m]);
}

// mov.l @(R0,<REG_M>),<REG_N>
sh4op(i0000_nnnn_mmmm_1110)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	ReadMemBOU32(r[n], r[0], r[m]);
}

// mov.b <REG_M>,@(R0,<REG_N>)
sh4op(i0000_nnnn_mmmm_0100)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	WriteMemBOU8(r[0], r[n], r[m]);
}

// mov.w <REG_M>,@(R0,<REG_N>)
sh4op(i0000_nnnn_mmmm_0101)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	WriteMemBOU16(r[0], r[n], r[m]);
}

// mov.l <REG_M>,@(R0,<REG_N>)
sh4op(i0000_nnnn_mmmm_0110)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	WriteMemBOU32(r[0], r[n], r[m]);
}

//=============================================================================
// DISPLACEMENT ADDRESSING
//=============================================================================

// mov.l <REG_M>,@(<disp>,<REG_N>)
sh4op(i0001_nnnn_mmmm_iiii)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	const u32 disp = GetImm4(op) << 2;
	WriteMemBOU32(r[n], disp, r[m]);
}

// mov.l @(<disp>,<REG_M>),<REG_N>
sh4op(i0101_nnnn_mmmm_iiii)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	const u32 disp = GetImm4(op) << 2;
	ReadMemBOU32(r[n], r[m], disp);
}

//=============================================================================
// REGISTER INDIRECT (Simple)
//=============================================================================

// mov.b <REG_M>,@<REG_N>
sh4op(i0010_nnnn_mmmm_0000)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	WriteMemU8(r[n], r[m]);
}

// mov.w <REG_M>,@<REG_N>
sh4op(i0010_nnnn_mmmm_0001)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	WriteMemU16(r[n], r[m]);
}

// mov.l <REG_M>,@<REG_N>
sh4op(i0010_nnnn_mmmm_0010)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	WriteMemU32(r[n], r[m]);
}

// mov.b @<REG_M>,<REG_N>
sh4op(i0110_nnnn_mmmm_0000)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	ReadMemS8(r[n], r[m]);
}

// mov.w @<REG_M>,<REG_N>
sh4op(i0110_nnnn_mmmm_0001)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	ReadMemS16(r[n], r[m]);
}

// mov.l @<REG_M>,<REG_N>
sh4op(i0110_nnnn_mmmm_0010)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	ReadMemU32(r[n], r[m]);
}

// mov <REG_M>,<REG_N>
sh4op(i0110_nnnn_mmmm_0011)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	r[n] = r[m];
}

//=============================================================================
// PRE-DECREMENT STORE
//=============================================================================

// mov.b <REG_M>,@-<REG_N>
sh4op(i0010_nnnn_mmmm_0100)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	
	if (n == m)
	{
		WriteMemBOU8(r[n], (u32)-1, r[m]);
		r[n]--;
	}
	else
	{
		r[n]--;
		WriteMemU8(r[n], r[m]);
	}
}

// mov.w <REG_M>,@-<REG_N>
sh4op(i0010_nnnn_mmmm_0101)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	
	if (n == m)
	{
		WriteMemBOU16(r[n], (u32)-2, r[m]);
		r[n] -= 2;
	}
	else
	{
		r[n] -= 2;
		WriteMemU16(r[n], r[m]);
	}
}

// mov.l <REG_M>,@-<REG_N>
sh4op(i0010_nnnn_mmmm_0110)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	
	if (m == n)
	{
		WriteMemBOU32(r[n], (u32)-4, r[m]);
		r[n] -= 4;
	}
	else
	{
		r[n] -= 4;
		WriteMemU32(r[n], r[m]);
	}
}

//=============================================================================
// POST-INCREMENT LOAD
//=============================================================================

// mov.b @<REG_M>+,<REG_N>
sh4op(i0110_nnnn_mmmm_0100)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	ReadMemS8(r[n], r[m]);
	if (n != m)
		r[m] += 1;
}

// mov.w @<REG_M>+,<REG_N>
sh4op(i0110_nnnn_mmmm_0101)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	ReadMemS16(r[n], r[m]);
	if (n != m)
		r[m] += 2;
}

// mov.l @<REG_M>+,<REG_N>
sh4op(i0110_nnnn_mmmm_0110)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	ReadMemU32(r[n], r[m]);
	if (n != m)
		r[m] += 4;
}

//=============================================================================
// GBR-BASED ADDRESSING
//=============================================================================

// mov.b R0,@(<disp>,GBR)
sh4op(i1100_0000_iiii_iiii)
{
	const u32 disp = GetImm8(op);
	WriteMemBOU8(gbr, disp, r[0]);
}

// mov.w R0,@(<disp>,GBR)
sh4op(i1100_0001_iiii_iiii)
{
	const u32 disp = GetImm8(op) << 1;
	WriteMemBOU16(gbr, disp, r[0]);
}

// mov.l R0,@(<disp>,GBR)
sh4op(i1100_0010_iiii_iiii)
{
	const u32 disp = GetImm8(op) << 2;
	WriteMemBOU32(gbr, disp, r[0]);
}

// mov.b @(<disp>,GBR),R0
sh4op(i1100_0100_iiii_iiii)
{
	const u32 disp = GetImm8(op);
	ReadMemBOS8(r[0], gbr, disp);
}

// mov.w @(<disp>,GBR),R0
sh4op(i1100_0101_iiii_iiii)
{
	const u32 disp = GetImm8(op) << 1;
	ReadMemBOS16(r[0], gbr, disp);
}

// mov.l @(<disp>,GBR),R0
sh4op(i1100_0110_iiii_iiii)
{
	const u32 disp = GetImm8(op) << 2;
	ReadMemBOU32(r[0], gbr, disp);
}

//=============================================================================
// PC-RELATIVE LOADS
//=============================================================================

// mov.w @(<disp>,PC),<REG_N>
sh4op(i1001_nnnn_iiii_iiii)
{
	const u32 n = GetN(op);
	const u32 disp = GetImm8(op) << 1;
	ReadMemS16(r[n], next_pc + 2 + disp);
}

// mov.l @(<disp>,PC),<REG_N>
sh4op(i1101_nnnn_iiii_iiii)
{
	const u32 n = GetN(op);
	const u32 disp = GetImm8(op) << 2;
	const u32 addr = ((next_pc + 2) & 0xFFFFFFFC) + disp;
	ReadMemU32(r[n], addr);
}

//=============================================================================
// IMMEDIATE MOVES
//=============================================================================

// mov #<imm>,<REG_N>
sh4op(i1110_nnnn_iiii_iiii)
{
	const u32 n = GetN(op);
	r[n] = (u32)(s32)(s8)GetImm8(op);
}

// mova @(<disp>,PC),R0
sh4op(i1100_0111_iiii_iiii)
{
	const u32 disp = GetImm8(op) << 2;
	r[0] = ((next_pc + 2) & 0xFFFFFFFC) + disp;
}

//=============================================================================
// SYSTEM REGISTER LOADS (ldc/lds family)
//=============================================================================

// lds <REG_N>,MACH
sh4op(i0100_nnnn_0000_1010)
{
	const u32 n = GetN(op);
	mach = r[n];
}

// lds <REG_N>,MACL
sh4op(i0100_nnnn_0001_1010)
{
	const u32 n = GetN(op);
	macl = r[n];
}

// lds <REG_N>,PR
sh4op(i0100_nnnn_0010_1010)
{
	const u32 n = GetN(op);
	pr = r[n];
}

// lds <REG_N>,FPUL
sh4op(i0100_nnnn_0101_1010)
{
	const u32 n = GetN(op);
	fpul = r[n];
}

// ldc <REG_N>,DBR
sh4op(i0100_nnnn_1111_1010)
{
	const u32 n = GetN(op);
	dbr = r[n];
}

// ldc <REG_N>,GBR
sh4op(i0100_nnnn_0001_1110)
{
	const u32 n = GetN(op);
	gbr = r[n];
}

// ldc <REG_N>,VBR
sh4op(i0100_nnnn_0010_1110)
{
	const u32 n = GetN(op);
	vbr = r[n];
}

// ldc <REG_N>,SSR
sh4op(i0100_nnnn_0011_1110)
{
	const u32 n = GetN(op);
	ssr = r[n];
}

// ldc <REG_N>,SGR
sh4op(i0100_nnnn_0011_1010)
{
	const u32 n = GetN(op);
	sgr = r[n];
}

// ldc <REG_N>,SPC
sh4op(i0100_nnnn_0100_1110)
{
	const u32 n = GetN(op);
	spc = r[n];
}

// ldc <REG_N>,RM_BANK
sh4op(i0100_nnnn_1mmm_1110)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op) & 0x7;
	r_bank[m] = r[n];
}

//=============================================================================
// STACK OPERATIONS (System Register Push)
//=============================================================================

// sts.l FPUL,@-<REG_N>
sh4op(i0100_nnnn_0101_0010)
{
	const u32 n = GetN(op);
	r[n] -= 4;
	WriteMemU32(r[n], fpul);
}

// sts.l MACH,@-<REG_N>
sh4op(i0100_nnnn_0000_0010)
{
	const u32 n = GetN(op);
	r[n] -= 4;
	WriteMemU32(r[n], mach);
}

// sts.l MACL,@-<REG_N>
sh4op(i0100_nnnn_0001_0010)
{
	const u32 n = GetN(op);
	r[n] -= 4;
	WriteMemU32(r[n], macl);
}

// sts.l PR,@-<REG_N>
sh4op(i0100_nnnn_0010_0010)
{
	const u32 n = GetN(op);
	r[n] -= 4;
	WriteMemU32(r[n], pr);
}

// sts.l DBR,@-<REG_N>
sh4op(i0100_nnnn_1111_0010)
{
	const u32 n = GetN(op);
	r[n] -= 4;
	WriteMemU32(r[n], dbr);
}

// stc.l GBR,@-<REG_N>
sh4op(i0100_nnnn_0001_0011)
{
	const u32 n = GetN(op);
	r[n] -= 4;
	WriteMemU32(r[n], gbr);
}

// stc.l VBR,@-<REG_N>
sh4op(i0100_nnnn_0010_0011)
{
	const u32 n = GetN(op);
	r[n] -= 4;
	WriteMemU32(r[n], vbr);
}

// stc.l SSR,@-<REG_N>
sh4op(i0100_nnnn_0011_0011)
{
	const u32 n = GetN(op);
	r[n] -= 4;
	WriteMemU32(r[n], ssr);
}

// stc.l SGR,@-<REG_N>
sh4op(i0100_nnnn_0011_0010)
{
	const u32 n = GetN(op);
	r[n] -= 4;
	WriteMemU32(r[n], sgr);
}

// stc.l SPC,@-<REG_N>
sh4op(i0100_nnnn_0100_0011)
{
	const u32 n = GetN(op);
	r[n] -= 4;
	WriteMemU32(r[n], spc);
}

// stc.l RM_BANK,@-<REG_N>
sh4op(i0100_nnnn_1mmm_0011)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op) & 0x7;
	r[n] -= 4;
	WriteMemU32(r[n], r_bank[m]);
}

//=============================================================================
// STACK OPERATIONS (System Register Pop)
//=============================================================================

// lds.l @<REG_N>+,MACH
sh4op(i0100_nnnn_0000_0110)
{
	const u32 n = GetN(op);
	ReadMemU32(mach, r[n]);
	r[n] += 4;
}

// lds.l @<REG_N>+,MACL
sh4op(i0100_nnnn_0001_0110)
{
	const u32 n = GetN(op);
	ReadMemU32(macl, r[n]);
	r[n] += 4;
}

// lds.l @<REG_N>+,PR
sh4op(i0100_nnnn_0010_0110)
{
	const u32 n = GetN(op);
	ReadMemU32(pr, r[n]);
	r[n] += 4;
}

// lds.l @<REG_N>+,FPUL
sh4op(i0100_nnnn_0101_0110)
{
	const u32 n = GetN(op);
	ReadMemU32(fpul, r[n]);
	r[n] += 4;
}

// lds.l @<REG_N>+,DBR
sh4op(i0100_nnnn_1111_0110)
{
	const u32 n = GetN(op);
	ReadMemU32(dbr, r[n]);
	r[n] += 4;
}

// ldc.l @<REG_N>+,GBR
sh4op(i0100_nnnn_0001_0111)
{
	const u32 n = GetN(op);
	ReadMemU32(gbr, r[n]);
	r[n] += 4;
}

// ldc.l @<REG_N>+,VBR
sh4op(i0100_nnnn_0010_0111)
{
	const u32 n = GetN(op);
	ReadMemU32(vbr, r[n]);
	r[n] += 4;
}

// ldc.l @<REG_N>+,SSR
sh4op(i0100_nnnn_0011_0111)
{
	const u32 n = GetN(op);
	ReadMemU32(ssr, r[n]);
	r[n] += 4;
}

// ldc.l @<REG_N>+,SGR
sh4op(i0100_nnnn_0011_0110)
{
	const u32 n = GetN(op);
	ReadMemU32(sgr, r[n]);
	r[n] += 4;
}

// ldc.l @<REG_N>+,SPC
sh4op(i0100_nnnn_0100_0111)
{
	const u32 n = GetN(op);
	ReadMemU32(spc, r[n]);
	r[n] += 4;
}

// ldc.l @<REG_N>+,RM_BANK
sh4op(i0100_nnnn_1mmm_0111)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op) & 0x7;
	ReadMemU32(r_bank[m], r[n]);
	r[n] += 4;
}

//=============================================================================
// DISPLACEMENT ADDRESSING WITH R0
//=============================================================================

// mov.b R0,@(<disp>,<REG_M>)
sh4op(i1000_0000_mmmm_iiii)
{
	const u32 m = GetM(op);
	const u32 disp = GetImm4(op);
	WriteMemBOU8(r[m], disp, r[0]);
}

// mov.w R0,@(<disp>,<REG_M>)
sh4op(i1000_0001_mmmm_iiii)
{
	const u32 m = GetM(op);
	const u32 disp = GetImm4(op) << 1;
	WriteMemBOU16(r[m], disp, r[0]);
}

// mov.b @(<disp>,<REG_M>),R0
sh4op(i1000_0100_mmmm_iiii)
{
	const u32 m = GetM(op);
	const u32 disp = GetImm4(op);
	ReadMemBOS8(r[0], r[m], disp);
}

// mov.w @(<disp>,<REG_M>),R0
sh4op(i1000_0101_mmmm_iiii)
{
	const u32 m = GetM(op);
	const u32 disp = GetImm4(op) << 1;
	ReadMemBOS16(r[0], r[m], disp);
}

//=============================================================================
// SPECIAL INSTRUCTIONS
//=============================================================================

// movca.l R0,@<REG_N>
sh4op(i0000_nnnn_1100_0011)
{
	const u32 n = GetN(op);
	WriteMemU32(r[n], r[0]);
}

// clrmac
sh4op(i0000_0000_0010_1000)
{
	macl = 0;
	mach = 0;
}
