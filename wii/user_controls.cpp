// user_controls.cpp - fully user-remappable Dreamcast control layout
//
// Parses user_controls.cfg (two fixed sections, [Dreamcast_Wiimote] and
// [Dreamcast_Gamecube]) into a small in-memory table, then evaluates that
// table every frame the USER CFG special layout is active (see
// drkMapleDevices.cpp UpdateInputState()). See apps/discs/user_controls.cfg
// for the file format.

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "wii/user_controls.h" // must come after gccore.h/wpad.h -- its C++
                                // section uses u8/u16/u32/s8/s32 without
                                // including gctypes.h itself (see its own
                                // header comment)
#include "plugs/drkMapleDevices/dc_pad_bits.h"

namespace {

// Dreamcast targets that behave as a simple OR-set of physical sources
// (repeat the same key on several lines to add more sources).
enum UcTarget { UC_A = 0, UC_B, UC_X, UC_Y, UC_START, UC_TRIGGER_L, UC_TRIGGER_R, UC_Z, UC_BTN_COUNT };

// L/D both target the same bit (Dreamcast's digital left-trigger mirror);
// R/C both target key_CONT_C (digital right-trigger mirror) -- see the
// header comment in apps/discs/user_controls.cfg for why two spellings.
const u16 kTargetBit[UC_BTN_COUNT] =
{
    key_CONT_A, key_CONT_B, key_CONT_X, key_CONT_Y, key_CONT_START,
    key_CONT_D, key_CONT_C, key_CONT_Z
};

// GameCube C-stick digital taps -- not a maple bit, just this module's own
// bookkeeping for the c_stick_* sources.
#define CSTICK_UP     (1 << 0)
#define CSTICK_DOWN   (1 << 1)
#define CSTICK_LEFT   (1 << 2)
#define CSTICK_RIGHT  (1 << 3)
#define CSTICK_THRESHOLD 45

#define USER_ANALOG_DEADZONE 20 // mirrors drkMapleDevices.cpp's ANALOG_DEADZONE

struct WiimoteSection
{
    bool defined;
    u32  wii[UC_BTN_COUNT];
    u32  nunchuk[UC_BTN_COUNT];
    u32  classic[UC_BTN_COUNT];
    int  dpadMode;   // 0=none, 1=this section's own d-pad (wiimote + classic)
    int  stickMode;  // 0=none, 1=nunchuk/classic expansion stick
    u32  exitWii, exitNunchuk, exitClassic; // AND-mask, all must be held
};

struct GamecubeSection
{
    bool defined;
    u32 gc[UC_BTN_COUNT];
    u8  cstick[UC_BTN_COUNT]; // CSTICK_* bits OR'd into this target
    int dpadMode;  // 0=none, 1=gc d-pad
    int stickMode; // 0=none, 1=gc main stick
    u32 exitGc;    // AND-mask, all must be held
};

WiimoteSection  s_wm;
GamecubeSection s_gc;
bool s_loaded = false;

// ---------------------------------------------------------------------------
// String helpers (small local copies -- see wii/game_presets.cpp for the
// same idea; kept separate on purpose, these two files don't share code)
// ---------------------------------------------------------------------------

char* Trim(char* s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    char* end = s + strlen(s);
    while (end > s && isspace((unsigned char)*(end - 1))) end--;
    *end = '\0';
    return s;
}

void StripComment(char* s)
{
    for (char* p = s; *p; p++)
        if (*p == '#' || *p == ';') { *p = '\0'; break; }
}

bool Eq(const char* a, const char* b)
{
    char la[64], lb[64];
    strncpy(la, a, sizeof(la) - 1); la[sizeof(la) - 1] = '\0';
    strncpy(lb, b, sizeof(lb) - 1); lb[sizeof(lb) - 1] = '\0';
    for (char* p = la; *p; p++) *p = (char)tolower((unsigned char)*p);
    for (char* p = lb; *p; p++) *p = (char)tolower((unsigned char)*p);
    return strcmp(la, lb) == 0;
}

bool IsOff(const char* v) { return Eq(v, "undefined") || Eq(v, "none") || Eq(v, "off"); }

// ---------------------------------------------------------------------------
// Token resolvers -- one physical source name -> underlying bit(s)
// ---------------------------------------------------------------------------

bool ResolveWiimoteToken(const char* tok, int* word, u32* bit)
{
    struct Entry { const char* name; int word; u32 bit; };
    static const Entry table[] = {
        { "a", 0, WPAD_BUTTON_A }, { "b", 0, WPAD_BUTTON_B },
        { "1", 0, WPAD_BUTTON_1 }, { "2", 0, WPAD_BUTTON_2 },
        { "home", 0, WPAD_BUTTON_HOME },
        { "minus", 0, WPAD_BUTTON_MINUS }, { "-", 0, WPAD_BUTTON_MINUS },
        { "plus", 0, WPAD_BUTTON_PLUS },   { "+", 0, WPAD_BUTTON_PLUS },
        { "up", 0, WPAD_BUTTON_UP }, { "down", 0, WPAD_BUTTON_DOWN },
        { "left", 0, WPAD_BUTTON_LEFT }, { "right", 0, WPAD_BUTTON_RIGHT },
        { "nunchuck_z", 1, WPAD_NUNCHUK_BUTTON_Z },
        { "nunchuck_c", 1, WPAD_NUNCHUK_BUTTON_C },
        { "classic_a", 2, WPAD_CLASSIC_BUTTON_A }, { "classic_b", 2, WPAD_CLASSIC_BUTTON_B },
        { "classic_x", 2, WPAD_CLASSIC_BUTTON_X }, { "classic_y", 2, WPAD_CLASSIC_BUTTON_Y },
        { "classic_l", 2, WPAD_CLASSIC_BUTTON_FULL_L }, { "classic_r", 2, WPAD_CLASSIC_BUTTON_FULL_R },
        { "classic_zl", 2, WPAD_CLASSIC_BUTTON_ZL }, { "classic_zr", 2, WPAD_CLASSIC_BUTTON_ZR },
        { "classic_plus", 2, WPAD_CLASSIC_BUTTON_PLUS }, { "classic_minus", 2, WPAD_CLASSIC_BUTTON_MINUS },
        { "classic_home", 2, WPAD_CLASSIC_BUTTON_HOME },
        { "classic_up", 2, WPAD_CLASSIC_BUTTON_UP }, { "classic_down", 2, WPAD_CLASSIC_BUTTON_DOWN },
        { "classic_left", 2, WPAD_CLASSIC_BUTTON_LEFT }, { "classic_right", 2, WPAD_CLASSIC_BUTTON_RIGHT },
    };

    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++)
    {
        if (Eq(tok, table[i].name)) { *word = table[i].word; *bit = table[i].bit; return true; }
    }

