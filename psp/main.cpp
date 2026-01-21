

#include "types.h"
#include "dc/mem/_vmem.h"
#include "dc/sh4/sh4_registers.h"
#include "dc/sh4/sh4_opcode_list.h"
#include "stdclass.h"
#include "dc/dc.h"
#include "config/config.h"
#include "plugins/plugin_manager.h"
#include "cl/cl.h"

#include <pspgu.h>
#include <psprtc.h>

#undef r
#undef fr

PSP_MODULE_INFO(VER_SHORTNAME, PSP_MODULE_USER, VER_MAJOR, VER_MINOR);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER|THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-256);

#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <psppower.h>

#ifdef NOPSPLINK

PspDebugRegBlock exception_regs;

//int ftext;
extern u8 CodeCache[3*1024*1024];
extern u32 LastAddr,pc;
#define ftext (CodeCache[0])

static const char *codeTxt[32] =
{
    "Interrupt", "TLB modification", "TLB load/inst fetch", "TLB store",
    "Address load/inst fetch", "Address store", "Bus error (instr)",
    "Bus error (data)", "Syscall", "Breakpoint", "Reserved instruction",
    "Coprocessor unusable", "Arithmetic overflow", "Unknown 14",
    "Unknown 15", "Unknown 16[fpu]", "Unknown 17", "Unknown 18", "Unknown 19",
    "Unknown 20", "Unknown 21", "Unknown 22", "Unknown 23", "Unknown 24",
    "Unknown 25", "Unknown 26", "Unknown 27", "Unknown 28", "Unknown 29",
    "Unknown 31"
};

static const unsigned char regName[32][5] =
{
    "zr", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};

extern u32 last_block;
void exception_handler(PspDebugRegBlock * regs)
{
	FILE*f=fopen("host0:/dynarec_code.bin","wb");
	if (!f) f=fopen("dynarec_code.bin","wb");
	fwrite(CodeCache,LastAddr,1,f);
	fclose(f);
	fflush(stdout);

    int i;
    SceCtrlData pad;

    pspDebugScreenInit();
    pspDebugScreenSetBackColor(0x00FF0000);
    pspDebugScreenSetTextColor(0xFFFFFFFF);
    pspDebugScreenClear();
    pspDebugScreenPrintf("Your PSP has just crashed!\n");
    pspDebugScreenPrintf("Exception details:\n\n");

    pspDebugScreenPrintf("Exception - %s\n", codeTxt[(regs->cause >> 2) & 31]);
    pspDebugScreenPrintf("EPC       - %08X / %s.text + %08X\n", (int)regs->epc, "nullDC PSP port", (unsigned int)(regs->epc-(int)&ftext));
    pspDebugScreenPrintf("Cause     - %08X\n", (int)regs->cause);
    pspDebugScreenPrintf("Status    - %08X\n", (int)regs->status);
    pspDebugScreenPrintf("BadVAddr  - %08X\n", (int)regs->badvaddr);
    for(i=0; i<32; i+=4) pspDebugScreenPrintf("%s:%08X %s:%08X %s:%08X %s:%08X\n", regName[i], (int)regs->r[i], regName[i+1], (int)regs->r[i+1], regName[i+2], (int)regs->r[i+2], regName[i+3], (int)regs->r[i+3]);
	pspDebugScreenPrintf("pc = 0x%08X last block = %08X\n",next_pc,last_block);
    sceKernelDelayThread(1000000);
    pspDebugScreenPrintf("\n\nPress X to dump information on file exception.log and quit");
    pspDebugScreenPrintf("\nPress O to quit");

    for (;;){
        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_CROSS){
			FILE *log = fopen("host0:/exception.log", "w");
			if (!log) log = fopen("exception.log", "w");
			if (log != NULL){
                char testo[512];
                sprintf(testo, "Exception details:\n\n");
                fwrite(testo, 1, strlen(testo), log);
                sprintf(testo, "Exception - %s\n", codeTxt[(regs->cause >> 2) & 31]);
                fwrite(testo, 1, strlen(testo), log);
                sprintf(testo, "EPC       - %08X / %s.text + %08X\n", (int)regs->epc, "nullDC PSP port", (unsigned int)(regs->epc-(int)&ftext));
                fwrite(testo, 1, strlen(testo), log);
                sprintf(testo, "Cause     - %08X\n", (int)regs->cause);
                fwrite(testo, 1, strlen(testo), log);
                sprintf(testo, "Status    - %08X\n", (int)regs->status);
                fwrite(testo, 1, strlen(testo), log);
                sprintf(testo, "BadVAddr  - %08X\n", (int)regs->badvaddr);
                fwrite(testo, 1, strlen(testo), log);
                for(i=0; i<32; i+=4){
                    sprintf(testo, "%s:%08X %s:%08X %s:%08X %s:%08X\n", regName[i], (int)regs->r[i], regName[i+1], (int)regs->r[i+1], regName[i+2], (int)regs->r[i+2], regName[i+3], (int)regs->r[i+3]);
                    fwrite(testo, 1, strlen(testo), log);
                }
                fclose(log);
            }
            break;
        }else if (pad.Buttons & PSP_CTRL_CIRCLE){
            break;
        }
		sceKernelDelayThread(100000);
    }
    sceKernelExitGame();
}

