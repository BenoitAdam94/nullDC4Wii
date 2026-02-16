// drkPvr.cpp : Defines the entry point for the DLL application.
//

/*

Future Enhancement Opportunities
If you want to go further (these WOULD break compatibility with types.h, ta_structs.h):
Moderate Breaking Changes:

Replace globals with a context struct
Use smart pointers (if C++11 available)
Add debug/release separate config paths

Aggressive Optimizations:

SIMD vectorization for vertex processing
Memory pool allocator for TA structures
Cache-aligned structures for Wii Broadway CPU
Multithreading for tile rendering

*/

#include "drkPvr.h"
#include "ta.h"
#include "spg.h"
#include "regs.h"
#include "Renderer_if.h"
#include "config/config.h"
#include <algorithm>

// Configuration constants
#define params PVRPARAMS

// Global state
wchar emu_name[512];
pvr_init_params params;
_settings_type settings;

// Feature flags (currently unused but reserved for future use)
u32 enable_FS_mid = 0;
u32 AA_mid_menu = 0;
u32 AA_mid_0 = 0;

/**
 * VRAM lock callback - called when VRAM is locked for write
 * @param block The VRAM block being locked
 * @param addr  The address being written to
 * 
 * Currently disabled but can be enabled for texture cache invalidation
 */
void FASTCALL libPvr_vramLockCB(vram_block* block, u32 addr)
{
    // Currently disabled - can be enabled for texture invalidation
    // rend_if_vram_locked_write(block, addr);
    // renderer->VramLockedWrite(block);
    // rend_text_invl(block);
}

/**
 * Plugin load - called when plugin is first loaded by emulator
 * Performs one-time initialization
 * @return rv_ok on success, rv_error on failure
 */
s32 FASTCALL libPvr_Load()
{
    LoadSettings();
    printf("drkpvr: Using %s\n", REND_NAME);
    return rv_ok;
}

/**
 * Plugin unload - cleanup when plugin is unloaded
 * Only called if libPvr_Init was successfully called
 */
void FASTCALL libPvr_Unload()
{
    // Nothing to cleanup currently
}

/**
 * Reset PVR state (preserves VRAM which is reset by emulator)
 * @param Manual True if manually triggered reset, false if automatic
 */
void FASTCALL libPvr_Reset(bool Manual)
{
    Regs_Reset(Manual);
    spg_Reset(Manual);
    rend_reset(Manual);
}

/**
 * Initialize PVR subsystem - called when entering SH4 thread
 * Called from the new thread context for thread-specific initialization
 * @param param Initialization parameters including VRAM pointer
 * @return rv_ok on success, rv_error on failure
 */
s32 FASTCALL libPvr_Init(pvr_init_params* param)
{
    // Store initialization parameters
    memcpy(&params, param, sizeof(params));

    // Initialize subsystems in order, with proper cleanup on failure
    if (!Regs_Init())
    {
        printf("drkpvr: Failed to initialize registers\n");
        return rv_error;
    }
    
    if (!spg_Init())
    {
        printf("drkpvr: Failed to initialize SPG\n");
        Regs_Term();  // Cleanup on failure
        return rv_error;
    }
    
    if (!rend_init())
    {
        printf("drkpvr: Failed to initialize renderer\n");
        spg_Term();   // Cleanup on failure
        Regs_Term();
        return rv_error;
    }

    // Start renderer thread (only the renderer needs thread-specific initialization)
    if (!rend_thread_start())
    {
        printf("drkpvr: Failed to start render thread\n");
        rend_term();  // Cleanup on failure
        spg_Term();
        Regs_Term();
        return rv_error;
    }

    return rv_ok;
}

/**
 * Terminate PVR subsystem - called when exiting SH4 thread
 * Called from the thread context for thread-specific cleanup
 */
void FASTCALL libPvr_Term()
{
    // Cleanup in reverse order of initialization
    rend_thread_end();
    rend_term();
    spg_Term();
    Regs_Term();
}

