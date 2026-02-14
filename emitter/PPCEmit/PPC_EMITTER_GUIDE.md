# PowerPC Emitter Documentation

## Overview

The PowerPC emitter is a code generation library for dynamically creating PowerPC machine code at runtime. This is used in the Dreamcast emulator to perform dynamic recompilation (JIT compilation) of SH4 instructions to native PowerPC code for the Wii.

## File Structure

- **ppc_emit.h** - Auto-generated low-level instruction encoding functions
- **ppc_emitter.h** - High-level helper macros and convenience functions
- **ppc64_info.txt** - PowerPC instruction format reference
- **list.xml** - Instruction list metadata

## Basic Usage

### 1. Include the Emitter

```c
#include "ppc_emitter.h"
```

### 2. Implement the ppc_emit Function

You must provide this function to handle the generated code:

```c
void ppc_emit(u32 insn) {
    // Write the instruction to your code buffer
    *(u32*)code_ptr = insn;
    code_ptr += 4;
}
```

### 3. Implement emit_GetCCPtr

For label support, implement:

```c
void* emit_GetCCPtr() {
    return code_ptr; // Return current code pointer
}
```

## Register Conventions

### General Purpose Registers (GPRs)

PowerPC has 32 general-purpose registers (r0-r31):

- **r0** - Special cases (reads as 0 in some contexts)
- **r1 (sp)** - Stack pointer
- **r2** - TOC pointer (Table of Contents) / Reserved
- **r3-r10** - Function arguments and return values
  - r3 = first arg / first return value
  - r4 = second arg / second return value
  - r5-r10 = additional arguments
- **r11-r12** - Volatile (scratch registers)
- **r13** - Small data area pointer / Reserved
- **r14-r31** - Non-volatile (callee-saved)

### Floating Point Registers (FPRs)

PowerPC has 32 floating-point registers (f0-f31):

- **f0** - Volatile
- **f1-f13** - Function arguments and return values
- **f14-f31** - Non-volatile (callee-saved in most ABIs)

### Special Purpose Registers

- **LR (Link Register)** - Return address
- **CTR (Count Register)** - Loop counter / branch target
- **CR (Condition Register)** - 8 x 4-bit condition fields (CR0-CR7)
- **XER** - Fixed-point exception register

## Common Operations

### Loading Immediate Values

```c
// Load 16-bit immediate
ppc_li(ppc_r3, 42);          // r3 = 42

// Load 32-bit immediate
ppc_li32(ppc_r4, 0x12345678); // r4 = 0x12345678

// Load upper 16 bits
ppc_lis(ppc_r5, 0x1234);     // r5 = 0x12340000
```

### Register Moves

```c
// Move register to register
ppc_mr(ppc_r3, ppc_r4);      // r3 = r4
ppc_mov(ppc_r3, ppc_r4);     // Alternative syntax
ppc_copy(ppc_r3, ppc_r4);    // Alternative syntax
```

### Arithmetic Operations

```c
// Addition
ppc_add(ppc_r3, ppc_r4, ppc_r5);     // r3 = r4 + r5
ppc_addi(ppc_r3, ppc_r4, 100);       // r3 = r4 + 100

// Subtraction
ppc_sub(ppc_r3, ppc_r4, ppc_r5);     // r3 = r4 - r5
ppc_subfic(ppc_r3, ppc_r4, 100);     // r3 = 100 - r4

// Multiplication
ppc_mullw(ppc_r3, ppc_r4, ppc_r5);   // r3 = r4 * r5 (low word)
ppc_mulhwu(ppc_r3, ppc_r4, ppc_r5);  // r3 = (r4 * r5) >> 32 (unsigned)

// Division
ppc_divw(ppc_r3, ppc_r4, ppc_r5);    // r3 = r4 / r5 (signed)
ppc_divwu(ppc_r3, ppc_r4, ppc_r5);   // r3 = r4 / r5 (unsigned)

// Negation
ppc_neg(ppc_r3, ppc_r4);             // r3 = -r4
```

