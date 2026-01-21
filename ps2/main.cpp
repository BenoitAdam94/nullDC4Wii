#include "types.h"
#include "dc/mem/_vmem.h"
#include "dc/sh4/sh4_registers.h"
#include "dc/sh4/sh4_opcode_list.h"
#include "stdclass.h"
#include "dc/dc.h"
#include "config/config.h"
#include "plugins/plugin_manager.h"
#include "cl/cl.h"
#include <chrono>


#undef r
#undef fr

void __debugbreak() { fflush(stdout); *(int*)0=1;}

void SetApplicationPath(wchar* path);

int main(int argc, wchar* argv[])
{
	/*
	if (!freopen("host0:/ndclog.txt","w",stdout))
		freopen("ndclog.txt","w",stdout);
	setbuf(stdout,0);
	if (!freopen("host0:/ndcerrlog.txt","w",stderr))
		freopen("ndcerrlog.txt","w",stderr);
	setbuf(stderr,0);
	*/

	__asm__ __volatile__ ("ctc1 $0, $31");

	int rv=EmuMain(argc,argv);

	return rv;
}


int os_GetFile(char *szFileName, char *szParse,u32 flags)
{
	// strcpy(szFileName, "/Users/skmp/projects/nullDCe/ps2/boot.gdi");
	return 0;
}

int os_msgbox(const wchar* text,unsigned int type)
{
	printf("OS_MSGBOX: %s\n",text);
	return 0;
}

double os_GetSeconds()
{
	auto now = std::chrono::system_clock::now();
    std::chrono::duration<double> duration = now.time_since_epoch();

	return duration.count();
}