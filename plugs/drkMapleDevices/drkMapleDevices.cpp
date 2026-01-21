// drkMapleDevices.cpp : Defines the entry point for the DLL application.
//
#include "plugins/plugin_header.h"
#include <string.h>
#include <math.h>

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include "dc/dc.h"

#if HOST_OS==OS_WII
	#include <wiiuse/wpad.h>
	#include "dc/dc.h"
#endif


#define key_CONT_C  (1 << 0)
#define key_CONT_B  (1 << 1)
#define key_CONT_A  (1 << 2)
#define key_CONT_START  (1 << 3)
#define key_CONT_DPAD_UP  (1 << 4)
#define key_CONT_DPAD_DOWN  (1 << 5)
#define key_CONT_DPAD_LEFT  (1 << 6)
#define key_CONT_DPAD_RIGHT  (1 << 7)
#define key_CONT_Z  (1 << 8)
#define key_CONT_Y  (1 << 9)
#define key_CONT_X  (1 << 10)
#define key_CONT_D  (1 << 11)
#define key_CONT_DPAD2_UP  (1 << 12)
#define key_CONT_DPAD2_DOWN  (1 << 13)
#define key_CONT_DPAD2_LEFT  (1 << 14)
#define key_CONT_DPAD2_RIGHT  (1 << 15)

u16 kcode[4]={0xFFFF,0xFFFF,0xFFFF,0xFFFF};
u32 vks[4]={0};
s8 joyx[4]={0},joyy[4]={0};
u8 rt[4]={0},lt[4]={0};

#if HOST_OS == OS_PSP
#include <pspctrl.h>
	void UpdateInputState(u32 port)
	{
		SceCtrlData pad;
		sceCtrlPeekBufferPositive(&pad, 1);
		//kcode,rt,lt,joyx,joyy
		joyx[port]=pad.Lx-128;
		joyy[port]=pad.Ly-128;
		rt[port]=pad.Buttons&PSP_CTRL_RTRIGGER?255:0;
		lt[port]=pad.Buttons&PSP_CTRL_LTRIGGER?255:0;

		kcode[port]=0xFFFF;
		if (pad.Buttons&PSP_CTRL_CROSS)
			kcode[port]&=~key_CONT_A;
		if (pad.Buttons&PSP_CTRL_CIRCLE)
			kcode[port]&=~key_CONT_B;
		if (pad.Buttons&PSP_CTRL_TRIANGLE)
			kcode[port]&=~key_CONT_Y;
		if (pad.Buttons&PSP_CTRL_SQUARE)
			kcode[port]&=~key_CONT_X;

		if (pad.Buttons&PSP_CTRL_START)
			kcode[port]&=~key_CONT_START;

		if (pad.Buttons&PSP_CTRL_UP)
			kcode[port]&=~key_CONT_DPAD_UP;
		if (pad.Buttons&PSP_CTRL_DOWN)
			kcode[port]&=~key_CONT_DPAD_DOWN;
		if (pad.Buttons&PSP_CTRL_LEFT)
			kcode[port]&=~key_CONT_DPAD_LEFT;
		if (pad.Buttons&PSP_CTRL_RIGHT)
			kcode[port]&=~key_CONT_DPAD_RIGHT;
	}
#elif HOST_OS==OS_LINUX

#ifndef _ANDROID

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <linux/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/input.h>
#include <poll.h>
#include <termios.h>

int input_fd[3]={-1,-1,-1};

void setnonblocking(int sock)
{
	int opts;

	opts = fcntl(sock,F_GETFL);
	if (opts < 0) {
		perror("fcntl(F_GETFL)");
		return;
	}
	opts = (opts | O_NONBLOCK);
	if (fcntl(sock,F_SETFL,opts) < 0) {
		perror("fcntl(F_SETFL)");
		return;
	}
	return;
}

