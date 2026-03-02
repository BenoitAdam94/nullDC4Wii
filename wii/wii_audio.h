#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Call once at startup, after ASND_Init() in main.cpp
void wii_audio_init();

// Call once at shutdown
void wii_audio_term();

// Call once per frame (from DoRender, before VIDEO_WaitVSync).
// Runs 735 AICA timesteps and pushes the resulting samples into the ASND voice.
void wii_audio_frame();

#ifdef __cplusplus
}
#endif
