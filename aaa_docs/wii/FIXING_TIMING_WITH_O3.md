# Fixing Dreamcast Timing Issues with -O3

## The Problem

**Symptom:** Dreamcast boot logo plays too fast, games run too fast
**Cause:** Your emulator is running faster, but timing isn't scaled properly

## What's Happening

### Without -O3:
```
Emulator runs at ~50 FPS
→ SH4 emulates ~100 million cycles/second
→ Timing feels correct
→ Boot logo plays at normal speed
```

### With -O3:
```
Emulator runs at ~70 FPS (40% faster!)
→ SH4 still thinks it's emulating 100 million cycles/second
→ But actually emulating ~140 million cycles/second
→ Everything runs 40% too fast!
→ Boot logo zips by quickly
```

## The Root Cause

Your emulator probably has a timing loop like this:

```cpp
// CURRENT (BROKEN WITH -O3)
void emulator_frame() {
    // Execute instructions until we've done "enough"
    int cycles = 0;
    while (cycles < CYCLES_PER_FRAME) {
        cycles += execute_one_instruction();
    }
    
    // Present frame
    flip_screen();
}
```

**Problem:** With -O3, you execute MORE instructions in the same wall-clock time, so the emulated system runs faster than real hardware!

---

## Solution 1: Frame Limiter (Quick Fix)

Add a frame rate limiter to cap at 60 FPS (or 50 for PAL):

```cpp
#include <ogc/lwp_watchdog.h>  // Wii timing functions

// Target frame time (60 FPS = 16.67ms per frame)
#define TARGET_FRAME_TIME_US 16667  // microseconds

u64 last_frame_time = 0;

void emulator_frame() {
    // Execute frame
    int cycles = 0;
    while (cycles < CYCLES_PER_FRAME) {
        cycles += execute_one_instruction();
    }
    
    // Present frame
    flip_screen();
    
    // FRAME LIMITER
    u64 current_time = gettime();
    u64 elapsed_us = ticks_to_microsecs(diff_ticks(last_frame_time, current_time));
    
    if (elapsed_us < TARGET_FRAME_TIME_US) {
        usleep(TARGET_FRAME_TIME_US - elapsed_us);
    }
    
    last_frame_time = gettime();
}
```

**Pros:** Simple, works immediately
**Cons:** Still wastes CPU if running too fast

---

## Solution 2: Accurate Cycle Counting (Better)

Make sure your cycle counting is accurate:

```cpp
// Define exact SH4 clock speed (200 MHz)
#define SH4_CLOCK_SPEED 200000000  // 200 MHz

// NTSC: 60 FPS, PAL: 50 FPS
#define TARGET_FPS 60
#define CYCLES_PER_FRAME (SH4_CLOCK_SPEED / TARGET_FPS)  // ~3,333,333 cycles

void emulator_frame() {
    u32 cycles_executed = 0;
    
    while (cycles_executed < CYCLES_PER_FRAME) {
        u16 opcode = fetch_opcode();
        
        // Execute instruction
        execute_instruction(opcode);
        
        // CRITICAL: Add accurate cycle count!
        cycles_executed += get_instruction_cycles(opcode);
    }
    
    // Update timers, interrupts, etc.
    update_hardware(cycles_executed);
    
    flip_screen();
}
```

### Accurate Cycle Counts:

```cpp
u32 get_instruction_cycles(u16 opcode) {
    // Most SH4 instructions: 1 cycle
    // Memory access: +2-3 cycles
    // Multiply: 2-4 cycles
    // Divide: 37-39 cycles
    
    // Simplified version:
    if ((opcode & 0xF000) == 0x6000) {
        // Memory loads
        return 2;
    } else if ((opcode & 0xF00F) == 0x0007) {
        // mul.l - 2-4 cycles
        return 3;
    } else if ((opcode & 0xF00F) == 0x3004) {
        // div1 - part of division
        return 1;
    }
    // ... more cases
    
    return 1;  // Default: 1 cycle
}
```

---

## Solution 3: Time-Based Execution (Best)

Instead of counting cycles, use real wall-clock time:

```cpp
#include <ogc/lwp_watchdog.h>

// How much real time = 1 emulated second
#define EMULATED_SPEED 1.0  // 1.0 = normal speed, 2.0 = 2x speed, etc.

// Wii timing
u64 emulation_start_time;
u64 last_sync_time;
u32 cycles_since_sync;

void init_timing() {
    emulation_start_time = gettime();
    last_sync_time = emulation_start_time;
    cycles_since_sync = 0;
}

void emulator_frame() {
    u64 current_time = gettime();
    
    // Calculate how many cycles we SHOULD have executed
    u64 real_time_us = ticks_to_microsecs(diff_ticks(emulation_start_time, current_time));
    u64 target_cycles = (real_time_us * SH4_CLOCK_SPEED * EMULATED_SPEED) / 1000000;
    
    // Execute until we catch up
    while (sh4_cycles_executed < target_cycles) {
        execute_one_instruction();
    }
    
    flip_screen();
}
```

**Pros:** Automatically stays in sync with real time
**Cons:** More complex, but most accurate

---

## Solution 4: V-Sync Lock (Simplest)

If you're rendering with GX, use V-Sync to limit frame rate:

```cpp
void emulator_main_loop() {
    while (running) {
        // Execute one frame worth of cycles
        for (int i = 0; i < CYCLES_PER_FRAME; i++) {
            execute_one_instruction();
        }
        
        // Render
        render_frame();
        
        // WAIT FOR V-SYNC (locks to 60 FPS automatically!)
        VIDEO_WaitVSync();
    }
}
```

**Pros:** Dead simple, leverages hardware
**Cons:** Assumes video output is 60Hz (usually is on Wii)

---

## Quick Fix for Your Code

Find your main emulation loop and add this:

```cpp
#include <ogc/lwp_watchdog.h>

// At the top of your file
#define FRAME_TIME_US 16667  // 60 FPS

// In your main loop
void sh4_run_frame() {
    static u64 last_time = 0;
    
    // Your existing frame execution
    sh4_execute_cycles(CYCLES_PER_FRAME);
    
    // NEW: Frame limiter
    if (last_time != 0) {
        u64 now = gettime();
        u64 elapsed_us = ticks_to_microsecs(diff_ticks(last_time, now));
        
        if (elapsed_us < FRAME_TIME_US) {
            usleep(FRAME_TIME_US - elapsed_us);
        }
    }
    
    last_time = gettime();
}
```

---

## Better Solution: Configurable Speed

Let users choose speed (handy for debugging too!):

```cpp
// Add to your menu/config
float emulation_speed = 1.0f;  // 1.0 = 100% speed

// In your frame loop
void emulator_frame() {
    // Adjust target based on desired speed
    u32 target_cycles = (u32)(CYCLES_PER_FRAME * emulation_speed);
    
    u32 cycles = 0;
    while (cycles < target_cycles) {
        cycles += execute_one_instruction();
    }
    
    // Frame limiter adjusted for speed
    u32 target_frame_time = (u32)(16667 / emulation_speed);
    
    // ... rest of frame limiter code
}
```

**Bonus:** Now users can:
- Run at 50% speed for difficult games
- Run at 200% speed for slow parts
- Toggle frame limiter on/off

---

## Hardware Timer Accuracy

Make sure your TMU (Timer Unit) emulation accounts for cycles:

```cpp
void update_timers(u32 cycles) {
    // TMU channels count DOWN
    for (int i = 0; i < 3; i++) {
        if (TMU_TSTR & (1 << i)) {  // Timer enabled?
            TMU_TCNT[i] -= cycles;
            
            if (TMU_TCNT[i] <= 0) {
                // Timer underflow
                TMU_TCNT[i] = TMU_TCOR[i];  // Reload
                
                // Trigger interrupt if enabled
                if (TMU_TCR[i] & 0x20) {
                    raise_timer_interrupt(i);
                }
            }
        }
    }
}
```

Call this after executing cycles:

```cpp
void emulator_frame() {
    u32 cycles = 0;
    while (cycles < CYCLES_PER_FRAME) {
        cycles += execute_one_instruction();
    }
    
    // IMPORTANT: Update hardware timers!
    update_timers(cycles);
    update_interrupts(cycles);
    // ... other hardware
}
```

---

## Common Timing Mistakes

### ❌ WRONG: Using instruction count
```cpp
// BAD - different instructions take different cycles!
for (int i = 0; i < 1000000; i++) {
    execute_one_instruction();
}
```

### ✅ CORRECT: Using cycle count
```cpp
// GOOD - accounts for actual CPU time
u32 cycles = 0;
while (cycles < 200000000 / 60) {  // 60 FPS
    cycles += execute_instruction_and_get_cycles();
}
```

---