// Forward declaration for utility function
char* GetNullDCSoruceFileName(char* full);

/**
 * Get integer configuration value
 * @param key Configuration key name
 * @param def Default value if key not found
 * @return Configuration value or default
 */
int cfgGetInt(const char* key, int def)
{
    return cfgLoadInt("drkpvr", key, def);
}

/**
 * Set integer configuration value
 * @param key Configuration key name
 * @param val Value to store
 */
void cfgSetInt(const char* key, int val)
{
    cfgSaveInt("drkpvr", key, val);
}

/**
 * Load all settings from configuration system
 * Applies reasonable defaults if settings don't exist
 */
void LoadSettings()
{
    // Emulation settings - affect accuracy and compatibility
    settings.Emulation.AlphaSortMode    = cfgGetInt("Emulation.AlphaSortMode", 1);
    settings.Emulation.PaletteMode      = cfgGetInt("Emulation.PaletteMode", 1);
    settings.Emulation.ModVolMode       = cfgGetInt("Emulation.ModVolMode", 1);
    settings.Emulation.ZBufferMode      = cfgGetInt("Emulation.ZBufferMode", 0);

    // OSD settings - display overlays
    settings.OSD.ShowFPS                = cfgGetInt("OSD.ShowFPS", 0);
    settings.OSD.ShowStats              = cfgGetInt("OSD.ShowStats", 0);

    // Fullscreen settings - defaults to auto-detect (-1)
    settings.Fullscreen.Enabled         = cfgGetInt("Fullscreen.Enabled", 0);
    settings.Fullscreen.Res_X           = cfgGetInt("Fullscreen.Res_X", -1);
    settings.Fullscreen.Res_Y           = cfgGetInt("Fullscreen.Res_Y", -1);
    settings.Fullscreen.Refresh_Rate    = cfgGetInt("Fullscreen.Refresh_Rate", -1);

    // Enhancement settings - visual improvements
    settings.Enhancements.MultiSampleCount   = cfgGetInt("Enhancements.MultiSampleCount", 0);
    settings.Enhancements.MultiSampleQuality = cfgGetInt("Enhancements.MultiSampleQuality", 0);
    settings.Enhancements.AspectRatioMode    = cfgGetInt("Enhancements.AspectRatioMode", 1);
}

/**
 * Save all settings to configuration system
 * Called when user changes settings or on clean shutdown
 */
void SaveSettings()
{
    // Emulation settings
    cfgSetInt("Emulation.AlphaSortMode", settings.Emulation.AlphaSortMode);
    cfgSetInt("Emulation.PaletteMode", settings.Emulation.PaletteMode);
    cfgSetInt("Emulation.ModVolMode", settings.Emulation.ModVolMode);
    cfgSetInt("Emulation.ZBufferMode", settings.Emulation.ZBufferMode);

    // OSD settings
    cfgSetInt("OSD.ShowFPS", settings.OSD.ShowFPS);
    cfgSetInt("OSD.ShowStats", settings.OSD.ShowStats);

    // Fullscreen settings
    cfgSetInt("Fullscreen.Enabled", settings.Fullscreen.Enabled);
    cfgSetInt("Fullscreen.Res_X", settings.Fullscreen.Res_X);
    cfgSetInt("Fullscreen.Res_Y", settings.Fullscreen.Res_Y);
    cfgSetInt("Fullscreen.Refresh_Rate", settings.Fullscreen.Refresh_Rate);

    // Enhancement settings
    cfgSetInt("Enhancements.MultiSampleCount", settings.Enhancements.MultiSampleCount);
    cfgSetInt("Enhancements.MultiSampleQuality", settings.Enhancements.MultiSampleQuality);
    cfgSetInt("Enhancements.AspectRatioMode", settings.Enhancements.AspectRatioMode);
}