void InitPandoraInput()
{
	for (int id = 0; ; id++)
	{
		char fname[64];
		char name[256] = { 0, };
		int fd;

		snprintf(fname, sizeof(fname), "/dev/input/event%i", id);
		fd = open(fname, O_RDONLY);
		if (fd == -1)
		{
			break;
		}
		ioctl(fd, EVIOCGNAME(sizeof(name)), name);
		printf("INPUT: %s --> %s\n",fname,name);
		if (strcmp(name, "gpio-keys") == 0)
		{
			input_fd[0] = fd;
		}
		else if (strcmp(name, "vsense66") == 0)
		{
			input_fd[1] = fd;
		}
		else if (strcmp(name, "vsense67") == 0)
		{
			input_fd[2] = fd;
		}
		else
		{
			close(fd);
			continue;
		}
		setnonblocking(fd);
		printf("using fd %d\n",fd);
	}

	if (input_fd[0] == -1) printf("Warning: couldn't find game-keys device\n");
	if (input_fd[1] == -1) printf("Warning: couldn't find nub1 device\n");
	if (input_fd[2] == -1) printf("Warning: couldn't find nub2 device\n");
}




    u32 GetKey()
    {
        u32 smth=0;
        if(read(0, &smth, 1))
        return smth;
    }


    #define GetAsyncKeyState(c) ((k)==(c))

    void UpdateInputState_console(u32 port)
    {
		kcode[port]=0xFFFF;

		for(;;)
		{
			u32 k=GetKey();
			

			rt[port]=GetAsyncKeyState('a')?255:0;
			lt[port]=GetAsyncKeyState('s')?255:0;

			
			if (GetAsyncKeyState('v')) { kcode[port]&=~key_CONT_A; }
			if (GetAsyncKeyState('c')) { kcode[port]&=~key_CONT_B; }
			if (GetAsyncKeyState('x')) { kcode[port]&=~key_CONT_Y; }
			if (GetAsyncKeyState('z')) { kcode[port]&=~key_CONT_X; }

			if (GetAsyncKeyState(0x0A))
				kcode[port]&=~key_CONT_START;
			
			if (GetAsyncKeyState('i'))
			kcode[port]&=~key_CONT_DPAD_UP;
			if (GetAsyncKeyState('k'))
			kcode[port]&=~key_CONT_DPAD_DOWN;
			if (GetAsyncKeyState('j'))
			kcode[port]&=~key_CONT_DPAD_LEFT;
			if (GetAsyncKeyState('l'))
			kcode[port]&=~key_CONT_DPAD_RIGHT;

			if (k==0)
				break;
		}

    }

	void UpdateInputState(u32 port)
	{
		static bool inited=false;
		if (!inited)
		{
			inited=true;
			InitPandoraInput();
			kcode[port]=0xFFFF;
		}
		if (input_fd[0]==-1)
		{
			UpdateInputState_console(port);
			return;
		}
		input_event ev[64];

		for (int j = 0; j < 3; j++)
		{
			//printf("reading from %d %d\n",j,input_fd[j]);
			int rd = read(input_fd[j], ev, sizeof(ev));
			//printf("read returned %d\n",rd);
			if (rd<=0)
				continue;
			rd/=sizeof(ev[0]);

			if (rd ==0) 
			{
				//perror("\nevtest: error reading");
				continue;
			}

			for (int i = 0; i < rd ; i++)
			{
				//printf("INPUT from %d :%d: %d -- %d\n",j,ev[i].type, ev[i].code, ev[i].value);
				if (ev[i].type == EV_SYN) continue;
				if (ev[i].type == EV_KEY)
				{
					//setkey(ev[i].code, ev[i].value);
					int bit=0;
					switch(ev[i].code)
					{
					case KEY_UP: 
						bit=key_CONT_DPAD_UP;
						break;
					case KEY_DOWN: 
						bit=key_CONT_DPAD_DOWN;
						break;
					case KEY_LEFT: 
						bit=key_CONT_DPAD_LEFT;
						break;
					case KEY_RIGHT: 
						bit=key_CONT_DPAD_RIGHT;
						break;

					case KEY_MENU: 
						Stop_DC();
						break;

					case KEY_LEFTALT:
					case BTN_SELECT:
					case BTN_START: 
						bit=key_CONT_START;
						break;

						//KEY_END (Y/North), KEY_HOME (A/East), KEY_PAGEDOWN (X/South), KEY_END (B/West), 
						//KEY_RIGHTSHIFT (Shoulder L), KEY_RIGHTCTRL (Shoulder R), KEY_KPPLUS (Shoulder L2), 
						//KEY_KPMINUS (Shoulder R2), KEY_COFFEE (Hold)
					case KEY_PAGEDOWN:
					case BTN_X: 
						bit=key_CONT_X;
						break;

					case KEY_END:
					case BTN_Y: 
						bit=key_CONT_Y;
						break;

					case KEY_HOME:
					case BTN_A: 
						bit=key_CONT_A;
						break;

					case KEY_PAGEUP:
					case BTN_B: 
						bit=key_CONT_B;
						break;
						//triggers ?

					case KEY_KPPLUS:
					case KEY_RIGHTSHIFT:
					case BTN_TL: 
						lt[port]=ev[i].value?255:0;
						break;

					case KEY_KPMINUS:
					case KEY_RIGHTCTRL:
					case BTN_TR:
						rt[port]=ev[i].value?255:0;
						break;

					default:
						printf("DEFAULT CASE ON INPUTMAP (%d)\n",ev[i].code);
					}

					if (ev[i].value)
					{
						kcode[port]&=~bit;
					}
					else
					{
						kcode[port]|=bit;
					}

					//printf("bit: %08X, kcode %08X\n",bit,kcode[port]);
				}
				else if (ev[i].type == EV_ABS) 
				{
					if (ev[i].code == ABS_X)
					{
						joyx[port]= ev[i].value*128/265;
					}
					else if (ev[i].code == ABS_Y)
					{
						joyy[port]= ev[i].value*128/265;
					}
					else
					{
						printf("unexpected EV_ABS code: %i\n", ev[i].code);
					}
				}
				else 
				{
					printf("unexpected event type %i received\n", ev[i].type);
					continue;
				}
			}
		}

	}