### Logical Operations

```c
// AND
ppc_and(ppc_r3, ppc_r4, ppc_r5);     // r3 = r4 & r5
ppc_andi(ppc_r3, ppc_r4, 0xFF);      // r3 = r4 & 0xFF

// OR
ppc_or(ppc_r3, ppc_r4, ppc_r5);      // r3 = r4 | r5
ppc_ori(ppc_r3, ppc_r4, 0xFF);       // r3 = r4 | 0xFF

// XOR
ppc_xor(ppc_r3, ppc_r4, ppc_r5);     // r3 = r4 ^ r5
ppc_xori(ppc_r3, ppc_r4, 0xFF);      // r3 = r4 ^ 0xFF

// NOT
ppc_not(ppc_r3, ppc_r4);             // r3 = ~r4

// NOR
ppc_nor(ppc_r3, ppc_r4, ppc_r5);     // r3 = ~(r4 | r5)
```

### Shift and Rotate Operations

```c
// Shift left
ppc_slw(ppc_r3, ppc_r4, ppc_r5);     // r3 = r4 << r5

// Shift right (logical)
ppc_srw(ppc_r3, ppc_r4, ppc_r5);     // r3 = r4 >> r5 (unsigned)

// Shift right (arithmetic - sign extend)
ppc_sraw(ppc_r3, ppc_r4, ppc_r5);    // r3 = r4 >> r5 (signed)
ppc_srawi(ppc_r3, ppc_r4, 8);        // r3 = r4 >> 8 (signed, immediate)

// Rotate left
ppc_rotlwi(ppc_r3, ppc_r4, 8);       // r3 = r4 rotated left 8 bits

// Rotate right
ppc_rotrwi(ppc_r3, ppc_r4, 8);       // r3 = r4 rotated right 8 bits
```

### Comparisons

```c
// Compare signed (sets CR0)
ppc_cmpw(ppc_cr0, ppc_r3, ppc_r4);   // Compare r3 vs r4 (signed)
ppc_cmpwi(ppc_cr0, ppc_r3, 100);     // Compare r3 vs 100 (signed)

// Compare unsigned (sets CR0)
ppc_cmplw(ppc_cr0, ppc_r3, ppc_r4);  // Compare r3 vs r4 (unsigned)
ppc_cmplwi(ppc_cr0, ppc_r3, 100);    // Compare r3 vs 100 (unsigned)
```

### Memory Operations

```c
// Load operations
ppc_lwz(ppc_r3, ppc_r4, 0);          // r3 = *(u32*)(r4 + 0)
ppc_lbz(ppc_r3, ppc_r4, 10);         // r3 = *(u8*)(r4 + 10)
ppc_lhz(ppc_r3, ppc_r4, 20);         // r3 = *(u16*)(r4 + 20)
ppc_lha(ppc_r3, ppc_r4, 20);         // r3 = *(s16*)(r4 + 20) (sign-extended)

// Zero-offset versions
ppc_lwz0(ppc_r3, ppc_r4);            // r3 = *(u32*)r4
ppc_lbz0(ppc_r3, ppc_r4);            // r3 = *(u8*)r4

// Store operations
ppc_stw(ppc_r3, ppc_r4, 0);          // *(u32*)(r4 + 0) = r3
ppc_stb(ppc_r3, ppc_r4, 10);         // *(u8*)(r4 + 10) = r3
ppc_sth(ppc_r3, ppc_r4, 20);         // *(u16*)(r4 + 20) = r3

// Indexed loads/stores
ppc_lwzx(ppc_r3, ppc_r4, ppc_r5);    // r3 = *(u32*)(r4 + r5)
ppc_stwx(ppc_r3, ppc_r4, ppc_r5);    // *(u32*)(r4 + r5) = r3
```

### Branching

