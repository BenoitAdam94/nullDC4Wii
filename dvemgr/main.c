#include <pspsdk.h>
#include "pspge_driver.h"
#include <pspkernel.h>
#include <pspsysevent.h>


#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("pspDveManager_Module", 0x1006, 1, 0);

int sceHprm_driver_1528D408();
int sceImposeSetVideoOutMode(int, int, int);
int sceDve_driver_DEB2F80C(int);
int sceDve_driver_93828323(int);
int sceDve_driver_0B85524C(int);
int sceDve_driver_A265B504(int, int, int);

#define RETURN(x) res = x; pspSdkSetK1(k1); return x
//0x5BAA5439	sceGeEdramSetSize
int pspSetVramSize(int size)
{
	sceGeEdramSetSize(size);
	return sceGeEdramGetSize();
} 
int pspDveMgrCheckVideoOut()
{
	int k1 = pspSdkSetK1(0);
	int intr = sceKernelCpuSuspendIntr();

	// Warning: nid changed between 3.60 and 3.71
	int cable = sceHprm_driver_1528D408();

	sceKernelCpuResumeIntr(intr);
	pspSdkSetK1(k1);

	return cable;
}

int pspDveMgrSetVideoOut(int u, int mode, int width, int height, int x, int y, int z)
{
	int k1 = pspSdkSetK1(0);
	int res;

	res = sceDve_driver_DEB2F80C(u);
	if (res < 0)
	{
		RETURN(-1);
	}
	
	// These params will end in sceDisplaySetMode
	res = sceImposeSetVideoOutMode(mode, width, height);
	if (res < 0)
	{
		RETURN(-2);
	}

	res = sceDve_driver_93828323(0);
	if (res < 0)
	{
		RETURN(-3);
	}

	res = sceDve_driver_0B85524C(1);
	if (res < 0)
	{
		RETURN(-4);
	}

	res = sceDve_driver_A265B504(x, y, z);
	if (res < 0)
	{
		RETURN(-5);
	}
	
	pspSdkSetK1(k1);
	return res;
}

int module_start(SceSize args, void *argp)
{	
	return 0;
}

int module_stop(SceSize args, void *argp)
{
	return 0;
}

#define PROFILER_REG_BASE 0xBC400000
#define PROFILER_REG_COUNT 21

void usrDebugProfilerEnable(void)
{
	_sw(1, PROFILER_REG_BASE);
}

void usrDebugProfilerDisable(void)
{
	_sw(0, PROFILER_REG_BASE);
	asm("sync\r\n");
}

void usrDebugProfilerClear(void)
{
	u32 addr;
	int i;

	addr = PROFILER_REG_BASE;
	/* Don't clear the enable register */
	for(i = 1; i < PROFILER_REG_COUNT; i++)
	{
		addr += 4;
		_sw(0, addr);
	}
}

void usrDebugProfilerGetRegs(PspDebugProfilerRegs *regs)
{
	u32 *p_regs;
	u32 addr;
	int i;

	if(regs == NULL)
	{
		return;
	}

	p_regs = (u32 *) regs;

	addr = PROFILER_REG_BASE;
	for(i = 0; i < PROFILER_REG_COUNT; i++)
	{
		p_regs[i] = _lw(addr);
		addr += 4;
	}
}