    if (!IsOff(tok))
        printf("[user_controls] Unknown Wiimote source: '%s'\n", tok);
    return false;
}

bool ResolveGamecubeToken(const char* tok, u32* gcBit, u8* cstickBit)
{
    *gcBit = 0;
    *cstickBit = 0;

    struct Entry { const char* name; u32 bit; };
    static const Entry table[] = {
        { "a", PAD_BUTTON_A }, { "b", PAD_BUTTON_B }, { "x", PAD_BUTTON_X }, { "y", PAD_BUTTON_Y },
        { "start", PAD_BUTTON_START },
        { "up", PAD_BUTTON_UP }, { "down", PAD_BUTTON_DOWN },
        { "left", PAD_BUTTON_LEFT }, { "right", PAD_BUTTON_RIGHT },
        { "l", PAD_TRIGGER_L }, { "r", PAD_TRIGGER_R }, { "z", PAD_TRIGGER_Z },
    };
    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++)
    {
        if (Eq(tok, table[i].name)) { *gcBit = table[i].bit; return true; }
    }

    struct CEntry { const char* name; u8 bit; };
    static const CEntry ctable[] = {
        { "c_stick_up", CSTICK_UP }, { "cstick_up", CSTICK_UP },
        { "c_stick_down", CSTICK_DOWN }, { "cstick_down", CSTICK_DOWN },
        { "c_stick_left", CSTICK_LEFT }, { "cstick_left", CSTICK_LEFT },
        { "c_stick_right", CSTICK_RIGHT }, { "cstick_right", CSTICK_RIGHT },
    };
    for (size_t i = 0; i < sizeof(ctable) / sizeof(ctable[0]); i++)
    {
        if (Eq(tok, ctable[i].name)) { *cstickBit = ctable[i].bit; return true; }
    }

    if (!IsOff(tok))
        printf("[user_controls] Unknown GameCube source: '%s'\n", tok);
    return false;
}

