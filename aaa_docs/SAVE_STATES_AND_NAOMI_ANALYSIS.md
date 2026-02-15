# Naomi Emulation on Wii - Memory Feasibility Analysis

## Quick Answer

**Yes, you can emulate Naomi on Wii!** But you'll need to choose between:
- ✅ **Naomi emulation** (fits comfortably)
- ⚠️ **Save states** (would need careful management)
- ❌ **Both at the same time** (doesn't fit)

---

## What is Naomi?

**NAOMI** = **N**aomi **A**rcade **M**achine with **O**perating System **I**nstalled

- Released: 1998 (same year as Dreamcast)
- Manufacturer: Sega
- Purpose: Arcade system board
- Architecture: **Almost identical to Dreamcast!**

---

## Naomi Hardware Specifications

### Naomi vs Dreamcast Comparison

| Component | Dreamcast | Naomi | Difference |
|-----------|-----------|-------|------------|
| **CPU** | Hitachi SH-4 @ 200 MHz | Hitachi SH-4 @ 200 MHz | ✅ Identical |
| **Main RAM** | 16 MB | **32 MB** | 🔴 **2x more** |
| **VRAM** | 8 MB | 8 MB | ✅ Identical |
| **Sound RAM** | 2 MB | 2 MB | ✅ Identical |
| **GPU** | PowerVR CLX2 | PowerVR CLX2 | ✅ Identical |
| **Sound** | AICA (Yamaha) | AICA (Yamaha) | ✅ Identical |
| **Media** | GD-ROM | ROM Board / Cart | Different |
| **Total RAM** | **26 MB** | **42 MB** | **+16 MB** |

### Key Differences:

1. **Main RAM: 32 MB instead of 16 MB** (the BIG difference)
2. **ROM-based games** instead of disc (easier to emulate!)
3. **No GD-ROM drive** (simpler hardware)
4. **Better cooling** (arcade cabinet)

---

## Memory Requirements Analysis

### Current Dreamcast Memory Usage

```
MEM2 (64 MB available):
├── DC Sound RAM:     2 MB
├── DC VRAM:          8 MB
├── DC Main RAM:     16 MB
├── VRAM buffer:     16 MB
├── Emulator code:   ~6 MB
├── Save state:       0 MB (not allocated)
-----------------------------------
Total used:          48 MB
Free:                16 MB ✅
```

### Naomi Memory Requirements

```
MEM2 (64 MB available):
├── Naomi Sound RAM:  2 MB   (same)
├── Naomi VRAM:       8 MB   (same)
├── Naomi Main RAM:  32 MB   (+16 MB! 🔴)
├── VRAM buffer:     16 MB   (same)
├── Emulator code:   ~6 MB   (same)
├── Save state:       0 MB   (not allocated)
-----------------------------------
Total needed:        64 MB
Free:                 0 MB ⚠️
```

**Result: Naomi fits, but uses ALL of MEM2!**

---

## Save State Memory Requirements

### What is a Save State?

A save state is a complete snapshot of the emulated system at a moment in time, including:
- All RAM contents
- All registers
- All hardware states

### Dreamcast Save State Size

```
Component               Size      Notes
----------------------------------------
Main RAM               16 MB     Full copy
VRAM                    8 MB     Full copy
Sound RAM               2 MB     Full copy
CPU Registers          ~1 KB     SH-4 state
GPU Registers         ~64 KB     PowerVR state
Sound Registers       ~64 KB     AICA state
Other state           ~512 KB    Misc hardware
----------------------------------------
Total per state:      ~26 MB
```

### Naomi Save State Size

```
Component               Size      Notes
----------------------------------------
Main RAM               32 MB     Full copy (+16 MB!)
VRAM                    8 MB     Full copy
Sound RAM               2 MB     Full copy
CPU Registers          ~1 KB     SH-4 state
GPU Registers         ~64 KB     PowerVR state
Sound Registers       ~64 KB     AICA state
Other state           ~512 KB    Misc hardware
----------------------------------------
Total per state:      ~42 MB
```

---

## Memory Budget Scenarios

### Scenario 1: Dreamcast Only (Current)

```
MEM2 Budget: 64 MB
├── Emulation:        48 MB
├── Save state:        0 MB (on-demand)
├── Free:             16 MB ✅
-----------------------------------
Status: ✅ COMFORTABLE
Can add:
  - 1 save state slot (26 MB) ❌ Won't fit with buffer
  - OR reduce VRAM buffer to 8 MB, then 1 slot fits ✅
```

### Scenario 2: Naomi Only (No Save States)

```
MEM2 Budget: 64 MB
├── Emulation:        64 MB (exact fit!)
├── Save state:        0 MB
├── Free:              0 MB ⚠️
-----------------------------------
Status: ✅ FITS (but tight!)
Can add:
  - Save states? ❌ NO ROOM
  - Reduce VRAM buffer? Maybe 8 MB → frees 8 MB
```

### Scenario 3: Naomi + Save States (Optimized)

```
MEM2 Budget: 64 MB

Option A: Reduced VRAM buffer
├── Emulation:        56 MB (VRAM buffer: 8 MB)
├── Save state:        0 MB (stored on SD card)
├── Free:              8 MB
-----------------------------------
Status: ✅ WORKS with SD card saves

Option B: Use compression
├── Emulation:        64 MB
├── Save state:        0 MB (compress to SD: ~10-20 MB)
├── Temp buffer:       0 MB (allocate during save only)
-----------------------------------
Status: ✅ WORKS but slow save/load
```

### Scenario 4: Dreamcast + 1 Save State Slot

```
MEM2 Budget: 64 MB

Option A: Reduce VRAM buffer to 8 MB
├── DC Emulation:     40 MB (8 MB VRAM buffer)
├── Save state:       26 MB (1 slot in RAM)
├── Free:             -2 MB ❌ DOESN'T FIT

Option B: Compress save state
├── DC Emulation:     48 MB
├── Save state:       13 MB (compressed 50%)
├── Free:              3 MB ✅ TIGHT FIT

Option C: Save to SD card
├── DC Emulation:     48 MB
├── Save state:        0 MB (SD card only)
├── Free:             16 MB ✅ COMFORTABLE
```

---

## Optimization Strategies

### Strategy 1: Reduce VRAM Buffer

**Current:** 16 MB VRAM buffer (2x the actual VRAM)

**Why so large?**
- Texture conversion (compressed → uncompressed)
- Format conversion (Dreamcast formats → GX formats)
- Mipmap storage
- Double buffering

**Can we reduce it?**

```
Option A: 8 MB buffer (1x VRAM)
  - Savings: 8 MB freed
  - Risk: Some games may have texture artifacts
  - Testing: Required per-game

Option B: 4 MB buffer (0.5x VRAM)  
  - Savings: 12 MB freed
  - Risk: Higher chance of artifacts
  - May require dynamic allocation

Option C: Dynamic allocation
  - Allocate only what's needed per-game
  - Most games use < 4 MB textures
  - Best memory efficiency
```

### Strategy 2: Compress Save States

**Compression ratios for game RAM:**

| Data Type | Typical Ratio | Example |
|-----------|---------------|---------|
| Code/Instructions | 40-50% | 10 MB → 4-5 MB |
| Graphics data | 20-40% | 8 MB → 1.6-3.2 MB |
| Audio samples | 50-70% | 2 MB → 1-1.4 MB |
| Game state | 60-80% | Varies |
| **Overall** | **40-60%** | **26 MB → 10-15 MB** |

**Compression options:**

```cpp
// Option A: zlib (balanced)
compressed_size = compress_zlib(save_state, 26*MB);
// Result: ~13 MB, moderate speed

// Option B: LZ4 (fast)
compressed_size = compress_lz4(save_state, 26*MB);  
// Result: ~16 MB, very fast

// Option C: LZMA (best ratio)
compressed_size = compress_lzma(save_state, 26*MB);
// Result: ~8 MB, very slow
```

### Strategy 3: Save to SD Card

**Pros:**
- Unlimited save slots
- No RAM usage
- Persistent across sessions
- Can compress for faster writes

**Cons:**
- Slower save/load (2-5 seconds)
- SD card wear (limited writes)
- Requires SD card access

**Implementation:**

```cpp
bool SaveStateToSD(const char* slot_name) {
    // Allocate temp buffer
    u8* temp = malloc(26*MB);
    if (!temp) return false;
    
    // Copy state to temp
    memcpy(temp, mem_b.data, mem_b.size);
    memcpy(temp+16*MB, vram.data, vram.size);
    // ... copy all state
    
    // Write to SD (optionally compress)
    FILE* f = fopen(slot_name, "wb");
    fwrite(temp, 1, 26*MB, f);
    fclose(f);
    
    // Free temp buffer
    free(temp);
    return true;
}
```

### Strategy 4: Single Save Slot in RAM (Fast Access)

**For "quick save/load" during gameplay:**

```
Allocate 1 slot in RAM when user requests it:
├── Normal play:      64 MB (all for emulation)
├── User hits "save": Allocate 26 MB, make save
├── Keep in RAM:      26 MB (for instant load)
├── Can't allocate:   Fall back to SD card
```

---

## Recommended Solutions

### For Dreamcast:

#### **Solution A: SD Card Saves (RECOMMENDED)**
```
✅ Unlimited save slots
✅ No memory pressure  
✅ 16 MB free RAM for other features
⚠️ 2-5 second save/load time
```

#### **Solution B: Compressed RAM Saves**
```
✅ Fast save/load (~0.5 seconds)
✅ 1-2 save slots fit in RAM
⚠️ Uses 13-15 MB per slot
⚠️ Limited slots
```

---

### For Naomi:

#### **Solution A: SD Card Only (RECOMMENDED)**
```
✅ Naomi emulation works
✅ Unlimited save slots on SD
❌ No in-RAM saves (not enough space)
⚠️ 3-7 second save/load time (42 MB)
```

#### **Solution B: Reduce VRAM Buffer**
```
✅ Frees 8-12 MB
✅ May enable compressed RAM saves
⚠️ Needs per-game testing
⚠️ Potential texture issues
```

#### **Solution C: Hybrid Approach**
```
✅ Quick save: Last state in RAM (compressed)
✅ Full saves: SD card (multiple slots)
✅ Best of both worlds
⚠️ More complex implementation
```

---

## Memory Budget Tables

### Dreamcast - Final Budgets

| Scenario | Emulation | Save Slots | Free | Status |
|----------|-----------|------------|------|--------|
| **No saves** | 48 MB | 0 | 16 MB | ✅ Comfortable |
| **SD saves** | 48 MB | 0 (SD) | 16 MB | ✅ Best option |
| **1 compressed RAM** | 48 MB | 13 MB | 3 MB | ✅ Tight |
| **1 uncompressed RAM** | 48 MB | 26 MB | -10 MB | ❌ No fit |

### Naomi - Final Budgets

| Scenario | Emulation | Save Slots | Free | Status |
|----------|-----------|------------|------|--------|
| **No saves** | 64 MB | 0 | 0 MB | ✅ Exact fit |
| **SD saves** | 64 MB | 0 (SD) | 0 MB | ✅ Best option |
| **Reduced buffer** | 56 MB | 0 | 8 MB | ✅ For quick save |
| **Compressed RAM** | 64 MB | 42 MB | -42 MB | ❌ No fit |

---

## Code Memory Optimization

### Additional Memory Savings

Beyond the allocations we've discussed, you can save memory in:

#### 1. **Reduce BIOS allocation**
```cpp
// Current: Always allocates 2 MB
bios_b.Resize(BIOS_SIZE, false);  // 2 MB

// Optimized: Allocate actual size needed
bios_b.Resize(actual_bios_size, false);  // ~512 KB typical
// Savings: ~1.5 MB
```

#### 2. **Lazy VRAM buffer allocation**
```cpp
// Current: Always allocates 16 MB
// Optimized: Allocate based on game needs
if (game_needs_large_textures)
    alloc_vram_buffer(16*MB);
else
    alloc_vram_buffer(4*MB);  // Most games
// Savings: 12 MB for most games
```

#### 3. **Dynamic recompiler cache**
```cpp
// Current: Fixed size recompiler cache
// Optimized: Start small, grow if needed
dynarec_cache = 2*MB;  // Start
if (cache_thrashing_detected)
    expand_cache(4*MB);  // Grow
// Savings: 2-6 MB
```

---

## Practical Recommendations

### If Building Dreamcast Emulator:

1. ✅ **Use SD card for save states** (easiest, most flexible)
2. ✅ **Add optional RAM compression** (for power users)
3. ✅ **Keep VRAM buffer at 16 MB** (compatibility first)
4. ✅ **You have 16 MB free** (use for features!)

### If Building Naomi Emulator:

1. ✅ **Use SD card for save states** (only option that fits)
2. ⚠️ **Consider reducing VRAM buffer to 8 MB** (test carefully)
3. ⚠️ **May need dynamic allocation** (to squeeze in quick save)
4. ❌ **No multi-slot RAM saves** (not enough space)

### If Supporting Both:

```
Detection at runtime:
├── If Dreamcast game loaded:
│   └── Allocate 16 MB main RAM
│       └── 16 MB free for saves
│
├── If Naomi game loaded:
│   └── Allocate 32 MB main RAM
│       └── 0 MB free (SD only)
```

---

## Implementation Example

### Dynamic Memory Allocation System

```cpp
enum SystemType {
    SYSTEM_DREAMCAST,
    SYSTEM_NAOMI
};

struct MemoryConfig {
    u32 main_ram_size;
    u32 vram_size;
    u32 sound_ram_size;
    u32 vram_buffer_size;
    bool allow_ram_saves;
};

MemoryConfig GetMemoryConfig(SystemType type) {
    MemoryConfig cfg;
    cfg.vram_size = 8 * MB;
    cfg.sound_ram_size = 2 * MB;
    
    switch(type) {
        case SYSTEM_DREAMCAST:
            cfg.main_ram_size = 16 * MB;
            cfg.vram_buffer_size = 16 * MB;
            cfg.allow_ram_saves = true;  // 16 MB free
            break;
            
        case SYSTEM_NAOMI:
            cfg.main_ram_size = 32 * MB;
            cfg.vram_buffer_size = 16 * MB;
            cfg.allow_ram_saves = false; // 0 MB free
            break;
    }
    
    return cfg;
}

bool AllocateMemory(SystemType type) {
    MemoryConfig cfg = GetMemoryConfig(type);
    
    // Calculate total needed
    u32 total = cfg.main_ram_size + 
                cfg.vram_size + 
                cfg.sound_ram_size + 
                cfg.vram_buffer_size;
    
    // Check if fits
    u32 available = GetAvailableMEM2();
    if (total > available) {
        printf("ERROR: Need %d MB, only %d MB available\n",
               total/MB, available/MB);
        return false;
    }
    
    // Allocate
    u8* base = (u8*)SYS_GetArena2Lo();
    
    aica_ram.data = base;
    aica_ram.size = cfg.sound_ram_size;
    base += cfg.sound_ram_size;
    
    vram.data = base;
    vram.size = cfg.vram_size;
    base += cfg.vram_size;
    
    mem_b.data = base;
    mem_b.size = cfg.main_ram_size;
    base += cfg.main_ram_size;
    
    vram_buffer = base;
    vram_buffer_size = cfg.vram_buffer_size;
    base += cfg.vram_buffer_size;
    
    SYS_SetArena2Lo(base);
    
    printf("Allocated %s memory: %d MB\n",
           type == SYSTEM_DREAMCAST ? "Dreamcast" : "Naomi",
           total/MB);
    printf("Remaining MEM2: %d MB\n",
           ((u32)SYS_GetArena2Hi() - (u32)base)/MB);
    
    return true;
}
```

---

## Final Answer

### Can you emulate Naomi on Wii?

# ✅ **YES!**

**But with constraints:**

| Feature | Dreamcast | Naomi |
|---------|-----------|-------|
| **Emulation** | ✅ Yes (48 MB) | ✅ Yes (64 MB) |
| **In-RAM saves** | ✅ Yes (with compression) | ❌ No room |
| **SD card saves** | ✅ Yes (unlimited) | ✅ Yes (unlimited) |
| **Free RAM** | 16 MB | 0 MB |
| **Quick save/load** | ✅ Possible | ⚠️ Needs optimization |

### Recommended Configuration:

```
Dreamcast Mode:
├── Emulation: 48 MB
├── Saves: SD card (fast enough)
└── Free: 16 MB (for features)

Naomi Mode:
├── Emulation: 64 MB (all of MEM2)
├── Saves: SD card only
└── Free: 0 MB (no room for extras)

Both:
├── Use SD card as primary save method
├── Optionally: 1 compressed quick-save slot
└── Detect system type at load time
```

### You Must Choose:

**Option A:** Naomi emulation ✅ + SD card saves ✅ + No RAM saves ❌  
**Option B:** Dreamcast only ✅ + RAM saves ✅ + Extra features ✅

**Both are viable!** But Naomi uses all available RAM, so save states must go to SD card.