static void setup_exception_handler()
{
   SceKernelLMOption option;
   int args[2], fd, modid;

   memset(&option, 0, sizeof(option));
   option.size = sizeof(option);
   option.mpidtext = PSP_MEMORY_PARTITION_KERNEL;
   option.mpiddata = PSP_MEMORY_PARTITION_KERNEL;
   option.position = 0;
   option.access = 1;
	printf("Loading exception.prx ..\n");
   if((modid = sceKernelLoadModule("exception.prx", 0, &option)) >= 0)
   {
	   printf("Loaded exception.prx ..\n");
      args[0] = (int)exception_handler;
      args[1] = (int)&exception_regs;
      sceKernelStartModule(modid, 8, args, &fd, NULL);
	  printf("Started exception.prx ..\n");
   }
}
#endif

static int exitRequest = 0;

int running()
{
	return !exitRequest;
}

static int exit_callback(int arg1, int arg2, void *common)
{
	exitRequest = 1;
	Stop_DC();
	return 0;
}

static int callback_thread(SceSize args, void *argp)
{
	int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);

	sceKernelRegisterExitCallback(cbid);

	sceKernelSleepThreadCB();

	return 0;
}

int setup_callbacks(void)
{
#ifdef NOPSPLINK
	setup_exception_handler();
#endif

	int thid = 0;

	thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}

	return thid;
}


extern int startUsbHost();

int main(int argc, wchar* argv[])
{
	startUsbHost();
	if (!freopen("host0:/ndclog.txt","w",stdout))
		freopen("ndclog.txt","w",stdout);
	setbuf(stdout,0);
	if (!freopen("host0:/ndcerrlog.txt","w",stderr))
		freopen("ndcerrlog.txt","w",stderr);
	setbuf(stderr,0);
	scePowerSetClockFrequency(333,333,166);

	__asm__ __volatile__ ("ctc1 $0, $31");

	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	setup_callbacks();

	int rv=EmuMain(argc,argv);

	sceKernelExitGame();
	return rv;
}

#include <dirent.h>
#include <pspctrl.h>

#include <pspkernel.h>
#include <pspusb.h>
#include <pspsdk.h>

#define HOSTFSDRIVER_NAME	"USBHostFSDriver"
#define HOSTFSDRIVER_PID	(0x1C9)
#define USBHOSTFS_PRX		"usbhostfs.prx"

int startUsbHost()
{
	static bool started=false;

	if (started)
		return 1;

	static SceUID pMod = 0;//( SceModule * )sceKernelFindModuleByName( "USBHostFS" );
	if ( !pMod )
	{
		if ( (pMod=pspSdkLoadStartModule( USBHOSTFS_PRX, 0 )) < 0 )
		{
			//log( "Error starting usbhostfs.prx\n" );
			return -1;
		}
	}
	int ret = sceUsbStart( PSP_USBBUS_DRIVERNAME, 0, 0 );
	if ( ret != 0 )
	{
		//log( "Error starting USB Bus driver (0x%08X)\n", ret );
		return -1;
	}
	ret = sceUsbStart( HOSTFSDRIVER_NAME, 0, 0 );
	if ( ret != 0 )
	{
		//log( "Error starting USB Host driver (0x%08X)\n", ret );
		return -1;
	}

	ret = sceUsbActivate( HOSTFSDRIVER_PID );
	if (ret<0)
		return ret;

	if (!(sceUsbGetState() & PSP_USB_CABLE_CONNECTED))
		return -1;

	while (~sceUsbGetState() & PSP_USB_CONNECTION_ESTABLISHED)
		sceKernelDelayThread( 2000 );

	started=ret>=0;
	return ret;
}

struct FileInfoS
{
	string name;
	string path;
};

