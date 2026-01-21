/*
	SPG emulation; Scanline/Raster beam registers & interrupts
	H blank interrupts aren't properly emulated
*/

#include "spg.h"
#include "Renderer_if.h"
#include "regs.h"

u32 spg_InVblank=0;
s32 spg_ScanlineSh4CycleCounter;
u32 spg_ScanlineCount=512;
u32 spg_CurrentScanline=-1;
u32 spg_VblankCount=0;
s32 spg_LineSh4Cycles=0;
u32 spg_FrameSh4Cycles=0;


double spg_last_vps=0;
#if HOST_OS == OS_PSP
extern "C"
{
	void usrDebugProfilerEnable(void);
	void usrDebugProfilerDisable(void);
	void usrDebugProfilerClear(void);

	typedef struct _PspDebugProfilerRegs2
	{
		volatile u32 enable;
		volatile u32 systemck;
		volatile u32 cpuck;
		//volatile u32 totalstall; ??
		volatile u32 internal;
		volatile u32 memory;
		volatile u32 copz;
		volatile u32 vfpu;
		volatile u32 sleep;
		volatile u32 bus_access;
		volatile u32 uncached_load;
		volatile u32 uncached_store;
		volatile u32 cached_load;
		volatile u32 cached_store;
		volatile u32 i_miss;
		volatile u32 d_miss;
		volatile u32 d_writeback;
		volatile u32 cop0_inst;
		volatile u32 fpu_inst;
		volatile u32 vfpu_inst;
		volatile u32 local_bus;
		volatile u32 waste[5];
	} PspDebugProfilerRegs2;

	void usrDebugProfilerGetRegs(PspDebugProfilerRegs2 *regs);
}
#endif


//54 mhz pixel clock (actually, this is defined as 27 .. why ? --drk)
#define PIXEL_CLOCK (54*1000*1000/2)


//Called when spg registers are updated
void CalculateSync()
{
	u32 pixel_clock;
	float scale_x=1,scale_y=1;

	if (FB_R_CTRL.vclk_div)
	{
		//VGA :)
		pixel_clock=PIXEL_CLOCK;
	}
	else
	{
		//It is half for NTSC/PAL
		pixel_clock=PIXEL_CLOCK/2;
	}

	//Derive the cycle counts from the pixel clock
	spg_ScanlineCount=SPG_LOAD.vcount+1;

	//Rounding errors here but meh
	spg_LineSh4Cycles=(u64)SH4_CLOCK*(u64)(SPG_LOAD.hcount+1)/(u64)pixel_clock;

	if (SPG_CONTROL.interlace)
	{
		//this is a temp hack and needs work ...
		spg_LineSh4Cycles/=2;
		u32 interl_mode=(VO_CONTROL>>4)&0xF;

		scale_y=1;
	}
	else
	{
		if ((SPG_CONTROL.NTSC == 0 && SPG_CONTROL.PAL ==0) ||
			(SPG_CONTROL.NTSC == 1 && SPG_CONTROL.PAL ==1))
		{
			scale_y=1.0f;//non interlaced vga mode has full resolution :)
		}
		else
			scale_y=0.5f;//non interlaced modes have half resolution
	}

	rend_set_fb_scale(scale_x,scale_y);

	spg_FrameSh4Cycles=spg_ScanlineCount*spg_LineSh4Cycles;
}

