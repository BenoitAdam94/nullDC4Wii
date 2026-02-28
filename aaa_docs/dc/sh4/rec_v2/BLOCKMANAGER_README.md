# Improved Block Manager for Dreamcast Emulator on Wii

## Overview
This is an improved version of the block manager component for a Dreamcast emulator adapted for the Nintendo Wii platform. The block manager handles dynamic recompilation (dynarec) block caching and lookup.

**Important**: This improved version maintains **full compatibility** with the original API and data structures, including the exposed cache array for assembly code access on platforms like Beagleboard, Pandora, and Wii.

## Key Improvements

### 1. **Better Memory Management**
- **Pre-allocated storage**: Uses `reserve()` to reduce memory fragmentation
- **Maximum bucket size**: Prevents pathological cases where a single hash bucket grows too large (64 blocks max)
- **LRU replacement**: When a bucket is full, the least-used block is replaced
- **Memory-efficient**: Better suited for Wii's limited RAM (~88 MB available)

### 2. **Performance Optimizations**
- **Improved cache locality**: Better data structure layout for CPU cache efficiency
- **Inline cache check**: Fast path for most common lookups
- **Threshold-based cache updates**: Prevents cache thrashing (requires +2 lookups advantage)
- **Optimized removal**: Uses swap-and-pop instead of erase for O(1) removal

### 3. **New Features** (fully backward compatible)
- **Block removal**: `bm_RemoveCode()` for selective invalidation
- **Range invalidation**: `bm_InvalidateRange()` for handling self-modifying code
- **Statistics tracking**: Optional profiling support (enable with `BM_ENABLE_STATS`)
- **Initialization function**: `bm_Init()` for proper setup with pre-allocation

### 4. **Code Quality**
- **Better documentation**: Clear comments explaining design decisions
- **Safer code**: Checks for duplicate blocks, validates cache pointers
- **Maintainable**: Cleaner structure with helper functions
- **Compatible**: Uses original `DynarecBlock` structure, exposes cache array for assembly code

## Compatibility Notes

The improved version maintains:
- ✅ Original `DynarecBlock` structure (code, addr, lookups)
- ✅ Exposed `cache[BM_BLOCKLIST_COUNT]` array for assembly access
- ✅ Original hash parameters (16K buckets, shift by 2)
- ✅ Same API: `bm_GetCode()`, `bm_AddCode()`, `bm_Reset()`
- ✅ `extern "C"` linkage for assembly code on Linux/Wii
- ✅ `FASTCALL` calling convention

## Configuration

Hash parameters match the original:

```cpp
#define BM_BLOCKLIST_COUNT (16384)  // 16K hash buckets
#define BM_BLOCKLIST_MASK (BM_BLOCKLIST_COUNT-1)
#define BM_BLOCKLIST_SHIFT 2        // 4-byte alignment
```

New configurable limits for Wii optimization:

```cpp
#define BM_INITIAL_CAPACITY 16       // Initial blocks per bucket
#define BM_MAX_BLOCKS_PER_BUCKET 64 // Maximum blocks per bucket
```

### Memory Usage
- Each `DynarecBlock`: 12 bytes (code ptr + addr + lookups)
- With 16K buckets, initial capacity of 16: ~3 MB base memory
- Maximum with 64 blocks/bucket: ~12 MB
- Cache array: 64 KB (16K pointers)

## Usage

### Initialization
**NEW**: Call during emulator startup for optimal performance:
```cpp
bm_Init();  // Pre-allocates memory, initializes cache
```

If you don't call `bm_Init()`, the code will still work but won't benefit from pre-allocation.

### Normal Operation
The API remains **100% compatible** with the original:
```cpp
// Lookup a block
DynarecCodeEntry* code = bm_GetCode(address);

// Add a new block
bm_AddCode(address, compiled_code);

// Reset all blocks (e.g., on emulator reset)
bm_Reset();
```

### Assembly Code Access
The cache array is still exposed for assembly code:
```cpp
extern DynarecBlock* cache[BM_BLOCKLIST_COUNT];

// Assembly code can directly access cache like before
// e.g., cache[hash_idx]->code
```

### New Functions (Optional)

#### Remove a specific block:
```cpp
if (bm_RemoveCode(address)) {
    // Block was found and removed
}
```