```c
// Unconditional branch
ppc_b(offset);                       // PC += offset * 4
ppc_bl(offset);                      // LR = PC + 4; PC += offset * 4 (call)

// Conditional branches (based on CR0)
ppc_beq(offset);                     // Branch if equal
ppc_bne(offset);                     // Branch if not equal
ppc_blt(offset);                     // Branch if less than
ppc_bgt(offset);                     // Branch if greater than
ppc_ble(offset);                     // Branch if less than or equal
ppc_bge(offset);                     // Branch if greater than or equal

// Branch to link register (return)
ppc_blr();                           // PC = LR (return)

// Conditional return
ppc_beqlr();                         // if (CR0.EQ) PC = LR
ppc_bnelr();                         // if (!CR0.EQ) PC = LR

// Branch to counter register
ppc_bctr();                          // PC = CTR
ppc_bctrl();                         // LR = PC + 4; PC = CTR (computed call)
```

### Special Register Operations

```c
// Link register
ppc_mflr(ppc_r3);                    // r3 = LR
ppc_mtlr(ppc_r3);                    // LR = r3

// Counter register
ppc_mfctr(ppc_r3);                   // r3 = CTR
ppc_mtctr(ppc_r3);                   // CTR = r3

// Condition register
ppc_mfcr(ppc_r3);                    // r3 = CR
ppc_mtcrf(0xFF, ppc_r3);             // CR = r3 (all fields)
```

### Floating Point Operations

```c
// Move/copy
ppc_fmr(ppc_f1, ppc_f2);             // f1 = f2

// Arithmetic (double precision)
ppc_fadd(ppc_f1, ppc_f2, ppc_f3);    // f1 = f2 + f3
ppc_fsub(ppc_f1, ppc_f2, ppc_f3);    // f1 = f2 - f3
ppc_fmul(ppc_f1, ppc_f2, ppc_f3);    // f1 = f2 * f3
ppc_fdiv(ppc_f1, ppc_f2, ppc_f3);    // f1 = f2 / f3

// Arithmetic (single precision)
ppc_fadds(ppc_f1, ppc_f2, ppc_f3);   // f1 = f2 + f3 (single)
ppc_fsubs(ppc_f1, ppc_f2, ppc_f3);   // f1 = f2 - f3 (single)
ppc_fmuls(ppc_f1, ppc_f2, ppc_f3);   // f1 = f2 * f3 (single)
ppc_fdivs(ppc_f1, ppc_f2, ppc_f3);   // f1 = f2 / f3 (single)

// Other FP operations
ppc_fabs(ppc_f1, ppc_f2);            // f1 = abs(f2)
ppc_fneg(ppc_f1, ppc_f2);            // f1 = -f2

// Load/store floating point
ppc_lfs(ppc_f1, ppc_r3, 0);          // f1 = *(float*)(r3 + 0)
ppc_lfd(ppc_f1, ppc_r3, 0);          // f1 = *(double*)(r3 + 0)
ppc_stfs(ppc_f1, ppc_r3, 0);         // *(float*)(r3 + 0) = f1
ppc_stfd(ppc_f1, ppc_r3, 0);         // *(double*)(r3 + 0) = f1
```

## Function Prologue/Epilogue Example

```c
// Function prologue
void emit_function_prologue(int stack_size)
{
    // Create stack frame
    ppc_stwu_sp(-stack_size);          // SP = SP - stack_size; *(SP) = old_SP
    
    // Save link register
    ppc_mflr(ppc_r0);
    ppc_stw(ppc_r0, ppc_sp, stack_size + 4);
    
    // Save non-volatile registers if needed
    ppc_stw(ppc_r31, ppc_sp, stack_size - 4);
    // ... save other registers
}

// Function epilogue
void emit_function_epilogue(int stack_size)
{
    // Restore non-volatile registers
    ppc_lwz(ppc_r31, ppc_sp, stack_size - 4);
    // ... restore other registers
    
    // Restore link register
    ppc_lwz(ppc_r0, ppc_sp, stack_size + 4);
    ppc_mtlr(ppc_r0);
    
    // Destroy stack frame
    ppc_lwz_sp(0);                     // SP = *(SP)
    
    // Return
    ppc_blr();
}
```