int ResolveTargetKey(const char* key)
{
    if (Eq(key, "a")) return UC_A;
    if (Eq(key, "b")) return UC_B;
    if (Eq(key, "x")) return UC_X;
    if (Eq(key, "y")) return UC_Y;
    if (Eq(key, "start")) return UC_START;
    if (Eq(key, "l") || Eq(key, "d")) return UC_TRIGGER_L;
    if (Eq(key, "r") || Eq(key, "c")) return UC_TRIGGER_R;
    if (Eq(key, "z")) return UC_Z;
    return -1; // D-PAD / STICK / exit_combo are handled separately
}

// Splits "RandLandZ" / "-and+" on the literal word "and" (case-insensitive)
// and resolves each token via the section's own resolver, ANDing every
// matched bit into that section's exit-combo mask.
void ApplyExitCombo(bool isWiimoteSection, const char* val)
{
    char buf[128];
    strncpy(buf, val, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    for (char* p = buf; *p; p++) *p = (char)tolower((unsigned char)*p);

    if (isWiimoteSection) { s_wm.exitWii = s_wm.exitNunchuk = s_wm.exitClassic = 0; }
    else                    s_gc.exitGc = 0;

    char* p = buf;
    while (*p)
    {
        char* sep = strstr(p, "and");
        char* tokEnd = sep ? sep : p + strlen(p);
        char saved = *tokEnd;
        *tokEnd = '\0';
        char* tok = Trim(p);

        if (*tok)
        {
            if (isWiimoteSection)
            {
                int word; u32 bit;
                if (ResolveWiimoteToken(tok, &word, &bit))
                {
                    if (word == 0) s_wm.exitWii |= bit;
                    else if (word == 1) s_wm.exitNunchuk |= bit;
                    else s_wm.exitClassic |= bit;
                }
            }
            else
            {
                u32 gcBit; u8 cBit;
                ResolveGamecubeToken(tok, &gcBit, &cBit); // c-stick ignored for exit combos
                s_gc.exitGc |= gcBit;
            }
        }

        *tokEnd = saved;
        p = sep ? sep + 3 : tokEnd;
    }
}

s8 ClampAnalog(s32 value, s32 deadzone)
{
    if (abs(value) < deadzone) return 0;
    if (value > 127) value = 127;
    if (value < -128) value = -128;
    return (s8)value;
}

} // namespace

// ---------------------------------------------------------------------------
// Public: load
// ---------------------------------------------------------------------------

