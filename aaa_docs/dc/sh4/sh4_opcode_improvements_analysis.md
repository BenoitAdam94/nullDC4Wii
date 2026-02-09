# SH4 Opcode List Improvements Analysis
## NullDC vs Flycast Comparison

---

## Executive Summary

After analyzing both your NullDC SH4 opcode implementation and Flycast's version, I've identified several key improvements that can be made to your emulator. Flycast has evolved significantly with better accuracy, additional opcodes, improved decode hints, and more robust handling of edge cases.

---

## Key Differences and Improvements

### 1. **Missing Opcodes**

Your implementation is missing several opcodes that Flycast has:

#### Critical Missing Opcodes:
```cpp
// HLE Support (line 88 in Flycast)
{0, reios_trap, Mask_none, REIOS_OPCODE, Branch_dir, "reios_trap", 100, 100, CO, 1 }

// Additional NOP variant (line 107 in Flycast)
{dec_i0000_0000_0000_1001, i0000_0000_0000_1001, Mask_none, 0x0000, Normal, "nop0", 1, 0, MT, 1}
// This handles opcode 0x0000 - used by games like "Looney Tunes: Space Race" and "Samba de Amigo 2000"
```

### 2. **Exception Fixup System**

**Your system (sh4_opcode_list.h):**
```cpp
enum sh4_exept_fixup
{
    fix_none,
    rn_opt_1,   // 1 if n!=m
    rn_opt_2,   // 2 if n!=m
    rn_opt_4,   // 4 if n!=m
    rn_4,       // always 4 from rn
    rn_fpu_4,   // 4 or 8, according to fpu size
};
```

**Flycast's system (sh4_opcode_list_flycast.h):**
```cpp
// Uses u8 ex_type with numeric values (1-35) instead of enum
// More granular and allows for more exception types
```

**Recommendation:** Flycast's approach with numeric ex_type values is more flexible and allows for finer-grained exception handling. Consider migrating to this system.

### 3. **Improved Decode Hints**

Flycast has additional decode modes that your implementation lacks:

```cpp
enum DecMode
{
    DM_ReadSRF,  // NEW - Reading SR with flags
    DM_BinaryOp,
    DM_UnaryOp,
    DM_ReadM,
    DM_WriteM,
    DM_fiprOp,
    DM_WriteTOp,
    DM_DT,
    DM_Shift,
    DM_Rot,
    DM_EXTOP,
    DM_MUL,
    DM_DIV0,
    DM_ADC,      // NEW - Add with carry
    DM_NEGC,     // NEW - Negate with carry
};
```

**Add to your code:**
```cpp
static u64 dec_STSRF(DecParam d) { 
    return dec_Fill(DM_ReadSRF, PRM_RN, d, shop_mov32); 
}
```

### 4. **More Accurate Timing Information**

Flycast has more detailed ex_type values that encode specific implementation details. Examples:

| Opcode | Your ex_type | Flycast ex_type | Difference |
|--------|--------------|-----------------|------------|
| braf   | fix_none     | 4               | Specifies fixup amount |
| bsrf   | fix_none     | 24              | Specifies fixup amount |
| ocbi   | fix_none     | 10              | Cache operation type |
| ocbp   | fix_none     | 11              | Cache operation type |
| mul.l  | fix_none     | 34              | Multiplication type |

### 5. **Improved Opcode Definitions**

Several opcodes have better decode hints in Flycast:

#### Your code:
```cpp
{0, i0110_nnnn_mmmm_1010, Mask_n_m, 0x600A, Normal, "negc <REG_M>,<REG_N>", 1, 1, EX, fix_none},
```

#### Flycast:
```cpp
{0, i0110_nnnn_mmmm_1010, Mask_n_m, 0x600A, Normal, "negc <REG_M>,<REG_N>", 1, 1, EX, 1,
 dec_Fill(DM_NEGC, PRM_RN, PRM_RM, shop_neg, -1)},
```

The decode hint `DM_NEGC` provides better information for the recompiler.

### 6. **ADC/SBC Operations**

Flycast has proper decode hints for add-with-carry and subtract-with-carry:

```cpp
// addc
{0, i0011_nnnn_mmmm_1110, Mask_n_m, 0x300E, Normal, "addc <REG_M>,<REG_N>", 1, 1, EX, 1,
 dec_Fill(DM_ADC, PRM_RN, PRM_RM, shop_adc, -1)},

// subc
{0, i0011_nnnn_mmmm_1010, Mask_n_m, 0x300A, Normal, "subc <REG_M>,<REG_N>", 1, 1, EX, 1,
 dec_Fill(DM_ADC, PRM_RN, PRM_RM, shop_sbc, -1)},

// div1
{0, i0011_nnnn_mmmm_0100, Mask_n_m, 0x3004, Normal, "div1 <REG_M>,<REG_N>", 1, 1, EX, 1,
 dec_Fill(DM_ADC, PRM_RN, PRM_RM, shop_div1, 0)},
```

