# Build Optimization Notes

## Current Solution

The functions are now compiled in `pvrLock.cpp` and can be called from any file including plugins.

## Performance Optimization

To get similar performance to inline functions, enable **Link-Time Optimization (LTO)** in your Makefile:

### Add to your Makefile:

```makefile
# Enable link-time optimization for cross-module inlining
CFLAGS += -flto
LDFLAGS += -flto

# Optional: More aggressive optimization
CFLAGS += -O3 -fomit-frame-pointer
```

### What LTO does:
- Allows GCC to inline functions across different .cpp files
- The `vramlock_Convert*` functions will be inlined into `gxRend.cpp` and other files automatically
- Provides nearly the same performance as manually inlining
- No code changes needed - just compiler flags

### Alternative: Manual Inlining (if LTO causes issues)

If LTO causes problems, you can manually inline the hot paths in your renderer:

```cpp
// In gxRend.cpp, for critical performance sections:
#define VRAMLOCK_INLINE_CONVERT32TO64(offset32) ({ \
    u32 _o32 = (offset32) & VRAM_MASK; \
    u32 _bank = ((_o32 >> 22) & 0x1) << 2; \
    u32 _lower = _o32 & 0x3; \
    u32 _shifted = (_o32 & 0x3FFFFC) << 1; \
    _shifted | _bank | _lower; \
})

// Then use: u32 offset = VRAMLOCK_INLINE_CONVERT32TO64(my_offset);
```

## Verification

To check if LTO is working:

```bash
# Compile with LTO and check assembly
powerpc-eabi-gcc -S -O3 -flto gxRend.cpp -o gxRend.s

# Look for 'bl vramlock_ConvOffset32toOffset64' in the assembly
# If LTO is working, you should see the bit manipulation code inlined instead
```

## Performance Testing

Profile before and after:
1. Without LTO: Standard function calls
2. With LTO: Automatic inlining where beneficial
3. Expected improvement: 10-30% in VRAM-heavy rendering code