void user_controls_load(const char* path)
{
    s_loaded = false;
    memset(&s_wm, 0, sizeof(s_wm));
    memset(&s_gc, 0, sizeof(s_gc));

    FILE* f = fopen(path, "r");
    if (!f)
    {
        printf("[user_controls] No file at %s -- USER CFG layout will fall back to Default\n", path);
        return;
    }

    enum { SEC_NONE, SEC_WIIMOTE, SEC_GAMECUBE } sec = SEC_NONE;
    char line[256];

    while (fgets(line, sizeof(line), f))
    {
        char* s = Trim(line);
        if (!*s || *s == '#' || *s == ';') continue;

        if (*s == '[')
        {
            const char* open = strchr(s, '[');
            const char* close = strchr(s, ']');
            char name[64] = "";
            if (open && close && close > open)
            {
                size_t len = close - open - 1;
                if (len >= sizeof(name)) len = sizeof(name) - 1;
                memcpy(name, open + 1, len);
                name[len] = '\0';
            }

            if (Eq(name, "dreamcast_wiimote")) { sec = SEC_WIIMOTE; s_wm.defined = true; }
            else if (Eq(name, "dreamcast_gamecube")) { sec = SEC_GAMECUBE; s_gc.defined = true; }
            else sec = SEC_NONE;
            continue;
        }

        if (sec == SEC_NONE) continue;

        char* eq = strchr(s, '=');
        if (!eq) continue;
        *eq = '\0';
        char* key = Trim(s);
        char* val = Trim(eq + 1);
        StripComment(val);
        val = Trim(val);
        if (!*key || !*val) continue;

        if (Eq(key, "exit_combo"))
        {
            ApplyExitCombo(sec == SEC_WIIMOTE, val);
            continue;
        }

        if (Eq(key, "d-pad") || Eq(key, "dpad"))
        {
            int mode = IsOff(val) ? 0 : 1;
            if (sec == SEC_WIIMOTE) s_wm.dpadMode = mode;
            else                    s_gc.dpadMode = mode;
            continue;
        }

        if (Eq(key, "stick"))
        {
            if (sec == SEC_WIIMOTE)
                s_wm.stickMode = (Eq(val, "nunchuck_stick") || Eq(val, "classic_stick")) ? 1 : 0;
            else
                s_gc.stickMode = Eq(val, "stick") ? 1 : 0;
            continue;
        }

        int target = ResolveTargetKey(key);
        if (target < 0)
        {
            printf("[user_controls] Unknown key '%s' in section\n", key);
            continue;
        }

        if (sec == SEC_WIIMOTE)
        {
            int word; u32 bit;
            if (ResolveWiimoteToken(val, &word, &bit))
            {
                if (word == 0) s_wm.wii[target] |= bit;
                else if (word == 1) s_wm.nunchuk[target] |= bit;
                else s_wm.classic[target] |= bit;
            }
        }
        else
        {
            u32 gcBit; u8 cBit;
            if (ResolveGamecubeToken(val, &gcBit, &cBit))
            {
                s_gc.gc[target] |= gcBit;
                s_gc.cstick[target] |= cBit;
            }
        }
    }

    fclose(f);
    s_loaded = s_wm.defined || s_gc.defined;
    printf("[user_controls] Loaded %s (wiimote section: %s, gamecube section: %s)\n",
           path, s_wm.defined ? "yes" : "no", s_gc.defined ? "yes" : "no");
}

int user_controls_loaded(void) { return s_loaded; }

// ---------------------------------------------------------------------------
// Public: evaluate (called once per port per frame from drkMapleDevices.cpp)
// ---------------------------------------------------------------------------