s32 ListFiles(vector<FileInfoS>& dirs,const char* dir)
{
	DIR* dirp;
    dirent* ent;
	dirp = opendir(dir);
    if (!dirp)
	{
        return -1;
    }
    while((ent = readdir(dirp)) != 0)
    {
		if (strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0 || strstr(ent->d_name,".cditoc")!=0)
			continue;
		char temp[512];
		sprintf(temp,"%s%s",dir,ent->d_name);
		SceIoStat st;
		sceIoGetstat(temp,&st);
		if (st.st_mode & FIO_S_IFDIR) 
		{
			sprintf(temp,"%s%s/",dir,ent->d_name);
			ListFiles(dirs,temp);
		}
		else
		{
			strcpy(temp,ent->d_name);
			char* t=temp;
			while(*t) { *t=tolower(*t); t++; }
			if (strstr(temp,".cdi")!=0 || strstr(temp,".mds")!=0 || strstr(temp,".gdi")!=0 || strstr(temp,".nrg")!=0)
			{
				FileInfoS fies;
				fies.name=dir;
				//fies.name+="/";
				fies.name+=ent->d_name;
				fies.path=dir;
				//fies.path+="/";
				fies.path+=ent->d_name;

				dirs.push_back(fies);
			}
		}
       // free(ent);
    }
    closedir(dirp);
	return 0;
}

void TryUSBHFS(vector<FileInfoS>& dirs)
{
	static bool usb_ok=false;
	if (startUsbHost()>=0)
	{
		usb_ok=true;
	}

	//if (usb_ok)
	{
		ListFiles(dirs,"host0:/");
		printf("HOST %d files\n",dirs.size());
	}
}

void pspguCrear();
void pspguWaitVblank();

int os_GetFile(char *szFileName, char *szParse,u32 flags)
{
	printf("Doing GD list\n");
	vector<FileInfoS> dirs;

	ListFiles(dirs,"./discs/");

	printf("%d files\n",dirs.size());
	if (dirs.size()==0)
	{
		printf("no gdroms found, starting bios\n");
		return 0;
	}
	//bool listing=true;
	size_t selection=0;

	for(;;)
	{
		pspguCrear();
		pspDebugScreenSetXY(0,0);

		pspDebugScreenSetTextColor(0xFF4F6F9F);
		pspDebugScreenPrintf("Select a file with up/dn/X, /\\ means no disc,O tries usbhostfs,[] relists ./gdroms/, HOME Exits to xmb\n>>%s<<\n",dirs[selection].name.c_str());

		u32 filec=0;
		for (size_t i=0;i<dirs.size();i++)
		{
			//pspDebugScreenSetXY(0,i+1);
			if (abs(selection-i)<=10 || (i>=selection && filec<20))
			{
				filec++;
				if (selection==i)
					pspDebugScreenSetTextColor(0xFFFFFFFF);
				else
					pspDebugScreenSetTextColor(0x7F7F7F7F);

				pspDebugScreenPrintf("%s\n",dirs[i].name.c_str());
			}
		}


		pspguWaitVblank();

		SceCtrlData pad;
		static SceCtrlData old={0};
		memset(&pad,0,sizeof(pad));
		sceCtrlPeekBufferPositive(&pad, 1);
		
		//only detect raising edge
		pad.Buttons^=old.Buttons;
		pad.Buttons&=~old.Buttons;

		if (pad.Buttons & PSP_CTRL_CROSS)
		{
			break;
		}
		if (pad.Buttons & PSP_CTRL_TRIANGLE)
		{
			printf("Selected NO DISC\n");
			return 0;
		}
		if (pad.Buttons & PSP_CTRL_CIRCLE)
		{
			dirs.clear();
			ListFiles(dirs,"./gdroms/");
			TryUSBHFS(dirs);
			selection=0;
		}

		if (pad.Buttons & PSP_CTRL_HOME)
		{
			sceKernelExitGame();
		}
		if (pad.Buttons & PSP_CTRL_UP)
		{
			selection--;
			if (selection<0)
				selection=dirs.size()-1;
		}
		if (pad.Buttons & PSP_CTRL_DOWN)
		{
			selection++;
			if (selection>=dirs.size())
				selection=0;
		}

		memcpy(&old,&pad,sizeof(pad));
	}

	printf("Selected %d file\n",selection);
	strcpy(szFileName,dirs[selection].path.c_str());
	pspDebugScreenSetTextColor(0x7F7F7F7F);
	return 1;
}

int os_msgbox(const wchar* text,unsigned int type)
{
	printf("OS_MSGBOX: %s\n",text);
	return 0;
}

double os_GetSeconds()
{
	u64 previous;
	sceRtcGetCurrentTick(&previous);
	u64 res = sceRtcGetTickResolution();

	return previous/res*1000.0;
}