#else
void UpdateInputState(u32 port)
	{
}
#endif

#elif HOST_OS==OS_WINDOWS
#include <windows.h>
	void UpdateInputState(u32 port)
	{
		//joyx[port]=pad.Lx;
		//joyy[port]=pad.Ly;
		rt[port]=GetAsyncKeyState('A')?255:0;
		lt[port]=GetAsyncKeyState('S')?255:0;

		kcode[port]=0xFFFF;
		if (GetAsyncKeyState('V'))
			kcode[port]&=~key_CONT_A;
		if (GetAsyncKeyState('C'))
			kcode[port]&=~key_CONT_B;
		if (GetAsyncKeyState('X'))
			kcode[port]&=~key_CONT_Y;
		if (GetAsyncKeyState('Z'))
			kcode[port]&=~key_CONT_X;

		if (GetAsyncKeyState(VK_SHIFT))
			kcode[port]&=~key_CONT_START;

		if (GetAsyncKeyState(VK_UP))
			kcode[port]&=~key_CONT_DPAD_UP;
		if (GetAsyncKeyState(VK_DOWN))
			kcode[port]&=~key_CONT_DPAD_DOWN;
		if (GetAsyncKeyState(VK_LEFT))
			kcode[port]&=~key_CONT_DPAD_LEFT;
		if (GetAsyncKeyState(VK_RIGHT))
			kcode[port]&=~key_CONT_DPAD_RIGHT;
	}
