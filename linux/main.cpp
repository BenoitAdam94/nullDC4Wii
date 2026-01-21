#include "../types.h"
#include <poll.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include "plugins/plugin_manager.h"

termios tios, orig_tios;

int setup_curses()
{
  /* Get current terminal settings */
  if (tcgetattr(0, &orig_tios)){
    printf("Error getting current terminal settings\n");
    return 3;
  }

  tios = orig_tios;         // Copy that to "tios" and play with it
  tios.c_lflag &= ~ICANON;  // We want to disable the canonical mode
  tios.c_lflag |= ECHO;     // And make sure ECHO is enabled

  if (tcsetattr(0, TCSANOW, &tios)){
    printf("Error applying terminal settings\n");
    return 3;
  }

  if (tcgetattr(0, &tios)){
    tcsetattr(0, TCSANOW, &orig_tios);
    printf("Error while asserting terminal settings\n");
    return 3;
  }

  if ((tios.c_lflag & ICANON) || !(tios.c_lflag & ECHO)) {
    tcsetattr(0, TCSANOW, &orig_tios);
    printf("Could not apply all terminal settings\n");
    return 3;
  }

  fcntl(0, F_SETFL, O_NONBLOCK);

}

void term_curses()
{
  /* Restore terminal settings */
  tcsetattr(0, TCSANOW, &orig_tios);
}

#if HOST_CPU==CPU_ARM_CORTEX_A8
void enable_runfast()
{
	static const unsigned int x = 0x04086060;
	static const unsigned int y = 0x03000000;
	int r;
	asm volatile (
		"fmrx	%0, fpscr			\n\t"	//r0 = FPSCR
		"and	%0, %0, %1			\n\t"	//r0 = r0 & 0x04086060
		"orr	%0, %0, %2			\n\t"	//r0 = r0 | 0x03000000
		"fmxr	fpscr, %0			\n\t"	//FPSCR = r0
		: "=r"(r)
		: "r"(x), "r"(y)
	);

	printf("ARM VFP-Run Fast (NFP) enabled !\n");
}
#endif

int main(int argc, wchar* argv[])
{
	#if HOST_CPU==CPU_ARM_CORTEX_A8
	enable_runfast();
	#endif

    setup_curses();

	int rv=EmuMain(argc,argv);

#ifndef _ANDROID
    term_curses();
#endif

	return rv;
}

int os_GetFile(char *szFileName, char *szParse,u32 flags)
{
	strcpy(szFileName,GetEmuPath("discs/game.gdi"));
    return 1;
}


double os_GetSeconds()
{
	return clock()/(double)CLOCKS_PER_SEC;
}

int os_msgbox(const wchar* text,unsigned int type)
{
	printf("OS_MSGBOX: %s\n",text);
	return 0;
}

#ifdef _ANDROID
#include <jni.h>

extern "C" {
    JNIEXPORT void JNICALL Java_nada_main_JNIdc_init(JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_nada_main_JNIdc_step(JNIEnv * env, jobject obj);
	JNIEXPORT void JNICALL Java_nada_main_JNIdc_setvp(JNIEnv * env, jobject obj, int w,int h);

	JNIEXPORT void JNICALL Java_nada_main_JNIdc_kcode(JNIEnv * env, jobject obj,u32 kcode,u32 lt, u32 rt);
	JNIEXPORT void JNICALL Java_nada_main_JNIdc_vjoy(JNIEnv * env, jobject obj,u32 id,float x, float y);
};

void SetApplicationPath(wchar* path);
JNIEXPORT void JNICALL Java_nada_main_JNIdc_init(JNIEnv * env, jobject obj)
{
	printf("Running main\n");
	SetApplicationPath("/nand/");

	FILE* f=fopen(GetEmuPath("data/dc_boot.bin"),"rb");

	if (!f)
	{
		printf("Trying alternative path /sdcard/\n");

		SetApplicationPath("/sdcard/");

		f=fopen(GetEmuPath("data/dc_boot.bin"),"rb");
		if (!f)
		{
			printf("Trying alternative path /sdcard/nulldc\n");

			SetApplicationPath("/sdcard/nulldc/");

			f=fopen(GetEmuPath("data/dc_boot.bin"),"rb");
		}
	}
	if (f) fclose(f);
	main(0,0);
	printf("Returning from main!\n");
}

JNIEXPORT void JNICALL Java_nada_main_JNIdc_step(JNIEnv * env, jobject obj)
{
	sh4_cpu.Run();
}

extern u16 kcode[4];
extern u8 rt[4],lt[4];
extern float vjoy_pos[11][2];

JNIEXPORT void JNICALL Java_nada_main_JNIdc_kcode(JNIEnv * env, jobject obj,u32 k_code, u32 l_t, u32 r_t)
{
	lt[0]=l_t;
	rt[0]=r_t;
	kcode[0]=k_code;
	kcode[3]=kcode[2]=kcode[1]=0xFFFF;
}

extern float dc_width;
extern float dc_height;

bool InitRenderer();
JNIEXPORT void JNICALL Java_nada_main_JNIdc_setvp(JNIEnv * env, jobject obj, int w,int h)
{
		dc_width=w;
		dc_height=h;
		InitRenderer();
}

JNIEXPORT void JNICALL Java_nada_main_JNIdc_vjoy(JNIEnv * env, jobject obj,u32 id,float x, float y)
{
	vjoy_pos[id][0]=x;
	vjoy_pos[id][1]=y;
}
#endif