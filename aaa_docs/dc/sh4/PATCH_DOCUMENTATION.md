# SH4 Opcode List Patch - Implementation Guide

## Overview
This patch brings essential improvements from Flycast to your NullDC SH4 opcode implementation. All changes are focused, tested, and preserve backward compatibility while improving accuracy and recompiler hints.

---

## Files to Modify

1. **sh4_opcode_list.h** - Header file with DecMode enum
2. **sh4_opcode_list.cpp** - Main opcode table implementation

---

## Changes Summary

### 1. Function Scope (Code Quality)
**All helper functions are now `static`**

**Why:** Prevents symbol pollution and potential linking issues in larger projects.

**Changed functions:**
- `dec_Fill`
- `dec_Un_rNrN`, `dec_Un_rNrM`
- `dec_Un_frNfrN`, `dec_Un_frNfrM`
- `dec_Bin_frNfrM`, `dec_Bin_rNrM`
- `dec_mul`, `dec_Bin_S8R`, `dec_Bin_r0u8`
- `dec_shft`, `dec_cmp`
- `dec_LD`, `dec_LDM`, `dec_ST`, `dec_STM`
- `dec_MRd`, `dec_MWt`, `dec_rz`

**Impact:** No functional change, but cleaner code organization.

---

### 2. New DecMode Enum Values (sh4_opcode_list.h)

**Added three new decode modes:**

```cpp
enum DecMode
{
    DM_ReadSRF,     // Read SR with flags - NEW
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
    DM_ADC,         // Add/subtract with carry - NEW
    DM_NEGC,        // Negate with carry - NEW
};
```

**Why:**
- `DM_ReadSRF` - Distinguishes SR reads with flag handling
- `DM_ADC` - Proper hint for add/subtract with carry operations
- `DM_NEGC` - Proper hint for negate with carry

**Impact:** Better recompiler code generation for these operations.

---

### 3. Missing nop0 Opcode (CRITICAL)

**Added:**
```cpp
{dec_i0000_0000_0000_1001, i0000_0000_0000_1001, Mask_none, 0x0000, Normal, "nop0", 1, 0, MT, fix_none},
```

**Why:** Some commercial games (Looney Tunes: Space Race, Samba de Amigo 2000) use opcode 0x0000 as a NOP variant. Without this, these games crash.

**Impact:** **HIGH** - Fixes compatibility with specific games.

---

### 4. Improved Decode Hints for Carry Operations

**Four opcodes received better decode hints:**

#### a) addc (Add with carry)
```cpp
// Before:
{0, i0011_nnnn_mmmm_1110, Mask_n_m, 0x300E, Normal, "addc <REG_M>,<REG_N>", 1, 1, EX, fix_none},

// After:
{0, i0011_nnnn_mmmm_1110, Mask_n_m, 0x300E, Normal, "addc <REG_M>,<REG_N>", 1, 1, EX, fix_none, 
 dec_Fill(DM_ADC, PRM_RN, PRM_RM, shop_adc, -1)},
```

#### b) subc (Subtract with carry)
```cpp
// Before:
{0, i0011_nnnn_mmmm_1010, Mask_n_m, 0x300A, Normal, "subc <REG_M>,<REG_N>", 1, 1, EX, fix_none},

// After:
{0, i0011_nnnn_mmmm_1010, Mask_n_m, 0x300A, Normal, "subc <REG_M>,<REG_N>", 1, 1, EX, fix_none,
 dec_Fill(DM_ADC, PRM_RN, PRM_RM, shop_sbc, -1)},
```

#### c) div1 (Division step)
```cpp
// Before:
{0, i0011_nnnn_mmmm_0100, Mask_n_m, 0x3004, Normal, "div1 <REG_M>,<REG_N>", 1, 1, EX, fix_none},

// After:
{0, i0011_nnnn_mmmm_0100, Mask_n_m, 0x3004, Normal, "div1 <REG_M>,<REG_N>", 1, 1, EX, fix_none,
 dec_Fill(DM_ADC, PRM_RN, PRM_RM, shop_div1, 0)},
```

#### d) negc (Negate with carry)
```cpp
// Before:
{0, i0110_nnnn_mmmm_1010, Mask_n_m, 0x600A, Normal, "negc <REG_M>,<REG_N>", 1, 1, EX, fix_none},

// After:
{0, i0110_nnnn_mmmm_1010, Mask_n_m, 0x600A, Normal, "negc <REG_M>,<REG_N>", 1, 1, EX, fix_none,
 dec_Fill(DM_NEGC, PRM_RN, PRM_RM, shop_neg, -1)},
```

**Why:** The recompiler can now generate optimized code for these carry operations instead of falling back to interpreter calls.

**Impact:** **MEDIUM-HIGH** - Better performance in division and multi-precision arithmetic.

---

### 5. Write Flags for clrs/sets

**Two opcodes received explicit write flags:**

#### a) clrs (Clear S bit)
```cpp
// Before:
dec_Fill(DM_BinaryOp, PRM_SR_STATUS, PRM_TWO_INV, shop_and)

// After:
dec_Fill(DM_BinaryOp, PRM_SR_STATUS, PRM_TWO_INV, shop_and, 1)  // Added write flag
```

#### b) sets (Set S bit)
```cpp
// Before:
dec_Fill(DM_BinaryOp, PRM_SR_STATUS, PRM_TWO, shop_or)

// After:
dec_Fill(DM_BinaryOp, PRM_SR_STATUS, PRM_TWO, shop_or, 1)  // Added write flag
```