#elif HOST_OS==OS_WII
	void UpdateInputState(u32 port)
	{		
        PAD_ScanPads();
		WPAD_ScanPads();
		
		u32 pressed = WPAD_ButtonsHeld(port);
		u32 padpressed = PAD_ButtonsHeld(port);
		s32 pad_stickx = PAD_StickX(port);
		s32 pad_sticky = PAD_StickY(port);

		if ( pressed & WPAD_BUTTON_HOME || (padpressed&PAD_TRIGGER_R && padpressed&PAD_TRIGGER_L && padpressed&PAD_TRIGGER_Z))
			exit(0);

		//kcode,rt,lt,joyx,joyy
		//joyx[port]=pad.Lx-128;
		//joyy[port]=pad.Ly-128;

		//rt[port]=pad.Buttons&PSP_CTRL_RTRIGGER?255:0;
		//lt[port]=pad.Buttons&PSP_CTRL_LTRIGGER?255:0;

		kcode[port]=0xFFFF;
		if (pressed&WPAD_BUTTON_A || padpressed&PAD_BUTTON_A)
			kcode[port]&=~key_CONT_A;
		if (pressed&WPAD_BUTTON_B || padpressed&PAD_BUTTON_B)
			kcode[port]&=~key_CONT_B;
		if (pressed&WPAD_BUTTON_1 || padpressed&PAD_BUTTON_Y)
			kcode[port]&=~key_CONT_Y;
		if (pressed&WPAD_BUTTON_2 || padpressed&PAD_BUTTON_X)
			kcode[port]&=~key_CONT_X;

		if (pressed&WPAD_BUTTON_PLUS || padpressed&PAD_BUTTON_START)
			kcode[port]&=~key_CONT_START;

		if (pressed&WPAD_BUTTON_UP || pad_sticky > 20)
			kcode[port]&=~key_CONT_DPAD_UP;
		if (pressed&WPAD_BUTTON_DOWN || pad_sticky < -20)
			kcode[port]&=~key_CONT_DPAD_DOWN;
		if (pressed&WPAD_BUTTON_LEFT || pad_stickx < -20)
			kcode[port]&=~key_CONT_DPAD_LEFT;
		if (pressed&WPAD_BUTTON_RIGHT || pad_stickx > 20)
			kcode[port]&=~key_CONT_DPAD_RIGHT;
	}
#elif HOST_OS == OS_PS3 && defined(CELL_SDK)
#include <cell/pad/libpad.h>
#include <cell/pad/error.h>
void UpdateInputState(u32 port)
{
	CellPadData data;
	if (cellPadGetData(0,&data)!=CELL_PAD_OK)
	{
		printf("Failed to get pad info\n");
	}

	if (data.len!=0)
	{
		//kcode,rt,lt,joyx,joyy
		joyx[port]=data.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X]-128;
		joyy[port]=data.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y]-128;
		rt[port]=data.button[CELL_PAD_BTN_OFFSET_DIGITAL2]&CELL_PAD_CTRL_R1?255:0;
		lt[port]=data.button[CELL_PAD_BTN_OFFSET_DIGITAL2]&CELL_PAD_CTRL_L1?255:0;

		kcode[port]=0xFFFF;
		if (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2]&CELL_PAD_CTRL_CROSS)
			kcode[port]&=~key_CONT_A;
		if (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2]&CELL_PAD_CTRL_CIRCLE)
			kcode[port]&=~key_CONT_B;
		if (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2]&CELL_PAD_CTRL_TRIANGLE)
			kcode[port]&=~key_CONT_Y;
		if (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2]&CELL_PAD_CTRL_SQUARE)
			kcode[port]&=~key_CONT_X;
		 
		if (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1]&CELL_PAD_CTRL_START)
			kcode[port]&=~key_CONT_START;

		if (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1]&CELL_PAD_CTRL_UP)
			kcode[port]&=~key_CONT_DPAD_UP;
		if (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1]&CELL_PAD_CTRL_DOWN)
			kcode[port]&=~key_CONT_DPAD_DOWN;
		if (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1]&CELL_PAD_CTRL_LEFT)
			kcode[port]&=~key_CONT_DPAD_LEFT;
		if (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1]&CELL_PAD_CTRL_RIGHT)
			kcode[port]&=~key_CONT_DPAD_RIGHT;
	}
}
#else
void UpdateInputState(u32 port)
{
	//no keys ..
}
#endif
