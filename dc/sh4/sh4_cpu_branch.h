// SH4 Branch Instructions
// Optimized for interpreter mode - see sh4_cpu_branch_rec.h for recompiler

#ifdef SH4_REC
#include "sh4_cpu_branch_rec.h"
#else

// Helper function for 8-bit signed branch offset calculation
// Inlined for performance
static inline u32 branch_target_s8(u32 op)
{
	return GetSImm8(op) * 2 + next_pc + 2;
}

// Helper function for 12-bit signed branch offset calculation
static inline u32 branch_target_s12(u32 op)
{
	return GetSImm12(op) * 2 + next_pc + 2;
}

//=============================================================================
// UNCONDITIONAL BRANCHES WITH DELAY SLOT
//=============================================================================

// braf <REG_N> - Branch Far to PC + Rn + 4
sh4op(i0000_nnnn_0010_0011)
{
	const u32 n = GetN(op);
	const u32 newpc = r[n] + next_pc + 2;
	ExecuteDelayslot();  // WARNING: r[n] may change during delay slot
	next_pc = newpc;
}

// bsrf <REG_N> - Branch to Subroutine Far (PC + Rn + 4)
sh4op(i0000_nnnn_0000_0011)
{
	const u32 n = GetN(op);
	const u32 newpc = r[n] + next_pc + 2;
	pr = next_pc + 2;  // Save return address (after delay slot)
	ExecuteDelayslot();  // WARNING: pr and r[n] may change during delay slot
	next_pc = newpc;
}

// rte - Return from Exception
sh4op(i0000_0000_0010_1011)
{
	const u32 newpc = spc;
	ExecuteDelayslot_RTE();
	next_pc = newpc;
	
	if (UpdateSR())
	{
		// Update interrupt controller if interrupts were enabled
		UpdateINTC();
	}
}

// rts - Return from Subroutine
sh4op(i0000_0000_0000_1011)
{
	const u32 newpc = pr;
	ExecuteDelayslot();  // WARNING: pr may change during delay slot
	next_pc = newpc;
}

// jmp @<REG_N> - Jump to address in register
sh4op(i0100_nnnn_0010_1011)
{
	const u32 n = GetN(op);
	const u32 newpc = r[n];
	ExecuteDelayslot();  // WARNING: r[n] may change during delay slot
	next_pc = newpc;
}

// jsr @<REG_N> - Jump to Subroutine at address in register
sh4op(i0100_nnnn_0000_1011)
{
	const u32 n = GetN(op);
	const u32 newpc = r[n];
	pr = next_pc + 2;  // Save return address (after delay slot)
	ExecuteDelayslot();  // WARNING: r[n] and pr may change during delay slot
	next_pc = newpc;
}

// bra <bdisp12> - Branch Always
sh4op(i1010_iiii_iiii_iiii)
{
	const u32 newpc = branch_target_s12(op);
	ExecuteDelayslot();
	next_pc = newpc;
}

// bsr <bdisp12> - Branch to Subroutine
sh4op(i1011_iiii_iiii_iiii)
{
	pr = next_pc + 2;  // Save return address (after delay slot)
	const u32 newpc = branch_target_s12(op);
	ExecuteDelayslot();
	next_pc = newpc;
}

//=============================================================================
// CONDITIONAL BRANCHES (NO DELAY SLOT)
//=============================================================================

// bf <bdisp8> - Branch if False (T == 0)
sh4op(i1000_1011_iiii_iiii)
{
	if (sr.T == 0)
	{
		next_pc = branch_target_s8(op);
	}
}

// bt <bdisp8> - Branch if True (T == 1)
sh4op(i1000_1001_iiii_iiii)
{
	if (sr.T != 0)
	{
		next_pc = branch_target_s8(op);
	}
}

//=============================================================================
// CONDITIONAL BRANCHES WITH DELAY SLOT
//=============================================================================

// bf.s <bdisp8> - Branch if False with delay slot
sh4op(i1000_1111_iiii_iiii)
{
	if (sr.T == 0)
	{
		const u32 newpc = branch_target_s8(op);
		ExecuteDelayslot();
		next_pc = newpc;
	}
}

// bt.s <bdisp8> - Branch if True with delay slot
sh4op(i1000_1101_iiii_iiii)
{
	if (sr.T != 0)
	{
		const u32 newpc = branch_target_s8(op);
		ExecuteDelayslot();
		next_pc = newpc;
	}
}

//=============================================================================
// EXCEPTION/TRAP
//=============================================================================

// trapa #<imm> - Trap Always
sh4op(i1100_0011_iiii_iiii)
{
	CCN_TRA = GetImm8(op) << 2;
	Do_Exeption(next_pc, 0x160, 0x100);
}

//=============================================================================
// POWER MANAGEMENT
//=============================================================================

// sleep - Enter sleep mode
sh4op(i0000_0000_0001_1011)
{
	sh4_sleeping = true;
	
	// Wait for interrupt - timeout after 1000 cycles to prevent hang
	int cycles = 0;
	bool interrupted = false;
	
	while (cycles < 1000)
	{
		if (UpdateSystem())
		{
			interrupted = true;
			break;
		}
		cycles++;
	}
	
	// If not interrupted, re-execute sleep instruction
	if (!interrupted)
	{
		next_pc -= 2;
	}
	
	sh4_sleeping = false;
}

#endif // SH4_REC
