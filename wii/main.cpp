#include "types.h"
#include <unistd.h>
#include "iso.h"
#include <fat.h>
#include <dirent.h>
#include <wiiuse/wpad.h>

void dirlist(char* path)
{
	DIR* pdir = opendir(path);

	if (pdir != NULL)
	{
		u32 sentinel = 0;

		while(true) 
		{
			struct dirent* pent = readdir(pdir);
			if(pent == NULL) break;

			if(strcmp(".", pent->d_name) != 0 && strcmp("..", pent->d_name) != 0)
			{
				char dnbuf[260];
				sprintf(dnbuf, "%s/%s", path, pent->d_name);

				struct stat statbuf;
				stat(dnbuf, &statbuf);

				if(S_ISDIR(statbuf.st_mode))
				{
					printf("%s <DIR>\n", dnbuf);
					dirlist(dnbuf);
				}
				else
				{
					printf("%s (%d)\n", dnbuf, (int)statbuf.st_size);
				}
				sentinel++;
			}
		}

		if (sentinel == 0)
			printf("empty\n");

		closedir(pdir);
		printf("\n");
	}
	else
	{
		printf("opendir() failure.\n");
	}
}

void SetApplicationPath(wchar* path);
int main(int argc, wchar* argv[])
{
    PAD_Init();
	WPAD_Init();

	//ISO9660_Mount() //won't work on dolphin for now ...
/*
	printf("%08X %08X\n",0xABCDEF98,host_to_le<4>(0xABCDEF98));
	printf("%04X %04X\n",0xABCD,host_to_le<2>(0xABCD));
	printf("%08X %08X\n",0xABCDEF98,HOST_TO_LE32(0xABCDEF98));
	printf("%04X %04X\n",0xABCD,HOST_TO_LE16(0xABCD));
*/

	if(fatInitDefault())
	{
		printf("SD card mounted !\n");
		//This commented code works but no text is output to the created file. --Arikado
		//dirlist("/");
		
		//not working for now
		if (!fopen("/dolphin","r"))
			freopen("/ndclog.txt","w",stdout);
	}
	
	SetApplicationPath("/");

	int rv=EmuMain(argc,argv);

	return rv;
}

#include <time.h>

int os_GetFile(char *szFileName, char *szParse,u32 flags)
{
	strcpy(szFileName,GetEmuPath("discs/game.gdi"));
    return true;
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