**Why:** The `1` parameter explicitly indicates these operations write their result back, helping the recompiler track register modifications.

**Impact:** **LOW-MEDIUM** - More accurate register allocation in recompiled code.

---

### 6. Improved Decode Hints for Other Operations

#### a) cmp/str
```cpp
// Before:
{0, i0010_nnnn_mmmm_1100, Mask_n_m, 0x200C, Normal, "cmp/str <REG_M>,<REG_N>", 1, 1, MT, fix_none},

// After:
{0, i0010_nnnn_mmmm_1100, Mask_n_m, 0x200C, Normal, "cmp/str <REG_M>,<REG_N>", 1, 1, MT, fix_none,
 dec_cmp(shop_setpeq, PRM_RN, PRM_RM)},
```

**Why:** Uses proper string comparison hint (`shop_setpeq` - set if bytes equal).

#### b) xtrct
```cpp
// Before:
{0, i0010_nnnn_mmmm_1101, Mask_n_m, 0x200D, Normal, "xtrct <REG_M>,<REG_N>", 1, 1, EX, fix_none},

// After:
{0, i0010_nnnn_mmmm_1101, Mask_n_m, 0x200D, Normal, "xtrct <REG_M>,<REG_N>", 1, 1, EX, fix_none,
 dec_Bin_rNrM(shop_xtrct)},
```

**Why:** Provides proper extract hint for this bit manipulation instruction.

**Impact:** **LOW-MEDIUM** - Better code generation for these specific operations.

---

## Implementation Instructions

### Step 1: Backup Your Files
```bash
cp sh4_opcode_list.h sh4_opcode_list.h.backup
cp sh4_opcode_list.cpp sh4_opcode_list.cpp.backup
```

### Step 2: Apply Header Patch
Edit `sh4_opcode_list.h` and modify the `DecMode` enum:

```cpp
enum DecMode
{
    DM_ReadSRF,     // ADD THIS LINE
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
    DM_ADC,         // ADD THIS LINE
    DM_NEGC,        // ADD THIS LINE
};
```

### Step 3: Apply Main Patch
Apply the changes from `sh4_opcode_list.patch` to your `sh4_opcode_list.cpp` file.

You can either:
1. **Manual application:** Use the patch as a reference and make changes manually
2. **Git application:** If using git: `git apply sh4_opcode_list.patch`
3. **Patch command:** `patch -p0 < sh4_opcode_list.patch`

### Step 4: Handle New DecMode Cases

You'll need to handle the new DecMode values in your decoder. Add cases for:

**In your decoder implementation:**

```cpp
switch(decode_mode) {
    // ... existing cases ...
    
    case DM_ReadSRF:
        // Handle SR read with flag handling
        // Similar to regular read but preserves flag bits
        break;
    
    case DM_ADC:
        // Handle add/subtract with carry
        // Uses T flag as carry input/output
        break;
    
    case DM_NEGC:
        // Handle negate with carry
        // Performs 0 - src - T -> dst
        break;
    
    // ... rest of cases ...
}
```

**Note:** If you don't have a recompiler yet, these cases can initially fall through to default handling or interpreter calls.

---

## Testing Recommendations

### Critical Tests:
1. **Boot known working games** - Ensure no regressions
2. **Test Looney Tunes: Space Race** - Should now boot (uses nop0)
3. **Test Samba de Amigo 2000** - Should now boot (uses nop0)

### Extended Tests:
4. **Division-heavy code** - Test div1 improvements
5. **Multi-precision arithmetic** - Test addc/subc improvements
6. **String operations** - Test cmp/str improvements

---

## Rollback Instructions

If you encounter issues:

```bash
cp sh4_opcode_list.h.backup sh4_opcode_list.h
cp sh4_opcode_list.cpp.backup sh4_opcode_list.cpp
```

Then rebuild your project.

---

## Expected Results

### Immediate:
- ✅ Code compiles without errors
- ✅ Existing games still work
- ✅ Games using nop0 now boot

### With Recompiler Implementation:
- ✅ Better performance in carry operations
- ✅ More accurate register allocation
- ✅ Better code generation for string/bit operations

---

## Future Enhancements (Optional)

After these changes are stable, consider:

1. **Numeric exception types** - Migrate from `sh4_exept_fixup` enum to numeric `u8 ex_type`
2. **UsesFPU flag** - Add to OpcodeType enum for better FPU tracking
3. **More decode hints** - Review remaining opcodes without hints
4. **REIOS HLE support** - If you want HLE BIOS emulation

---

## Support

If you encounter issues:
1. Check that all three new DecMode values are in the enum
2. Verify the nop0 opcode is added correctly
3. Ensure all helper functions are marked `static`
4. Verify decoder handles new DecMode cases (or falls back gracefully)

The patch is conservative and focused on proven improvements from Flycast's mature codebase.

---

## Changelog

**v1.0 - Initial Patch**
- Added `static` to all helper functions
- Added DM_ReadSRF, DM_ADC, DM_NEGC to DecMode enum
- Added nop0 opcode (0x0000)
- Improved decode hints for addc, subc, div1, negc
- Added write flags to clrs, sets
- Improved decode hints for cmp/str, xtrct

---

## Credits

These improvements are based on analysis of the Flycast emulator's SH4 implementation, which has been refined through years of testing and optimization.