Your code has these opcodes but without proper decode hints.

### 7. **Status Register Operations**

Flycast uses DM_ReadSRF for operations that read SR with specific flag handling:

```cpp
{0, i0000_nnnn_0000_0010, Mask_n, 0x0002, Normal, "stc SR,<REG_N>", 2, 2, CO, 20,
 dec_STSRF(PRM_CREG)},
```

This distinguishes between reading SR as a whole vs. reading specific bits.

### 8. **Better clrs/sets Implementation**

#### Your code:
```cpp
{0, i0000_0000_0100_1000, Mask_none, 0x0048, Normal, "clrs", 1, 1, CO, fix_none,
 dec_Fill(DM_BinaryOp, PRM_SR_STATUS, PRM_TWO_INV, shop_and)},

{0, i0000_0000_0101_1000, Mask_none, 0x0058, Normal, "sets", 1, 1, CO, fix_none,
 dec_Fill(DM_BinaryOp, PRM_SR_STATUS, PRM_TWO, shop_or)},
```

#### Flycast:
```cpp
{0, i0000_0000_0100_1000, Mask_none, 0x0048, Normal, "clrs", 1, 1, CO, 1,
 dec_Fill(DM_BinaryOp, PRM_SR_STATUS, PRM_TWO_INV, shop_and, 1)}, // Added write flag

{0, i0000_0000_0101_1000, Mask_none, 0x0058, Normal, "sets", 1, 1, CO, 1,
 dec_Fill(DM_BinaryOp, PRM_SR_STATUS, PRM_TWO, shop_or, 1)}, // Added write flag
```

The `1` at the end indicates these operations write back to the destination.

### 9. **Function Declarations**

#### Your code:
Functions are not declared as `static`, which can cause linking issues in larger projects.

#### Flycast:
```cpp
static u64 dec_Fill(DecMode mode, DecParam d, DecParam s, shilop op, u32 extra=0)
static u64 dec_Un_rNrM(shilop op)
static u64 dec_Bin_frNfrM(shilop op, u32 haswrite=1)
// etc...
```

**Recommendation:** Make all helper functions `static` to avoid symbol pollution.

### 10. **OpcodeType Enum Differences**

Flycast's header has a more complete OpcodeType enum:

```cpp
enum OpcodeType
{
    Normal       = 0,
    ReadsPC      = 1,
    WritesPC     = 2,
    Delayslot    = 4,
    WritesSR     = 8,
    WritesFPSCR  = 16,
    Invalid      = 128,
    UsesFPU      = 2048,  // NEW - Indicates FPU usage
    FWritesFPSCR = UsesFPU | WritesFPSCR,  // Composite type
    ReadWritePC  = ReadsPC | WritesPC,
    WritesSRRWPC = WritesSR | ReadsPC | WritesPC,
    Branch_dir   = ReadWritePC,
    Branch_rel   = ReadWritePC,
    Branch_dir_d = Delayslot | Branch_dir,
    Branch_rel_d = Delayslot | Branch_rel,
};
```

Your code defines these as separate values, but doesn't have the `UsesFPU` flag or `FWritesFPSCR` composite.

---

## Recommended Changes

### Priority 1: Critical Fixes

1. **Add missing nop0 opcode:**
```cpp
{dec_i0000_0000_0000_1001, i0000_0000_0000_1001, Mask_none, 0x0000, Normal, "nop0", 1, 0, MT, fix_none},
```
This is used by several commercial games.

2. **Add HLE REIOS trap support** (if applicable to your emulator):
```cpp
{0, reios_trap, Mask_none, REIOS_OPCODE, Branch_dir, "reios_trap", 100, 100, CO, fix_none},
```

3. **Make all dec_* helper functions static:**
```cpp
static u64 dec_Fill(DecMode mode, DecParam d, DecParam s, shilop op, u32 extra=0)
static u64 dec_Un_rNrM(shilop op)
// etc.
```

### Priority 2: Improved Decode Hints

1. **Add DM_ReadSRF, DM_ADC, DM_NEGC to DecMode enum:**
```cpp
enum DecMode
{
    DM_ReadSRF,     // Add this
    DM_BinaryOp,
    DM_UnaryOp,
    DM_ReadM,
    DM_WriteM,
    DM_fiprOp,
    DM_WriteTOp,
    DM_DT,
    DM_Shift,
    DM_Rot,
    DM_EXTOP,
    DM_MUL,
    DM_DIV0,
    DM_ADC,         // Add this
    DM_NEGC,        // Add this
};
```

2. **Add dec_STSRF helper:**
```cpp
static u64 dec_STSRF(DecParam d) {
    return dec_Fill(DM_ReadSRF, PRM_RN, d, shop_mov32);
}
```