void UserControls_Update(u32 wiiButtons, u32 gcButtons, u32 nunchuckButtons, u32 classicButtons,
                          s32 gcStickX, s32 gcStickY, s32 gcSubStickX, s32 gcSubStickY,
                          s32 expStickX, s32 expStickY,
                          u16* outKcode, s8* outJoyX, s8* outJoyY, u8* outLt, u8* outRt)
{
    u16 kcode = 0xFFFF;

    u8 cstick = 0;
    if (gcSubStickY >  CSTICK_THRESHOLD) cstick |= CSTICK_UP;
    if (gcSubStickY < -CSTICK_THRESHOLD) cstick |= CSTICK_DOWN;
    if (gcSubStickX < -CSTICK_THRESHOLD) cstick |= CSTICK_LEFT;
    if (gcSubStickX >  CSTICK_THRESHOLD) cstick |= CSTICK_RIGHT;

    for (int t = 0; t < UC_BTN_COUNT; t++)
    {
        bool pressed = false;
        if (s_wm.defined &&
            ((s_wm.wii[t] & wiiButtons) || (s_wm.nunchuk[t] & nunchuckButtons) || (s_wm.classic[t] & classicButtons)))
            pressed = true;
        if (s_gc.defined && ((s_gc.gc[t] & gcButtons) || (s_gc.cstick[t] & cstick)))
            pressed = true;

        if (pressed) kcode &= ~kTargetBit[t];
    }

    // D-Pad: each section can enable its OWN device family's physical d-pad
    // (Classic Controller included on the Wiimote side, same as the legacy
    // mapping). No per-direction cross-remapping here -- that's what the
    // CHUCHU ROCKET / DDR SELECT special layouts already do for their game.
    if (s_wm.defined && s_wm.dpadMode)
    {
        if (wiiButtons & WPAD_BUTTON_UP)    kcode &= ~key_CONT_DPAD_UP;
        if (wiiButtons & WPAD_BUTTON_DOWN)  kcode &= ~key_CONT_DPAD_DOWN;
        if (wiiButtons & WPAD_BUTTON_LEFT)  kcode &= ~key_CONT_DPAD_LEFT;
        if (wiiButtons & WPAD_BUTTON_RIGHT) kcode &= ~key_CONT_DPAD_RIGHT;

        if (classicButtons & WPAD_CLASSIC_BUTTON_UP)    kcode &= ~key_CONT_DPAD_UP;
        if (classicButtons & WPAD_CLASSIC_BUTTON_DOWN)  kcode &= ~key_CONT_DPAD_DOWN;
        if (classicButtons & WPAD_CLASSIC_BUTTON_LEFT)  kcode &= ~key_CONT_DPAD_LEFT;
        if (classicButtons & WPAD_CLASSIC_BUTTON_RIGHT) kcode &= ~key_CONT_DPAD_RIGHT;
    }
    if (s_gc.defined && s_gc.dpadMode)
    {
        if (gcButtons & PAD_BUTTON_UP)    kcode &= ~key_CONT_DPAD_UP;
        if (gcButtons & PAD_BUTTON_DOWN)  kcode &= ~key_CONT_DPAD_DOWN;
        if (gcButtons & PAD_BUTTON_LEFT)  kcode &= ~key_CONT_DPAD_LEFT;
        if (gcButtons & PAD_BUTTON_RIGHT) kcode &= ~key_CONT_DPAD_RIGHT;
    }

    // Stick: GameCube main stick takes priority when deflected (mirrors
    // MapAnalogStick()'s own fallback rule), else the Wiimote expansion
    // stick if that section enabled one. nunchuck_stick and classic_stick
    // are treated identically -- only one expansion can be attached at a
    // time anyway.
    s32 finalX = 0, finalY = 0;
    bool haveStick = false;

    if (s_gc.defined && s_gc.stickMode &&
        (abs(gcStickX) >= USER_ANALOG_DEADZONE || abs(gcStickY) >= USER_ANALOG_DEADZONE))
    {
        finalX = gcStickX; finalY = gcStickY; haveStick = true;
    }
    if (!haveStick && s_wm.defined && s_wm.stickMode)
    {
        finalX = expStickX; finalY = expStickY;
    }

    *outJoyX = ClampAnalog(finalX, USER_ANALOG_DEADZONE);
    *outJoyY = ClampAnalog(-finalY, USER_ANALOG_DEADZONE); // DC positive Y = down

    *outLt = (kcode & key_CONT_D) ? 0 : 255;
    *outRt = (kcode & key_CONT_C) ? 0 : 255;
    *outKcode = kcode;
}

bool UserControls_CheckExitCombo(u32 wiiButtons, u32 gcButtons, u32 nunchuckButtons, u32 classicButtons)
{
    if (s_wm.defined && (s_wm.exitWii || s_wm.exitNunchuk || s_wm.exitClassic))
    {
        bool wiiOk     = (wiiButtons      & s_wm.exitWii)     == s_wm.exitWii;
        bool nunchukOk = (nunchuckButtons & s_wm.exitNunchuk) == s_wm.exitNunchuk;
        bool classicOk = (classicButtons  & s_wm.exitClassic) == s_wm.exitClassic;
        if (wiiOk && nunchukOk && classicOk)
            return true;
    }
    if (s_gc.defined && s_gc.exitGc && (gcButtons & s_gc.exitGc) == s_gc.exitGc)
        return true;
    return false;
}