#### Invalidate a memory range (for self-modifying code):
```cpp
// Invalidate all blocks in address range
bm_InvalidateRange(start_address, end_address);
```

#### Statistics (when BM_ENABLE_STATS is defined):
```cpp
u32 hits, misses, cache_hits, total_blocks;
bm_GetStats(&hits, &misses, &cache_hits, &total_blocks);
printf("Cache efficiency: %.2f%%\n", 
       100.0f * cache_hits / (hits + cache_hits));
```

## Performance Tips

### For Wii Platform:
1. **Call bm_Init()**: Ensures memory is pre-allocated, reducing fragmentation
2. **Monitor bucket distribution**: Use stats to verify hash function works well
3. **Adjust max blocks per bucket**: Balance between memory usage and collision handling
4. **Enable stats during development**: Disable in release builds for best performance

### Hash Function:
The hash function `(addr >> 2) & 0x3FFF` assumes:
- 4-byte alignment (shift by 2) - matches SH4 instruction alignment
- 16K buckets - good balance for most games

## Wii-Specific Considerations

### Memory Constraints:
- Wii has ~88 MB usable RAM (24 MB main + 64 MB auxiliary)
- Block manager uses ~3-12 MB depending on usage
- Pre-allocation helps avoid fragmentation issues

### Cache Performance:
- Wii's Broadway CPU has 32 KB L1 cache
- Sequential access patterns work best
- Threshold-based cache updates reduce thrashing

### PowerPC Architecture:
- Code marked `FASTCALL` uses PowerPC calling conventions
- Consider using `__attribute__((hot))` for `bm_GetCode()` if using GCC
- Align frequently accessed structures to cache line boundaries (32 bytes)

## Migration from Original

### Minimal Changes Required:
1. ✅ Replace `blockmanager.cpp` and `blockmanager.h`
2. ✅ Add `bm_Init()` call during emulator startup (optional but recommended)
3. ✅ Done! Everything else is compatible

### Optional Enhancements:
1. Call `bm_InvalidateRange()` when detecting self-modifying code
2. Use `bm_RemoveCode()` for selective invalidation instead of full reset
3. Monitor statistics to tune parameters

### No Breaking Changes:
- Assembly code continues to work unchanged
- All existing code using the block manager works as-is
- Cache array access patterns remain the same

## Testing Recommendations

1. **Correctness**: Verify emulator still runs games correctly
2. **Performance**: Use stats to measure cache hit rates (aim for >95%)
3. **Memory**: Monitor total block count and memory usage
4. **Edge cases**: Test with games that have self-modifying code
5. **Stress test**: Run for extended periods to check for leaks
6. **Assembly code**: Verify any assembly that accesses cache array still works

## What Changed Under the Hood

### Improvements:
- ✅ Pre-allocation with `reserve()` to reduce fragmentation
- ✅ Max bucket size enforcement (prevents pathological cases)
- ✅ LRU replacement when buckets fill up
- ✅ Threshold-based cache updates (+2 lookups) to prevent thrashing
- ✅ Optimized removal using swap-and-pop
- ✅ Range invalidation for self-modifying code handling
- ✅ Optional statistics tracking

### What Stayed the Same:
- ✅ `DynarecBlock` structure (code, addr, lookups)
- ✅ Cache array exposure for assembly
- ✅ Hash function and parameters
- ✅ Core API (`bm_GetCode`, `bm_AddCode`, `bm_Reset`)
- ✅ Vector-based storage (same as original)

## Potential Future Improvements

1. **Block chaining**: Link frequently executed block sequences
2. **Hot path optimization**: Separate hot and cold blocks
3. **Compressed blocks**: Store rarely used blocks in compressed form
4. **Multi-level cache**: Add L2 cache for medium-frequency blocks
5. **Profile-guided optimization**: Use runtime stats to reorganize blocks

## Debugging

When issues occur:
1. Enable `BM_ENABLE_STATS` and check hit rates
2. Verify hash distribution is even across buckets
3. Check for excessive evictions (max bucket size being hit)
4. Monitor memory growth over time
5. Use Wii debugging tools to profile CPU cache misses
6. Verify assembly code cache access still works correctly

## License

Maintain the same license as the original Dreamcast emulator code.
