# Emulator Architecture Guide: Branching Strategy and Performance Optimization

## Table of Contents
1. [Introduction](#introduction)
2. [Single Branch vs Multiple Branches](#single-branch-vs-multiple-branches)
3. [Recommended Architecture](#recommended-architecture)
4. [Implementation Strategies](#implementation-strategies)
5. [Configuration System Design](#configuration-system-design)
6. [Performance vs Accuracy Trade-offs](#performance-vs-accuracy-trade-offs)
7. [Real-World Examples](#real-world-examples)
8. [Best Practices](#best-practices)
9. [Wii-Specific Considerations](#wii-specific-considerations)

---

## Introduction

When developing an emulator, especially one targeting resource-constrained platforms like the Nintendo Wii, developers face a critical decision: should they maintain separate codebases for performance and accuracy, or use a single codebase with runtime configuration?

This guide provides a comprehensive analysis of both approaches and recommends the optimal strategy for emulator development.

---

## Single Branch vs Multiple Branches

### Single Branch with Runtime Configuration ✅ **RECOMMENDED**

**Description**: One codebase with configurable rendering modes and optimization levels that can be switched at runtime.

#### Advantages

| Benefit | Description |
|---------|-------------|
| **Unified Maintenance** | Bug fixes and improvements apply to all modes automatically |
| **Reduced Code Duplication** | Shared infrastructure means less code to maintain |
| **User Flexibility** | Users can switch modes per-game without reinstalling |
| **Easier Testing** | Single binary to test and distribute |
| **Gradual Optimization** | Improvements can be made incrementally without breaking existing functionality |
| **Shared Infrastructure** | Texture cache, memory management, etc. benefit all modes |

#### Disadvantages

| Challenge | Mitigation |
|-----------|------------|
| Code complexity with many `if` statements | Use strategy pattern and polymorphism |
| Potential performance overhead from checks | Compiler optimization and branch prediction handle this well |
| Configuration explosion | Use sensible presets and auto-detection |

### Multiple Branches ❌ **NOT RECOMMENDED**

**Description**: Separate `performance` and `accuracy` branches with diverging codebases.

#### Disadvantages

| Issue | Impact |
|-------|--------|
| **Maintenance Burden** | Every bug fix needs to be applied to multiple branches |
| **Code Drift** | Branches diverge over time, making merging impossible |
| **Duplicate Work** | Features must be implemented multiple times |
| **User Confusion** | Which version should users download? |
| **Testing Nightmare** | Need to test multiple builds for each release |
| **Git History Pollution** | Merge conflicts and tangled history |

#### When It MIGHT Make Sense

- **Complete architecture rewrite** (e.g., moving from software to hardware renderer)
- **Experimental features** that may never merge (use feature branches instead)
- **Platform-specific ports** with completely different APIs (e.g., Wii vs Android)
- **Major API changes** (e.g., OpenGL to Vulkan) during transition period

---

## Recommended Architecture

### Three-Tier Configuration System

```
┌─────────────────────────────────────────┐
│         Emulation Core (Shared)         │
│  - CPU Emulation                        │
│  - Memory Management                    │
│  - Bus Timing                           │
│  - Save States                          │
└─────────────────────────────────────────┘
                    │
        ┌───────────┴───────────┐
        │                       │
┌───────▼────────┐    ┌────────▼────────┐
│  Fast Renderer │    │ Accurate Render │
│  - Culling     │    │ - Full Pipeline │
│  - Simplified  │    │ - All Effects   │
│  - Hacks OK    │    │ - Cycle Exact   │
└────────────────┘    └─────────────────┘
```

### Rendering Mode Hierarchy

```cpp
enum RenderMode {
    FAST,       // Maximum performance, visual compromises OK
    BALANCED,   // Good balance of speed and accuracy
    ACCURATE,   // Prioritize correctness over speed
    AUTO        // Per-game profile selection
};

enum AccuracyLevel {
    LOW,        // Major shortcuts, ~200% speed
    MEDIUM,     // Minor shortcuts, ~150% speed
    HIGH,       // Most features, ~100% speed
    PERFECT     // Cycle-accurate, ~50% speed
};
```

---

## Implementation Strategies

### Strategy Pattern for Rendering Paths

```cpp
// Renderer interface
class IRenderer {
public:
    virtual void RenderPolygon(PolyParam* poly, Vertex* vertices) = 0;
    virtual void RenderModifierVolume(ModVolParam* vol) = 0;
    virtual void SetupFramebuffer() = 0;
    virtual ~IRenderer() = default;
};

// Fast renderer implementation
class FastRenderer : public IRenderer {
public:
    void RenderPolygon(PolyParam* poly, Vertex* vertices) override {
        // Skip expensive operations
        if (poly->isp.Texture && !settings.enable_textures)
            return;
        
        // Simplified rendering
        DrawPolygonFast(poly, vertices);
    }
    
    void RenderModifierVolume(ModVolParam* vol) override {
        // Skip modifier volumes entirely in fast mode
        return;
    }
};

// Accurate renderer implementation  
class AccurateRenderer : public IRenderer {
public:
    void RenderPolygon(PolyParam* poly, Vertex* vertices) override {
        // Full pipeline emulation
        SetupBlending(poly);
        SetupDepthTest(poly);
        SetupTextures(poly);
        DrawPolygonAccurate(poly, vertices);
    }
    
    void RenderModifierVolume(ModVolParam* vol) override {
        // Proper modifier volume rendering
        StencilSetup();
        DrawModifierVolume(vol);
    }
};

// Factory pattern for creation
IRenderer* CreateRenderer(RenderMode mode) {
    switch (mode) {
        case FAST:     return new FastRenderer();
        case ACCURATE: return new AccurateRenderer();
        case BALANCED: return new BalancedRenderer();
        default:       return new AccurateRenderer();
    }
}
```

### Conditional Compilation for Platform-Specific Code

```cpp
#ifdef WII_PLATFORM
    #define USE_GX_RENDERER
    #define MAX_VERTICES (42 * 1024)  // Wii memory limit
#else
    #define USE_OPENGL_RENDERER
    #define MAX_VERTICES (256 * 1024) // PC can handle more
#endif

// Runtime switching within platform
void DoRender() {
    #ifdef USE_GX_RENDERER
        if (settings.mode == FAST) {
            DoRenderGX_Fast();
        } else {
            DoRenderGX_Accurate();
        }
    #endif
}
```

### Feature Flags for Gradual Rollout

```cpp
struct RenderFeatures {
    bool modifier_volumes;
    bool alpha_sorting;
    bool per_pixel_lighting;
    bool high_res_textures;
    bool anisotropic_filtering;
    bool fog_emulation;
    bool bump_mapping;
};

// Preset configurations
static const RenderFeatures FAST_PRESET = {
    .modifier_volumes = false,
    .alpha_sorting = false,
    .per_pixel_lighting = false,
    .high_res_textures = false,
    .anisotropic_filtering = false,
    .fog_emulation = false,
    .bump_mapping = false
};

static const RenderFeatures ACCURATE_PRESET = {
    .modifier_volumes = true,
    .alpha_sorting = true,
    .per_pixel_lighting = true,
    .high_res_textures = false, // Still too expensive on Wii
    .anisotropic_filtering = true,
    .fog_emulation = true,
    .bump_mapping = true
};
```

---

## Configuration System Design

### Hierarchical Settings Structure

```cpp
struct EmulationSettings {
    // Top-level mode selector
    RenderMode render_mode;
    AccuracyLevel accuracy_level;
    
    // Performance settings
    struct Performance {
        bool frameskip;
        int max_frameskip;
        bool dynamic_resolution;
        bool async_shaders;
    } performance;
    
    // Graphics settings
    struct Graphics {
        bool modifier_volumes;
        bool alpha_sorting;
        int internal_resolution;  // 1x, 2x, etc.
        bool widescreen_hack;
        bool texture_filtering;
    } graphics;
    
    // Accuracy settings
    struct Accuracy {
        bool cycle_accurate_timing;
        bool accurate_depth_buffer;
        bool accurate_fog;
        bool accurate_palette;
    } accuracy;
    
    // Per-game overrides
    struct GameProfile {
        char game_id[16];
        RenderMode override_mode;
        RenderFeatures features;
    } game_profile;
};
```

### Auto-Detection System

```cpp
class GameProfileManager {
private:
    std::map<std::string, GameProfile> profiles;
    
public:
    void LoadProfiles() {
        // Load from database
        profiles["T1234E"] = {  // Example game ID
            .render_mode = FAST,
            .features = {
                .modifier_volumes = false,  // Game doesn't use them
                .alpha_sorting = true,      // Game needs this
                .fog_emulation = false      // Game doesn't use fog
            }
        };
    }
    
    GameProfile* GetProfile(const char* game_id) {
        auto it = profiles.find(game_id);
        return it != profiles.end() ? &it->second : nullptr;
    }
    
    void ApplyProfile(const char* game_id) {
        auto* profile = GetProfile(game_id);
        if (profile) {
            settings.render_mode = profile->render_mode;
            settings.graphics = profile->features;
            printf("Applied profile for %s\n", game_id);
        }
    }
};
```

### Configuration File Format

```ini
[Global]
RenderMode=Auto
AccuracyLevel=Medium
ShowFPS=true

[Performance]
Frameskip=false
DynamicResolution=true
AsyncShaders=false

[Graphics]
ModifierVolumes=true
AlphaSorting=true
InternalResolution=1
WidescreenHack=false

[Game:T1234E]  # Crazy Taxi
RenderMode=Fast
ModifierVolumes=false
AlphaSorting=false
Comment=This game runs fast without these

[Game:T5678E]  # Shenmue
RenderMode=Accurate
ModifierVolumes=true
AlphaSorting=true
Comment=Needs accuracy for correct rendering
```

---

## Performance vs Accuracy Trade-offs

### Feature Impact Matrix

| Feature | Performance Impact | Visual Impact | Recommend For |
|---------|-------------------|---------------|---------------|
| **Modifier Volumes** | -30% FPS | Shadows, reflections | Accurate mode only |
| **Alpha Sorting** | -15% FPS | Transparency order | All modes |
| **Per-Pixel Lighting** | -20% FPS | Specular highlights | Balanced+ |
| **Fog Emulation** | -5% FPS | Depth fog effects | All modes |
| **High-Res Textures** | -25% FPS | Texture clarity | Disabled on Wii |
| **Anisotropic Filtering** | -10% FPS | Texture quality | Balanced+ |
| **Proper Depth Buffer** | -5% FPS | Z-fighting fixes | All modes |
| **Cycle-Accurate Timing** | -40% FPS | Animation accuracy | Accurate mode only |

### Performance Budget Allocation

For a 30 FPS target on Wii (33.3ms per frame):

```
Fast Mode Budget:
├─ CPU Emulation:      10ms (30%)
├─ Rendering:          15ms (45%)
├─ Audio:               5ms (15%)
└─ Overhead:            3ms (10%)
Total: ~30 FPS achievable

Accurate Mode Budget:
├─ CPU Emulation:      12ms (36%)
├─ Rendering:          20ms (60%)
├─ Audio:               5ms (15%)
└─ Overhead:            3ms (10%)
Total: ~24 FPS (acceptable for accuracy)
```

### Optimization Hierarchy

Priority order for optimizations:

1. **Critical Path** (Always optimize)
   - Vertex transformation
   - Texture upload
   - Framebuffer copy

2. **High-Return** (Optimize in all modes)
   - Texture caching
   - Display list compilation
   - Memory management

3. **Medium-Return** (Optimize in fast mode)
   - Polygon culling
   - LOD selection
   - Simplified shading

4. **Low-Return** (Skip in fast mode)
   - Modifier volumes
   - Perfect alpha sorting
   - Cycle-accurate timing

---

## Real-World Examples

### Dolphin Emulator

**Approach**: Single codebase with extensive configuration

```
Graphics Settings:
├─ Backend Selection (OpenGL/Vulkan/D3D)
├─ Enhancement Tab
│   ├─ Internal Resolution (1x-8x)
│   ├─ Anisotropic Filtering
│   ├─ Anti-Aliasing
│   └─ Post-Processing
├─ Hacks Tab
│   ├─ Skip EFB Access
│   ├─ Ignore Format Changes
│   └─ Fast Depth Calculation
└─ Advanced Tab
    ├─ Accuracy Options
    ├─ Debugging Features
    └─ Experimental Options
```

**Key Insight**: Users love having control. Presets (Fast/Balanced/Accurate) guide beginners while experts can fine-tune.

### PCSX2 (PlayStation 2 Emulator)

**Approach**: Speedhacks panel with sliders

```
Speed Hacks:
├─ EE Cyclerate:       [Slider: -3 to +3]
├─ VU Cycle Stealing:  [Slider: 0 to 3]
├─ MTVU (Multi-Thread VU1)
├─ Instant VU1
└─ Enable INTC Spin Detection

Game Fixes:
├─ FMV Aspect Ratio
├─ Skip MPEG Hack
└─ OPH Flag Hack
```

**Key Insight**: Sliders let users find the perfect balance for their system. Game-specific fixes in a database.

### RetroArch

**Approach**: Core options per emulator

```
Core Options (Flycast - Dreamcast):
├─ CPU Dynarec:        [Enabled/Disabled]
├─ Internal Resolution: [640x480/1280x960/1920x1440]
├─ Widescreen:         [On/Off]
├─ Skip Frame:         [Disabled/1/2/3]
└─ Threaded Rendering: [On/Off]
```

**Key Insight**: Standardized UI across all cores. Users learn once, apply everywhere.

---

## Best Practices

### 1. Start Accurate, Optimize Later

```cpp
// ❌ Wrong approach
void RenderPolygon() {
    // Implement fast hack first
    if (settings.fast_mode) {
        FastRender();
    } else {
        // TODO: Implement accurate version later
    }
}

// ✅ Correct approach
void RenderPolygon() {
    // Implement accurate version first
    AccurateRender();
    
    // Add fast path as optimization
    if (settings.fast_mode && CanUseFastPath()) {
        FastRender();
        return;
    }
    
    AccurateRender();
}
```

### 2. Use Sensible Defaults

```cpp
EmulationSettings GetDefaultSettings() {
    EmulationSettings defaults;
    
    #ifdef WII_PLATFORM
        // Wii defaults to Fast mode
        defaults.render_mode = FAST;
        defaults.accuracy_level = MEDIUM;
    #else
        // PC defaults to Accurate mode
        defaults.render_mode = ACCURATE;
        defaults.accuracy_level = HIGH;
    #endif
    
    return defaults;
}
```

### 3. Document Trade-offs

```cpp
struct SettingInfo {
    const char* name;
    const char* description;
    const char* performance_impact;
    const char* visual_impact;
};

static const SettingInfo MODIFIER_VOLUMES_INFO = {
    .name = "Modifier Volumes",
    .description = "Enables shadow and reflection effects",
    .performance_impact = "High (-30% FPS)",
    .visual_impact = "Significant - required for some games"
};
```

### 4. Profile-Guided Optimization

```cpp
struct PerformanceMetrics {
    float avg_frame_time;
    float render_time;
    float cpu_time;
    int dropped_frames;
};

void AutoAdjustSettings() {
    PerformanceMetrics metrics = GetMetrics();
    
    if (metrics.avg_frame_time > 40.0f) {  // Below 25 FPS
        // Automatically reduce quality
        if (settings.graphics.modifier_volumes) {
            settings.graphics.modifier_volumes = false;
            printf("Auto-disabled modifier volumes for performance\n");
        }
    }
}
```

### 5. Version Control Strategy

```
main
├─ feature/new-audio-system
├─ feature/vulkan-renderer
├─ bugfix/texture-corruption
└─ experimental/recompiler-v2

NOT:
main
├─ performance-branch
└─ accuracy-branch  ❌
```

**Use feature branches, not mode branches!**

---

## Wii-Specific Considerations

### Hardware Constraints

| Resource | Limit | Implication |
|----------|-------|-------------|
| **CPU** | 729 MHz PowerPC | Need CPU-efficient algorithms |
| **RAM** | 88 MB total (24MB MEM1 + 64MB MEM2) | Tight memory budget |
| **GPU** | Fixed-function GX | No programmable shaders |
| **Storage** | SD card only | Slow I/O, need caching |

### Recommended Settings for Wii

```cpp
// Wii-optimized defaults
struct WiiOptimizedSettings {
    // Must be disabled on Wii
    bool high_res_textures = false;        // Not enough memory
    bool programmable_shaders = false;     // GX doesn't support
    int internal_resolution = 1;           // 1x native only
    
    // Should be disabled for performance
    bool modifier_volumes = false;         // Too expensive
    bool cycle_accurate = false;           // Can't maintain 30 FPS
    
    // Can be enabled
    bool alpha_sorting = true;             // Manageable cost
    bool texture_filtering = true;         // GX handles this
    bool fog_emulation = true;             // Cheap on GX
    
    // Wii-specific optimizations
    bool use_gx_cache = true;              // Use GX texture cache
    bool async_texture_load = true;        // Load during VBlank
    bool display_list_cache = true;        // Cache GX commands
};
```

### Performance Profile Examples

#### Fast Profile (Targeting 30+ FPS)
```cpp
RenderMode: FAST
Features:
  - modifier_volumes: false
  - alpha_sorting: false (sort by depth only)
  - per_pixel_lighting: false
  - fog_emulation: false
  - texture_filtering: bilinear only
Performance Target: 30-60 FPS
Visual Quality: 70%
```

#### Balanced Profile (Targeting ~30 FPS)
```cpp
RenderMode: BALANCED
Features:
  - modifier_volumes: false
  - alpha_sorting: true
  - per_pixel_lighting: false
  - fog_emulation: true
  - texture_filtering: bilinear + some anisotropic
Performance Target: 25-30 FPS
Visual Quality: 85%
```

#### Accurate Profile (Targeting 20-25 FPS)
```cpp
RenderMode: ACCURATE
Features:
  - modifier_volumes: true
  - alpha_sorting: true (proper order)
  - per_pixel_lighting: true
  - fog_emulation: true
  - texture_filtering: full anisotropic
Performance Target: 20-25 FPS (acceptable for turn-based games)
Visual Quality: 95%
```

### Per-Game Auto Profiles

```cpp
// Games that work great in Fast mode
static const char* FAST_MODE_GAMES[] = {
    "T1234E",  // Crazy Taxi (simple graphics)
    "T2468E",  // Sonic Adventure (fast paced, FPS matters)
    "T1357E",  // Jet Grind Radio (cell shaded, simple)
};

// Games that need Accurate mode
static const char* ACCURATE_MODE_GAMES[] = {
    "T9876E",  // Shenmue (complex lighting)
    "T5432E",  // Soul Calibur (modifier volumes for shadows)
    "T8765E",  // Skies of Arcadia (fog effects critical)
};
```

---

## Summary

### Quick Decision Matrix

| Scenario | Recommendation |
|----------|----------------|
| **Starting a new emulator** | Single branch with Fast/Accurate modes |
| **Mature emulator needing optimization** | Add performance presets to existing code |
| **Experimental renderer rewrite** | Feature branch, merge when stable |
| **Platform-specific port** | Separate repository OK, share core emulation |
| **Resource-constrained target** | Essential to have Fast mode option |

### Implementation Checklist

- [ ] Define RenderMode enum (Fast/Balanced/Accurate/Auto)
- [ ] Create settings structure with hierarchical organization
- [ ] Implement feature flags for individual options
- [ ] Build preset configurations for each mode
- [ ] Add per-game profile system
- [ ] Create configuration file parser
- [ ] Add auto-detection for hardware capabilities
- [ ] Implement performance monitoring
- [ ] Document trade-offs for each setting
- [ ] Create user-facing preset descriptions
- [ ] Add in-game OSD for performance stats
- [ ] Build game compatibility database

### Key Takeaways

1. **Single Branch + Runtime Config** is almost always the right choice
2. **Start accurate, optimize later** - correctness first
3. **Presets are essential** - Fast/Balanced/Accurate guide users
4. **Per-game profiles** - different games need different settings
5. **Document everything** - users need to understand trade-offs
6. **Profile performance** - data-driven optimization decisions
7. **Gradual rollout** - use feature flags for new optimizations

---

## Conclusion

For your Wii Dreamcast emulator specifically, the recommended approach is:

✅ **Single codebase** with three rendering modes (Fast, Balanced, Accurate)

✅ **Runtime configuration** allowing per-game settings

✅ **Auto-detection** of optimal settings based on game database

✅ **Performance monitoring** with automatic quality adjustment

✅ **Clear presets** that users can understand

This approach provides the best balance of:
- **Maintainability** - one codebase to maintain
- **Flexibility** - users can tune for their preferences
- **Performance** - optimizations benefit everyone
- **Accuracy** - games that need it can enable it

Remember: **Users love options, but hate complexity.** Provide good defaults, clear presets, and the ability to fine-tune. That's the recipe for a successful emulator!

---

## Additional Resources

- [Dolphin Emulator Architecture](https://dolphin-emu.org/docs/)
- [PCSX2 Configuration Guide](https://pcsx2.net/config-guide/)
- [RetroArch Documentation](https://docs.libretro.com/)
- [Emulation Accuracy Research](https://www.obscuregamers.com/threads/emulation-accuracy.589/)

---

**Document Version**: 1.0  
**Last Updated**: February 2026  
**Author**: Architecture Guide for Dreamcast-on-Wii Emulator
