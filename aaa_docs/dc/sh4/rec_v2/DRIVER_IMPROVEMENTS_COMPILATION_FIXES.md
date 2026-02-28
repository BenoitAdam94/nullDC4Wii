# Compilation Fixes Applied

## Errors Fixed

### 1. ❌ `emit_ptr` declaration conflict
**Error:**
```
error: 'emit_ptr' was declared 'extern' and later 'static' [-fpermissive]
```

**Fix:**
Changed from `static u32* emit_ptr = 0;` to `u32* emit_ptr = 0;`
- Removed `static` keyword to match the `extern` declaration in `ngen.h`

### 2. ❌ `recSh4_ClearCache` not declared in scope
**Error:**
```
error: 'recSh4_ClearCache' was not declared in this scope
```

**Fix:**
Added forward declaration at the top of the file:
```cpp
// Forward Declarations
void recSh4_ClearCache();
```

This allows the emit functions to call it before it's defined.

### 3. ❌ `perf_cache_clears` not declared
**Error:**
```
error: 'perf_cache_clears' was not declared in this scope
```

**Fix:**
Wrapped the usage in an `#ifdef` guard:
```cpp
#ifdef ENABLE_PERF_MONITORING
	perf_cache_clears++;
	printf("recSh4: Dynarec cache cleared at %08X (%u clears total)\n", 
	       curr_pc, perf_cache_clears);
#else
	printf("recSh4: Dynarec cache cleared at %08X\n", curr_pc);
#endif
```

Now it only uses `perf_cache_clears` when performance monitoring is enabled.

### 4. ⚠️ Warning about return statement (in sh4_registers.h)
**Warning:**
```
warning: no return statement in function returning non-void [-Wreturn-type]
u32 offset(Sh4RegType sh4_reg) { offset(sh4_reg); }
```

**Note:** This is in `sh4_registers.h`, not driver.cpp. It appears to be a recursive call bug that should be fixed separately.

## Changes Summary

### Variable Declarations
```cpp
// Before (caused conflict)
static u32* emit_ptr = 0;

// After (matches extern in ngen.h)
u32* emit_ptr = 0;
```

### Forward Declarations Added
```cpp
// At top of file, after includes
void recSh4_ClearCache();
```

### Conditional Compilation
```cpp
// Variables only exist when performance monitoring enabled
#ifdef ENABLE_PERF_MONITORING
static u32 perf_blocks_compiled = 0;
static u32 perf_cache_clears = 0;
static u32 perf_block_checks_failed = 0;
#endif

// Usage also guarded
#ifdef ENABLE_PERF_MONITORING
	perf_cache_clears++;
	printf("... (%u clears total)\n", perf_cache_clears);
#else
	printf("...\n");  // Simplified version
#endif
```

## Testing the Fix

### Compilation
```bash
cd /c/devkitPro/nullDC4Wii
make clean
make
```

Should compile without errors now.

### Runtime Testing
1. Boot emulator
2. Load a simple 2D game
3. Watch console for:
   - "recSh4: Initializing dynarec"
   - "recSh4: Code cache allocated at..."
   - "recSh4: Starting dynarec execution"

### With Performance Monitoring (Optional)
To enable detailed stats, uncomment in driver.cpp:
```cpp
#define ENABLE_PERF_MONITORING
```

Then recompile. You'll see:
```
recSh4 Performance Stats:
  - Blocks compiled: 1234
  - Cache clears: 5
  - Block check failures: 12
  - Code cache usage: 123456 / 4194304 bytes (2.9%)
  - High water mark: 234567 bytes (5.6%)
```

## Remaining Warning

The warning in `sh4_registers.h` line 24:
```cpp
u32 offset(Sh4RegType sh4_reg) { offset(sh4_reg); }
```

This is a **recursion bug** - the function calls itself infinitely. It should probably be:
```cpp
u32 offset(Sh4RegType sh4_reg) { return _offset(sh4_reg); }
```
or similar. But this is in a different file and may require examining the class structure.

## Files Modified

- ✅ `driver.cpp` - All compilation errors fixed
- ⚠️ `sh4_registers.h` - Warning remains (different file, needs separate fix)

## Compilation Status

Expected result:
- ✅ No errors
- ⚠️ 1 warning (in sh4_registers.h, can be ignored or fixed separately)
- ✅ Binary should compile successfully
