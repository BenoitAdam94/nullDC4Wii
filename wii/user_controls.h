#pragma once

/*
    user_controls.h - fully user-remappable Dreamcast control layout

    user_controls.cfg lives next to boot.dol or in the games folder, same
    lookup order as game_presets.cfg (see loadUserControls() in wii/main.cpp).
    Selected from the CONTROLS menu's SPECIAL LAYOUT row (USER CFG entry,
    see main.cpp g_special_layout_preset / SPECIAL_LAYOUT_USER_CFG) —
    everything below is a no-op while any other layout is selected.

    See apps/discs/user_controls.cfg for the file format and the full list
    of remappable targets/sources.
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Parse the config file into memory. Safe to call even if the file doesn't
 * exist (prints a warning; user_controls_loaded() then stays false and the
 * USER CFG layout falls back to the legacy per-device mapping).
 * @param path Full path e.g. "sd:/discs/user_controls.cfg"
 */
void user_controls_load(const char* path);

/** True once a file with at least one recognized section was parsed. */
int user_controls_loaded(void);

#ifdef __cplusplus
}

// C++ evaluator API, consumed by plugs/drkMapleDevices/drkMapleDevices.cpp.
// Relies on the includer having already pulled in <gccore.h> (u8/u16/u32/s8/
// s32) — same convention as the rest of this plugin's headers.

/**
 * Evaluate the parsed mapping for one controller port's current raw input
 * and write the result in the same format drkMapleDevices.cpp's own
 * MapButtons()/MapAnalogStick()/MapTriggers() produce.
 * @param gcStickX/Y     GameCube main analog stick (PAD_StickX/Y)
 * @param gcSubStickX/Y   GameCube C-stick (PAD_SubStickX/Y) -- digital taps
 *                        only, for the c_stick_* sources
 * @param expStickX/Y    Wiimote expansion stick, already resolved to
 *                        whichever of Nunchuk/Classic Controller is attached
 *                        (same values UpdateInputState() feeds MapAnalogStick())
 */
void UserControls_Update(u32 wiiButtons, u32 gcButtons,
                          u32 nunchuckButtons, u32 classicButtons,
                          s32 gcStickX, s32 gcStickY,
                          s32 gcSubStickX, s32 gcSubStickY,
                          s32 expStickX, s32 expStickY,
                          u16* outKcode, s8* outJoyX, s8* outJoyY,
                          u8* outLt, u8* outRt);

/**
 * True if this port's raw input currently satisfies one of the file's
 * exit_combo lines. Checked in ADDITION to the Wiimote MINUS+PLUS combo,
 * which always exits regardless of this file's contents.
 */
bool UserControls_CheckExitCombo(u32 wiiButtons, u32 gcButtons,
                                  u32 nunchuckButtons, u32 classicButtons);
#endif