### ❌ WRONG: No frame limiting
```cpp
// BAD - runs as fast as possible
while (1) {
    execute_frame();
    render();
}
```

### ✅ CORRECT: Frame limited
```cpp
// GOOD - maintains 60 FPS
while (1) {
    execute_frame();
    render();
    VIDEO_WaitVSync();  // or manual timer
}
```

---

## Debugging Timing Issues

### Check Frame Rate:

```cpp
void display_fps() {
    static u64 last_time = 0;
    static int frame_count = 0;
    
    frame_count++;
    
    u64 now = gettime();
    u64 elapsed = ticks_to_millisecs(diff_ticks(last_time, now));
    
    if (elapsed >= 1000) {  // Every second
        float fps = (frame_count * 1000.0f) / elapsed;
        printf("FPS: %.2f\n", fps);
        
        frame_count = 0;
        last_time = now;
    }
}
```

**Expected:** Should show ~60 FPS for NTSC, ~50 for PAL

---

### Check Cycle Count:

```cpp
void debug_cycle_timing() {
    static u64 start_time = 0;
    static u64 total_cycles = 0;
    
    if (start_time == 0) {
        start_time = gettime();
    }
    
    total_cycles += last_frame_cycles;
    
    u64 elapsed_ms = ticks_to_millisecs(diff_ticks(start_time, gettime()));
    
    if (elapsed_ms >= 1000) {
        // Should be ~200,000,000 for real hardware
        u64 cycles_per_second = (total_cycles * 1000) / elapsed_ms;
        printf("Emulated cycles/sec: %llu (target: 200000000)\n", cycles_per_second);
        
        total_cycles = 0;
        start_time = gettime();
    }
}
```

---

## Real-World Example: Boot Logo Timing

The Dreamcast boot sequence has specific timing:

```cpp
void boot_sequence() {
    // Logo should take approximately:
    // - SEGA logo: ~2 seconds
    // - Swirl animation: ~3 seconds
    // - License text: ~2 seconds
    // Total: ~7 seconds
    
    u64 start_time = gettime();
    
    while (boot_not_complete) {
        emulator_frame();
        
        // Check if we're running too fast
        u64 elapsed = ticks_to_millisecs(diff_ticks(start_time, gettime()));
        if (boot_complete && elapsed < 5000) {
            printf("WARNING: Boot sequence too fast! (%llu ms)\n", elapsed);
        }
    }
}
```

---

## Recommended Fix for Your Emulator

**Step 1:** Add frame limiter (5 minutes):
```cpp
// Add to your main loop
VIDEO_WaitVSync();
```

**Step 2:** Verify cycle counting (30 minutes):
```cpp
// Make sure each instruction adds correct cycles
// Most instructions: 1 cycle
// Memory access: +1-2 cycles
```

**Step 3:** Test with real games (important!):
- Boot logo should take ~7 seconds
- Games should run at normal speed
- Sound should not be too fast/slow

---

## Alternative: Make Speed Adjustable

Add to your menu:

```cpp
enum SpeedMode {
    SPEED_50,   // 50% (slow motion)
    SPEED_100,  // 100% (normal)
    SPEED_150,  // 150% (fast)
    SPEED_200,  // 200% (very fast)
    SPEED_MAX   // Unlimited (no frame limiter)
};

SpeedMode current_speed = SPEED_100;

u32 get_target_frame_time() {
    switch (current_speed) {
        case SPEED_50:  return 33333;  // 30 FPS
        case SPEED_100: return 16667;  // 60 FPS
        case SPEED_150: return 11111;  // 90 FPS
        case SPEED_200: return 8333;   // 120 FPS
        case SPEED_MAX: return 0;      // No limit
    }
    return 16667;
}
```

This lets users:
- Run at normal speed for accurate emulation
- Speed up for grinding/slow sections
- Slow down for difficult sections

---

## Summary

**Your -O3 optimization is GOOD and SAFE!**

The problem is **timing**, not accuracy. The emulator is running the SH4 code correctly, just too fast!

**Quick fix:**
```cpp
VIDEO_WaitVSync();  // Add to your main loop
```

**Better fix:**
```cpp
// Proper frame limiter with cycle counting
while (cycles < CYCLES_PER_FRAME) {
    cycles += execute_instruction();
}
usleep(remaining_time);
```

**Best fix:**
```cpp
// Time-based execution that automatically syncs to real time
// See "Solution 3" above
```

Keep -O3! Just add proper timing control! 🎮⏱️