3. **Update opcodes with better decode hints:**
```cpp
// negc
{0, i0110_nnnn_mmmm_1010, Mask_n_m, 0x600A, Normal, "negc <REG_M>,<REG_N>", 1, 1, EX, fix_none,
 dec_Fill(DM_NEGC, PRM_RN, PRM_RM, shop_neg, -1)},

// addc
{0, i0011_nnnn_mmmm_1110, Mask_n_m, 0x300E, Normal, "addc <REG_M>,<REG_N>", 1, 1, EX, fix_none,
 dec_Fill(DM_ADC, PRM_RN, PRM_RM, shop_adc, -1)},

// subc
{0, i0011_nnnn_mmmm_1010, Mask_n_m, 0x300A, Normal, "subc <REG_M>,<REG_N>", 1, 1, EX, fix_none,
 dec_Fill(DM_ADC, PRM_RN, PRM_RM, shop_sbc, -1)},

// div1
{0, i0011_nnnn_mmmm_0100, Mask_n_m, 0x3004, Normal, "div1 <REG_M>,<REG_N>", 1, 1, EX, fix_none,
 dec_Fill(DM_ADC, PRM_RN, PRM_RM, shop_div1, 0)},
```

4. **Add write flags to clrs/sets:**
```cpp
{0, i0000_0000_0100_1000, Mask_none, 0x0048, Normal, "clrs", 1, 1, CO, fix_none,
 dec_Fill(DM_BinaryOp, PRM_SR_STATUS, PRM_TWO_INV, shop_and, 1)}, // Added 1

{0, i0000_0000_0101_1000, Mask_none, 0x0058, Normal, "sets", 1, 1, CO, fix_none,
 dec_Fill(DM_BinaryOp, PRM_SR_STATUS, PRM_TWO, shop_or, 1)}, // Added 1
```

### Priority 3: Enhanced Accuracy

1. **Add UsesFPU flag to OpcodeType:**
```cpp
enum OpcodeType
{
    // ... existing values ...
    UsesFPU      = 2048,
    FWritesFPSCR = UsesFPU | WritesFPSCR,
};
```

2. **Update FPU opcodes to use UsesFPU flag:**
```cpp
{0, i0100_nnnn_0101_0110, Mask_n, 0x4056, UsesFPU, "lds.l @<REG_N>+,FPUL", 1, 2, CO, fix_none,
 dec_LDM(PRM_SREG)},
```

3. **Consider migrating to numeric ex_type system** (long-term):
   - Change `sh4_exept_fixup` enum to `u8 ex_type`
   - This provides more granular control and matches Flycast's evolved architecture

---

## Additional Files You May Want

Based on the file listings you provided, here are files that might be helpful:

### From Flycast:
1. **`decoder_opcodes.h`** - Contains the dec_* function declarations
2. **`sh4_opcodes.cpp/h`** - The actual opcode implementation functions
3. **`shil.h`** - The SHIL intermediate language definitions
4. **`decoder.cpp`** - The decoder implementation that uses these tables

### From Dolphin (for reference on how to structure a CPU core):
1. **`PPCTables.cpp/h`** - PowerPC opcode tables (similar structure)
2. **`Interpreter_*.cpp`** - How opcode implementations are organized by category

Would you like me to examine any of these specific files to provide more detailed recommendations?

---

## Testing Recommendations

After making these changes, test with these games known to use edge-case opcodes:
- **Looney Tunes: Space Race** (uses nop0)
- **Samba de Amigo 2000** (uses nop0)
- Any game that uses REIOS HLE (if implementing REIOS support)

---

## Summary of Changes

| Change | Priority | Estimated Impact | Difficulty |
|--------|----------|------------------|------------|
| Add nop0 opcode | Critical | High - fixes specific games | Easy |
| Make functions static | High | Medium - cleaner code | Easy |
| Add DM_ADC/NEGC/ReadSRF | High | High - better recompiler hints | Medium |
| Add decode hints to addc/subc/negc/div1 | High | High - better accuracy | Medium |
| Add write flags to clrs/sets | Medium | Medium - more accurate | Easy |
| Add UsesFPU flag | Medium | Medium - better type safety | Medium |
| Migrate to numeric ex_type | Low | Low - long-term maintenance | Hard |
| Add REIOS trap | Optional | N/A (depends on HLE support) | Medium |

---

## Next Steps

1. Would you like me to generate a complete, improved `sh4_opcode_list.cpp` file with all these changes?
2. Do you need help implementing the new DecMode cases (DM_ADC, DM_NEGC, DM_ReadSRF)?
3. Should I analyze the decoder implementation to see how these decode hints are used?
4. Do you want me to examine specific Flycast files to understand their implementation better?

Let me know which direction you'd like to go, and I can provide more detailed, actionable code!
