// SH4 Arithmetic Instructions

//=============================================================================
// BASIC ARITHMETIC (Register-Register)
//=============================================================================

// add <REG_M>,<REG_N> - Add registers
sh4op(i0011_nnnn_mmmm_1100)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	r[n] += r[m];
}

// sub <REG_M>,<REG_N> - Subtract registers
sh4op(i0011_nnnn_mmmm_1000)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	r[n] -= r[m];
}

//=============================================================================
// IMMEDIATE ARITHMETIC
//=============================================================================

// add #<imm>,<REG_N> - Add sign-extended immediate to register
sh4op(i0111_nnnn_iiii_iiii)
{
	const u32 n = GetN(op);
	const s32 imm = GetSImm8(op);
	r[n] += imm;
}
