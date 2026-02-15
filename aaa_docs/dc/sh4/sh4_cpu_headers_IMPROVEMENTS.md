# SH4 CPU Emulator Improvements for Wii
## Recommendations and Optimizations

## 1. CRITICAL PERFORMANCE IMPROVEMENTS

### 1.1 Use `const` for Read-Only Values
**Impact: High** - Helps compiler optimize register allocation
```c
// BEFORE:
u32 n = GetN(op);
u32 m = GetM(op);

// AFTER:
const u32 n = GetN(op);
const u32 m = GetM(op);
```

### 1.2 Inline Helper Functions
**Impact: High** - Eliminates function call overhead
```c
// Move to header and mark as static inline:
static inline u32 branch_target_s8(u32 op)
static inline u32 branch_target_s12(u32 op)
```

### 1.3 Reduce Variable Declarations in Hot Paths
**Impact: Medium** - Reduces stack usage
```c
// BEFORE:
sh4op(i0100_nnnn_0000_1000)
{
	u32 n = GetN(op);
	r[n] <<= 2;
}

// AFTER (when n is used once):
sh4op(i0100_nnnn_0000_1000)
{
	r[GetN(op)] <<= 2;
}
```

## 2. CODE ORGANIZATION IMPROVEMENTS

### 2.1 Group Related Instructions
- Branch instructions by type (conditional/unconditional, with/without delay)
- Logical operations by category (register-register, immediate)
- Memory operations by size (byte/word/long)

### 2.2 Add Section Headers
Makes code navigation easier - see improved files for examples.

### 2.3 Consistent Commenting Style
```c
// GOOD: Brief, informative
// mov.b <REG_M>,@<REG_N> - Store byte from Rm to memory at Rn

// BAD: Redundant or missing
//mov.b <REG_M>,@<REG_N>        
```

## 3. MEMORY OPERATION IMPROVEMENTS

### 3.1 Consolidate Duplicate Patterns
Many mov instructions have similar patterns - consider macro:

```c
#define DEFINE_MOV_STORE(suffix, type, write_func) \
sh4op(i0010_nnnn_mmmm_##suffix) \
{ \
	const u32 n = GetN(op); \
	const u32 m = GetM(op); \
	write_func(r[n], r[m]); \
}

DEFINE_MOV_STORE(0000, byte, WriteMemU8)
DEFINE_MOV_STORE(0001, word, WriteMemU16)
DEFINE_MOV_STORE(0010, long, WriteMemU32)
```

### 3.2 Fix Pre-Decrement Edge Cases
Current code has special handling for m==n case - ensure consistency:

```c
// Current pattern in multiple places:
if (m == n) {
	WriteMemBOU8(r[n], (u32)-1, r[m]);
	r[n]--;
} else {
	r[n]--;
	WriteMemU8(r[n], r[m]);
}

// Consider: Always use offset-based write for consistency
r[n] -= sizeof(type);
WriteMemU##bits(r[n], r[m]);
```

## 4. SPECIFIC FILE IMPROVEMENTS

### 4.1 sh4_cpu_branch.h
- ✅ Moved helper functions to top as static inline
- ✅ Added clear section headers
- ✅ Improved sleep instruction with clearer logic
- ✅ Added const qualifiers
- ⚠️ Consider: Branch prediction hints if supported on PPC

### 4.2 sh4_cpu_logic.h
- ✅ Reorganized by operation type
- ✅ Removed unnecessary comments
- ✅ Simplified single-use variable cases
- ⚠️ Consider: Combine with shift operations from other files

### 4.3 sh4_cpu_movs.h
- ⚠️ NEEDS: Heavy refactoring - lots of duplicate code
- ⚠️ NEEDS: Better organization (group by addressing mode)
- ⚠️ NEEDS: Macro-based generation for similar instructions

### 4.4 sh4_cpu_arith.h
- ⚠️ INCOMPLETE: Missing many arithmetic operations
- ⚠️ NEEDS: Add remaining operations from sh4_cpu.h
- ⚠️ NEEDS: Add overflow/carry flag handling

## 5. WII-SPECIFIC OPTIMIZATIONS

### 5.1 PowerPC Alignment
Ensure frequently-accessed data structures are cache-line aligned (32 bytes on Wii):
```c
// In main CPU structure:
u32 r[16] __attribute__((aligned(32)));
```

