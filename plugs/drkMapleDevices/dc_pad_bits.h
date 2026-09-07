#pragma once

// Dreamcast maple controller button bits, as read from the pad's condition
// report (bit cleared = pressed). Shared between drkMapleDevices.cpp (the
// legacy per-device mapping) and wii/user_controls.cpp (the user-remappable
// mapping) so both agree on the same bit layout without duplicating it.
#define key_CONT_C          (1 << 0)
#define key_CONT_B          (1 << 1)
#define key_CONT_A          (1 << 2)
#define key_CONT_START       (1 << 3)
#define key_CONT_DPAD_UP     (1 << 4)
#define key_CONT_DPAD_DOWN   (1 << 5)
#define key_CONT_DPAD_LEFT   (1 << 6)
#define key_CONT_DPAD_RIGHT  (1 << 7)
#define key_CONT_Z           (1 << 8)
#define key_CONT_Y           (1 << 9)
#define key_CONT_X           (1 << 10)
#define key_CONT_D           (1 << 11)
#define key_CONT_DPAD2_UP     (1 << 12)
#define key_CONT_DPAD2_DOWN   (1 << 13)
#define key_CONT_DPAD2_LEFT   (1 << 14)
#define key_CONT_DPAD2_RIGHT  (1 << 15)