## Label Usage for Branches

The label system allows forward references in branches:

```c
// Create a forward reference
ppc_label* forward_label = ppc_CreateLabel();

// Emit a conditional branch to the label
ppc_bne((forward_label - emit_GetCCPtr()) >> 2);

// ... emit other code ...

// Mark the label location
forward_label->MarkLabel();
```

## Example: Simple Addition Function

```c
// int add(int a, int b) { return a + b; }
void emit_add_function()
{
    // Arguments in r3 and r4, result in r3
    ppc_add(ppc_r3, ppc_r3, ppc_r4);   // r3 = r3 + r4
    ppc_blr();                          // return
}
```

## Example: Conditional Code

```c
// int max(int a, int b) { return (a > b) ? a : b; }
void emit_max_function()
{
    ppc_label* skip_label;
    
    // Compare a (r3) with b (r4)
    ppc_cmpw(ppc_cr0, ppc_r3, ppc_r4);
    
    // Create label for forward branch
    skip_label = ppc_CreateLabel();
    
    // If a >= b, skip the move
    ppc_bge((skip_label - emit_GetCCPtr()) >> 2);
    
    // a < b, so return b
    ppc_mr(ppc_r3, ppc_r4);
    
    // Mark skip label
    skip_label->MarkLabel();
    
    // Return
    ppc_blr();
}
```

## Example: Loop

```c
// int sum(int n) { int s = 0; for (int i = 0; i < n; i++) s += i; return s; }
void emit_sum_function()
{
    ppc_label* loop_start;
    ppc_label* loop_end;
    
    // r3 = n (input)
    // r4 = i (counter)
    // r5 = sum
    
    ppc_li(ppc_r5, 0);                    // sum = 0
    ppc_li(ppc_r4, 0);                    // i = 0
    
    loop_start = ppc_CreateLabel();
    
    // Compare i with n
    ppc_cmpw(ppc_cr0, ppc_r4, ppc_r3);
    
    loop_end = ppc_CreateLabel();
    ppc_bge((loop_end - emit_GetCCPtr()) >> 2);  // if i >= n, exit
    
    // sum += i
    ppc_add(ppc_r5, ppc_r5, ppc_r4);
    
    // i++
    ppc_addi(ppc_r4, ppc_r4, 1);
    
    // Continue loop
    ppc_b((loop_start - emit_GetCCPtr()) >> 2);
    
    loop_end->MarkLabel();
    
    // Return sum in r3
    ppc_mr(ppc_r3, ppc_r5);
    ppc_blr();
}
```

## Tips and Best Practices

1. **Stack Alignment**: Keep stack 16-byte aligned on PowerPC
2. **Register Allocation**: Use r14-r31 for values that need to survive function calls
3. **Branch Distances**: PowerPC branches have limited range (±32KB for conditional branches)
4. **Condition Register**: CR0 is set by arithmetic operations with the Rc bit set
5. **Cache Management**: Use `dcbst`, `icbi`, `sync`, `isync` for code modification
6. **Performance**: Keep hot code paths straight-line when possible

## Memory Barriers and Cache Control

```c
// Data cache block store (flush to memory)
ppc_dcbst(ppc_r0, ppc_r3);

// Instruction cache block invalidate
ppc_icbi(ppc_r0, ppc_r3);

// Synchronize
ppc_sync();                             // Wait for memory operations
ppc_isync();                            // Synchronize instruction fetch
```

## Notes

- All instructions are 32 bits (4 bytes)
- Big-endian byte order
- Offsets in load/store are signed 16-bit (-32768 to 32767)
- Branch offsets are in units of 4 bytes (instructions)
- Most instructions have a record (Rc) bit that updates CR0

## Reference

For detailed instruction encodings, see `ppc64_info.txt` and the PowerPC ISA documentation.
