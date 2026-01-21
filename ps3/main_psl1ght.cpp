#include "types.h"
#include <unistd.h>
//#include <dirent.h>
//#include <sys/paths.h>
//#include <sys/process.h>
//#include <cell/pad/libpad.h>
//#include <cell/pad/error.h>

//SYS_PROCESS_PARAM(1001, 0x10000);

#define ROOT_PATH "/"
#define ROOT_PATH_2 "/dev_hdd0/game/NUDC00000" "/USRDIR/"
void SetApplicationPath(wchar* path);
int main(int argc, wchar* argv[])
{
//	if (CELL_PAD_OK!=cellPadInit(1))
//		printf("Pad init failed :(\n");
//	else
//		printf("Pad stuff working \n");

	printf("Setting the path to " ROOT_PATH "\n");
//	SetApplicationPath(ROOT_PATH);

	FILE* f=fopen(GetEmuPath("data/dc_boot.bin"),"rb");
	if (!f)
	{
		printf("Setting the path to " ROOT_PATH_2 "\n");
		SetApplicationPath(ROOT_PATH_2);
	}
	else
		fclose(f);

	int rv=EmuMain(argc,argv);
	
	return rv;
}

#include <time.h>

int os_GetFile(char *szFileName, char *szParse,u32 flags)
{
	strcpy(szFileName,GetEmuPath("discs/game.gdi"));
    return 1;
}

double os_GetSeconds()
{
	return time(0);
}

int os_msgbox(const wchar* text,unsigned int type)
{
	printf("OS_MSGBOX: %s\n",text);
	return 0;
}

void __debugbreak()
{
	printf("CRASHED \n");
	for(;;);
}

//namespace std
//{
//	void exception::_Raise()  const { __debugbreak(); }
//};