### 5.2 Branch Hints
Use GCC branch prediction hints:
```c
// For rare branches (exceptions, sleep):
if (__builtin_expect(condition, 0))

// For common branches:
if (__builtin_expect(sr.T != 0, 1))
```

### 5.3 Loop Unrolling
For operations like mac.l that iterate, consider manual unrolling:
```c
// Instead of loop, process 2-4 iterations at once
```

### 5.4 Minimize Memory Access
Cache frequently-used values in registers:
```c
// BEFORE (reads next_pc multiple times):
u32 newpc = r[n] + next_pc + 2;
pr = next_pc + 2;

// AFTER:
const u32 pc = next_pc + 2;
u32 newpc = r[n] + pc;
pr = pc;
```

## 6. DEBUGGING IMPROVEMENTS

### 6.1 Add Instruction Tracing (Debug Build Only)
```c
#ifdef SH4_DEBUG_TRACE
#define TRACE_INSTR(name) \
	if (sh4_trace_enabled) printf("PC=%08X: %s\n", pc, name)
#else
#define TRACE_INSTR(name)
#endif
```

### 6.2 Add Sanity Checks (Debug Build Only)
```c
#ifdef SH4_DEBUG
	// Check for invalid register access
	if (n >= 16) sh4_error("Invalid register index");
#endif
```

## 7. RECOMPILER PREPARATION

### 7.1 Separate Side Effects
Make it easier to identify pure operations vs. state changes:
```c
// Mark functions that only modify registers (no memory/exceptions)
#define SH4_PURE_REG_OP
```

### 7.2 Document Delay Slot Behavior
Critical for recompiler to handle correctly - already done in improved version.

## 8. MISSING FUNCTIONALITY TO ADD

Based on sh4_cpu.h declarations, these files are missing:

### From sh4_cpu_arith.h (NEEDS COMPLETION):
- addc, addv (with carry/overflow)
- subc, subv (with carry/overflow)
- neg, negc
- div0s, div0u, div1
- mul.l, muls, mulu
- dmuls.l, dmulu.l
- dt (decrement and test)
- cmp.* operations
- mac.* operations

### From sh4_cpu_logic.h (NEEDS ADDITIONS):
- shll, shlr, shal, shar (single-bit shifts)
- rotl, rotr, rotcl, rotcr
- shad, shld (dynamic shifts)
- not, swap.b, swap.w
- exts.b, exts.w, extu.b, extu.w
- xtrct
- tas.b
- tst operations

## 9. PRIORITY IMPLEMENTATION ORDER

1. **HIGH**: Add const qualifiers everywhere (easy win)
2. **HIGH**: Complete arithmetic operations (correctness)
3. **HIGH**: Refactor mov operations to reduce code duplication
4. **MEDIUM**: Add Wii-specific optimizations (branch hints, alignment)
5. **MEDIUM**: Reorganize all files with clear sections
6. **LOW**: Add debug tracing infrastructure
7. **LOW**: Prepare for recompiler transition

## 10. TESTING RECOMMENDATIONS

- Create test ROM with all SH4 instructions
- Verify delay slot behavior (especially with self-modifying code)
- Test edge cases: m==n in pre-decrement, overflow conditions
- Profile on real Wii hardware to identify hotspots
- Compare against reference SH4 emulator (e.g., Yabause)

## 11. CODE SIZE OPTIMIZATION

For Wii's limited memory:

### 11.1 Reduce Instruction Dispatcher Size
```c
// Use function pointer table instead of giant switch
typedef void (*sh4_opcode_handler)(u16 op);
extern const sh4_opcode_handler sh4_opcodes[65536];
```

### 11.2 Share Code for Similar Operations
Create common handlers with parameters:
```c
static inline void sh4_shift_left(u32 reg, u32 amount) {
	r[reg] <<= amount;
}
```

## 12. DOCUMENTATION NEEDS

- Add README explaining opcode naming convention (iXXXX_YYYY)
- Document GetN(), GetM(), GetImm*() macros
- Explain delay slot execution model
- Add performance tuning guide for Wii

## SUMMARY

**Quick Wins** (implement first):
1. Add const qualifiers (30 min)
2. Inline helper functions (15 min)
3. Add section headers (15 min)

**Medium Effort** (high value):
1. Complete missing arithmetic ops (2-3 hours)
2. Refactor mov operations (2-3 hours)
3. Add Wii-specific hints (1 hour)

**Long Term** (prepare for future):
1. Build comprehensive test suite
2. Profile and optimize hot paths
3. Prepare recompiler infrastructure
