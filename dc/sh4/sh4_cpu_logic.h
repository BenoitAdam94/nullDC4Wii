// SH4 Logical and Shift Instructions

//=============================================================================
// BITWISE LOGICAL OPERATIONS (Register-Register)
//=============================================================================

// and <REG_M>,<REG_N> - Bitwise AND
sh4op(i0010_nnnn_mmmm_1001)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	r[n] &= r[m];
}

// or <REG_M>,<REG_N> - Bitwise OR
sh4op(i0010_nnnn_mmmm_1011)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);	
	r[n] |= r[m];
}

// xor <REG_M>,<REG_N> - Bitwise XOR
sh4op(i0010_nnnn_mmmm_1010)
{
	const u32 n = GetN(op);
	const u32 m = GetM(op);
	r[n] ^= r[m];
}

//=============================================================================
// BITWISE LOGICAL OPERATIONS (Immediate with R0)
//=============================================================================

// and #<imm>,R0 - Bitwise AND immediate with R0
sh4op(i1100_1001_iiii_iiii)
{
	r[0] &= GetImm8(op);
}

// or #<imm>,R0 - Bitwise OR immediate with R0
sh4op(i1100_1011_iiii_iiii)
{
	r[0] |= GetImm8(op);
}

// xor #<imm>,R0 - Bitwise XOR immediate with R0
sh4op(i1100_1010_iiii_iiii)
{
	r[0] ^= GetImm8(op);
}

//=============================================================================
// MULTI-BIT SHIFT LEFT (Logical)
//=============================================================================

// shll2 <REG_N> - Shift Left Logical 2 bits
sh4op(i0100_nnnn_0000_1000)
{
	r[GetN(op)] <<= 2;
}

// shll8 <REG_N> - Shift Left Logical 8 bits
sh4op(i0100_nnnn_0001_1000)
{
	r[GetN(op)] <<= 8;
}

// shll16 <REG_N> - Shift Left Logical 16 bits
sh4op(i0100_nnnn_0010_1000)
{
	r[GetN(op)] <<= 16;
}

//=============================================================================
// MULTI-BIT SHIFT RIGHT (Logical)
//=============================================================================

// shlr2 <REG_N> - Shift Right Logical 2 bits
sh4op(i0100_nnnn_0000_1001)
{
	r[GetN(op)] >>= 2;
}

// shlr8 <REG_N> - Shift Right Logical 8 bits
sh4op(i0100_nnnn_0001_1001)
{
	r[GetN(op)] >>= 8;
}

// shlr16 <REG_N> - Shift Right Logical 16 bits
sh4op(i0100_nnnn_0010_1001)
{
	r[GetN(op)] >>= 16;
}

//=============================================================================
// SYSTEM/MISC
//=============================================================================

// nop - No Operation
sh4op(i0000_0000_0000_1001)
{
	// Intentionally empty - no operation performed
}
