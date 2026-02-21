#pragma once
#include "types.h"

// Dreamcast Maple bus controller input mapping
// Button IDs for a standard Sega controller (active-low: 0 = pressed)
enum PlainJoystickButtonId
{
	PJBI_B          = 1 << 1,   // 2
	PJBI_A          = 1 << 2,   // 4
	PJBI_START      = 1 << 3,   // 8
	PJBI_DPAD_UP    = 1 << 4,   // 16
	PJBI_DPAD_DOWN  = 1 << 5,   // 32
	PJBI_DPAD_LEFT  = 1 << 6,   // 64
	PJBI_DPAD_RIGHT = 1 << 7,   // 128
	PJBI_Y          = 1 << 9,   // 512
	PJBI_X          = 1 << 10,  // 1024

	PJBI_Count = 16
};

enum PlainJoystickAxisId
{
	PJAI_X1 = 0,
	PJAI_Y1 = 1,
	PJAI_X2 = 2,
	PJAI_Y2 = 3,
	
	PJAI_Count = 4
};

enum PlainJoystickTriggerId
{
	PJTI_L = 0,
	PJTI_R = 1,

	PJTI_Count = 2
};

struct PlainJoystickState
{
	PlainJoystickState()
	{
		kcode   = 0xFFFF;   // All buttons released (active-low)
		joy[0]  = joy[1] = joy[2] = joy[3] = 0x80; // Centered
		trigger[0] = trigger[1] = 0;                // Not pressed
	}

	// Bitmask of all mappable buttons
	static const u32 ButtonMask =
		PJBI_B | PJBI_A | PJBI_START |
		PJBI_DPAD_UP | PJBI_DPAD_DOWN | PJBI_DPAD_LEFT | PJBI_DPAD_RIGHT |
		PJBI_Y | PJBI_X;

	// Bitmask of all mappable analog axes
	static const u32 AxisMask = (1 << PJAI_X1) | (1 << PJAI_Y1);

	// Bitmask of all mappable triggers
	static const u32 TriggerMask = (1 << PJTI_L) | (1 << PJTI_R);

	u32 kcode;              // Active-low button bitfield
	u8  joy[PJAI_Count];    // Analog axes, 0x80 = center
	u8  trigger[PJTI_Count];// Analog triggers, 0 = released, 0xFF = full
};

struct IMapleConfigMap
{
	virtual void GetInput(PlainJoystickState* pjs) = 0;
	virtual void SetImage(void* img) = 0;
	virtual ~IMapleConfigMap() {}
};

void mcfg_CreateDevices();
void mcfg_DestroyDevices();