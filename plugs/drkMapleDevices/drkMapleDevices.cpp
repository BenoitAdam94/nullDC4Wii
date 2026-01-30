// drkMapleDevices.cpp : Wii Controller device (Also PSP/PS3/Windows but removed for clean code)
//
#include "plugins/plugin_header.h"
#include <string.h>
#include <math.h>

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include "dc/dc.h"

#include <wiiuse/wpad.h>
#include "dc/dc.h"

#define key_CONT_C (1 << 0)
#define key_CONT_B (1 << 1)
#define key_CONT_A (1 << 2)
#define key_CONT_START (1 << 3)
#define key_CONT_DPAD_UP (1 << 4)
#define key_CONT_DPAD_DOWN (1 << 5)
#define key_CONT_DPAD_LEFT (1 << 6)
#define key_CONT_DPAD_RIGHT (1 << 7)
#define key_CONT_Z (1 << 8)
#define key_CONT_Y (1 << 9)
#define key_CONT_X (1 << 10)
#define key_CONT_D (1 << 11)
#define key_CONT_DPAD2_UP (1 << 12)
#define key_CONT_DPAD2_DOWN (1 << 13)
#define key_CONT_DPAD2_LEFT (1 << 14)
#define key_CONT_DPAD2_RIGHT (1 << 15)

u16 kcode[4] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
u32 vks[4] = {0};
s8 joyx[4] = {0}, joyy[4] = {0};
u8 rt[4] = {0}, lt[4] = {0};

void UpdateInputState(u32 port)
{
  PAD_ScanPads();
  WPAD_ScanPads();

  u32 pressed = WPAD_ButtonsHeld(port);
  u32 padpressed = PAD_ButtonsHeld(port);
  s32 pad_stickx = PAD_StickX(port);
  s32 pad_sticky = PAD_StickY(port);

  // IF HOME (Wiimote) or R+L+Z (Gamecube controller), then exit app
  if (pressed & WPAD_BUTTON_HOME || (padpressed & PAD_TRIGGER_R && padpressed & PAD_TRIGGER_L && padpressed & PAD_TRIGGER_Z))
    exit(0);

  // kcode,rt,lt,joyx,joyy
  // joyx[port]=pad.Lx-128;
  // joyy[port]=pad.Ly-128;

  // rt[port]=pad.Buttons&PSP_CTRL_RTRIGGER?255:0;
  // lt[port]=pad.Buttons&PSP_CTRL_LTRIGGER?255:0;

  // Buttons
  kcode[port] = 0xFFFF;
  if (pressed & WPAD_BUTTON_A || padpressed & PAD_BUTTON_A)
    kcode[port] &= ~key_CONT_A;
  if (pressed & WPAD_BUTTON_B || padpressed & PAD_BUTTON_B)
    kcode[port] &= ~key_CONT_B;
  if (pressed & WPAD_BUTTON_1 || padpressed & PAD_BUTTON_Y)
    kcode[port] &= ~key_CONT_Y;
  if (pressed & WPAD_BUTTON_2 || padpressed & PAD_BUTTON_X)
    kcode[port] &= ~key_CONT_X;

  // Start
  if (pressed & WPAD_BUTTON_PLUS || padpressed & PAD_BUTTON_START)
    kcode[port] &= ~key_CONT_START;

  // D-Pad
  if (pressed & WPAD_BUTTON_UP || pad_sticky > 20)
    kcode[port] &= ~key_CONT_DPAD_UP;
  if (pressed & WPAD_BUTTON_DOWN || pad_sticky < -20)
    kcode[port] &= ~key_CONT_DPAD_DOWN;
  if (pressed & WPAD_BUTTON_LEFT || pad_stickx < -20)
    kcode[port] &= ~key_CONT_DPAD_LEFT;
  if (pressed & WPAD_BUTTON_RIGHT || pad_stickx > 20)
    kcode[port] &= ~key_CONT_DPAD_RIGHT;
}
