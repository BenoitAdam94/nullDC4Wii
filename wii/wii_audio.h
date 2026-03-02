#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Call once at startup, after ASND_Init() in main.cpp
void wii_audio_init();

// Call once at shutdown
void wii_audio_term();

// Call once after AICA_Init() completes — enables sample generation.
// Until this is called, wii_audio_frame() is a safe no-op.
void wii_audio_aica_ready();

// Call once per frame (from DoRender, before VIDEO_WaitVSync).
// Runs 735 AICA timesteps and pushes the resulting samples into the ASND voice.
void wii_audio_frame();

#ifdef __cplusplus
}
#endif