s32 render_end_pending_cycles;
//called from sh4 context , should update pvr/ta state and evereything else
void FASTCALL libPvr_UpdatePvr(u32 cycles)
{
	spg_ScanlineSh4CycleCounter -= cycles;

	if (spg_ScanlineSh4CycleCounter < spg_LineSh4Cycles)//60 ~herz = 200 mhz / 60=3333333.333 cycles per screen refresh
	{
		//ok .. here , after much effort , a full scanline was emulated !
		//now , we must check for raster beam interupts and vblank
		spg_CurrentScanline=(spg_CurrentScanline+1)%spg_ScanlineCount;
		spg_ScanlineSh4CycleCounter += spg_LineSh4Cycles;
		
		//Test for scanline interrupts

		if (SPG_VBLANK_INT.vblank_in_interrupt_line_number == spg_CurrentScanline)
			params.RaiseInterrupt(holly_SCANINT1);

		if (SPG_VBLANK_INT.vblank_out_interrupt_line_number == spg_CurrentScanline)
			params.RaiseInterrupt(holly_SCANINT2);

		if (SPG_VBLANK.vbstart == spg_CurrentScanline)
			spg_InVblank=1;

		if (SPG_VBLANK.vbend == spg_CurrentScanline)
			spg_InVblank=0;

		if (SPG_CONTROL.interlace)
			SPG_STATUS.fieldnum=~SPG_STATUS.fieldnum;
		else
			SPG_STATUS.fieldnum=0;

		SPG_STATUS.vsync=spg_InVblank;
		SPG_STATUS.scanline=spg_CurrentScanline;

		//Vblank/etc code
		if (SPG_VBLANK.vbstart == spg_CurrentScanline)
		{
			//Vblank counter
			spg_VblankCount++;
			
			// This turned out to be HBlank btw , needs to be emulated ;(
			params.RaiseInterrupt(holly_HBLank);
			
			//notify for vblank :)
			rend_vblank();

			double tdiff=os_GetSeconds()-spg_last_vps;

			if (tdiff>2)
			{
				spg_last_vps=os_GetSeconds();
				double spd_fps=(FrameCount)/tdiff;
				double spd_vbs=(spg_VblankCount)/tdiff;
				double spd_cpu=spd_vbs*spg_FrameSh4Cycles;
				spd_cpu/=1000000;	//mrhz kthx
				double fullvbs=(spd_vbs/spd_cpu)*200;
				double mv=VertexCount /1000.0;

				VertexCount=0;
				FrameCount=0;
				spg_VblankCount=0;

				char fpsStr[256];
				const char* mode=0;
				const char* res=0;

				res=SPG_CONTROL.interlace?"480i":"240p";

				if (SPG_CONTROL.NTSC==0 && SPG_CONTROL.PAL==1)
					mode="PAL";
				else if (SPG_CONTROL.NTSC==1 && SPG_CONTROL.PAL==0)
					mode="NTSC";
				else
				{
					res=SPG_CONTROL.interlace?"480i":"480p";
					mode="VGA";
				}

				sprintf(fpsStr,"%3.2f%% VPS:%3.2f(%s%s%3.2f)RPS:%3.2f vt:%4.2fK %4.2fK",
					spd_cpu*100/200,spd_vbs,
					mode,res,fullvbs,
					spd_fps,mv/spd_fps/tdiff, mv/tdiff);

				rend_set_fps_text(fpsStr);

				#ifndef TARGET_PSP


				printf("%s\n",fpsStr);
				#else
				static int x=0;
				x++;
				if (x==8 && 0)
				{
					x=0;
					PspDebugProfilerRegs2 regs;
					memset(&regs,0,sizeof(regs));
					usrDebugProfilerGetRegs (&regs) ;
					static const char* fopenmode="w";			//"w" the first time
					FILE* f=fopen("PERFLOG.txt",fopenmode);
					fopenmode="a";								//"a" after that :)
					if (f)
					{
						fprintf(f,	"\\/------------------------------------\\/ \r\n"
									"%s\r\n"
								,fpsStr);

	#define STA2(x,y) x,(x)/(float)((y)/100.f)
	#define STAT(x) STA2(x,regs.systemck)

						fprintf(f,	"********** Profile ***********\r\n");
						fprintf(f,	"enable         : %10u \r\n", regs.enable);
						fprintf(f,	"systemck       : %10u % 3.2f%% [cycles]\r\n", STAT(regs.systemck));
						fprintf(f,	"cpu ck         : %10u % 3.2f%% [cycles]\r\n", STAT(regs.cpuck));
						fprintf(f,	"stall (total)  : %10u % 3.2f%% [cycles]\r\n", STAT(regs.internal + regs.memory + regs.copz + regs.vfpu));
						fprintf(f,	"+-(internal)   : %10u % 3.2f%% [cycles]\r\n", STAT(regs.internal));
						fprintf(f,	"+-(memory)     : %10u % 3.2f%% [cycles]\r\n", STAT(regs.memory));
						fprintf(f,	"+-(COPz)       : %10u % 3.2f%% [cycles]\r\n", STAT(regs.copz));
						fprintf(f,	"+-(VFPU)       : %10u % 3.2f%% [cycles]\r\n", STAT(regs.vfpu));
						fprintf(f,	"sleep          : %10u % 3.2f%% [cycles]\r\n", STAT(regs.sleep));
						fprintf(f,	"bus access     : %10u % 3.2f%% [cycles]\r\n", STAT(regs.bus_access));
						fprintf(f,	"local bus      : %10u % 3.2f%% [cycles]\r\n", STAT(regs.local_bus));

						fprintf(f,	"cached load    : %10u % 3.2f%% [times]\r\n", STA2(regs.cached_load,regs.uncached_load+regs.cached_load));
						fprintf(f,	"uncached load  : %10u % 3.2f%% [times]\r\n", STA2(regs.uncached_load,regs.uncached_load+regs.cached_load));

						fprintf(f,	"cached store   : %10u % 3.2f%% [times]\r\n", STA2(regs.cached_store,regs.uncached_store+regs.cached_store));
						fprintf(f,	"uncached store : %10u % 3.2f%% [times]\r\n", STA2(regs.uncached_store,regs.uncached_store+regs.cached_store));

						fprintf(f,	"I cache miss   : %10u [times]\r\n", (regs.i_miss));
						fprintf(f,	"D cache miss   : %10u [times]\r\n", (regs.d_miss));
						fprintf(f,	"D cache wb     : %10u [times]\r\n", (regs.d_writeback));
						fprintf(f,	"COP0 inst.     : %10u [inst.]\r\n", (regs.cop0_inst));
						fprintf(f,	"FPU  inst.     : %10u [inst.]\r\n", (regs.fpu_inst));
						fprintf(f,	"VFPU inst.     : %10u [inst.]\r\n", (regs.vfpu_inst));


						fclose(f);
					

						usrDebugProfilerDisable() ;
						usrDebugProfilerClear ( ) ;
						usrDebugProfilerEnable( ) ;
					}
				}
			#endif
			}
		}
	}


		if (render_end_pending_cycles>0)
		{
			render_end_pending_cycles-=cycles;
			if (render_end_pending_cycles<=0)
			{
				params.RaiseInterrupt(holly_RENDER_DONE);
				params.RaiseInterrupt(holly_RENDER_DONE_isp);
				params.RaiseInterrupt(holly_RENDER_DONE_vd);
				rend_end_render();
			}
		}
		
}


bool spg_Init()
{
	return true;
}

void spg_Term()
{
}

void spg_Reset(bool Manual)
{
	CalculateSync();
}

