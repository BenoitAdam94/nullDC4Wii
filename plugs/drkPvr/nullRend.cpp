#include "config.h"

#if HOST_OS==OS_WINDOWS
#include <windows.h>
#endif

#include "nullRend.h"
#if REND_API == REND_PSP

#include "regs.h"

using namespace TASplitter;
#if HOST_SYS == SYS_PSP
#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>
#include <dirent.h>
#include <pspctrl.h>
#include <pspkernel.h>
#include <pspusb.h>
#include <pspsdk.h>
#include <pspimpose_driver.h>
extern "C"
{
int pspSetVramSize(int size);
int pspDveMgrCheckVideoOut();
int pspDveMgrSetVideoOut(int, int, int, int, int, int, int);
}
//#define ALIGN16 __attribute__((aligned(16)))

static unsigned int staticOffset = 0;


static unsigned int getMemorySize(unsigned int width, unsigned int height, unsigned int psm)
{
	switch (psm)
	{
		case GU_PSM_T4:
			return (width * height) >> 1;

		case GU_PSM_T8:
			return width * height;

		case GU_PSM_5650:
		case GU_PSM_5551:
		case GU_PSM_4444:
		case GU_PSM_T16:
			return 2 * width * height;

		case GU_PSM_8888:
		case GU_PSM_T32:
			return 4 * width * height;

		default:
			return 0;
	}
}

void* getStaticVramBuffer(unsigned int width, unsigned int height, unsigned int psm)
{
	unsigned int memSize = getMemorySize(width,height,psm);
	void* result = (void*)staticOffset;
	staticOffset += memSize;

	return result;
}

void* getStaticVramTexture(unsigned int width, unsigned int height, unsigned int psm)
{
	void* result = getStaticVramBuffer(width,height,psm);
	return (void*)(((unsigned int)result) + ((unsigned int)sceGeEdramGetAddr()));
}

#else
	#define ALIGN16
	#include "win-x86/plugs/drkPvr/gu_emu.h"
#endif

static unsigned int ALIGN16 list[262144];
bool PSP_720_480 = false;
u32 BUF_WIDTH=512;
u32 SCR_WIDTH=480;
u32 SCR_HEIGHT=272;

void* fbp0;
void* fbp1;
void* zbp;
u32 ALIGN16 palette_lut[1024];

const u32 MipPoint[8] =
{
	0x00006,//8
	0x00016,//16
	0x00056,//32
	0x00156,//64
	0x00556,//128
	0x01556,//256
	0x05556,//512
	0x15556//1024
};


struct Vertex
{
	float u,v;
    unsigned int col;
    float x, y, z;
};

struct VertexList
{
	union
	{
		Vertex* ptr;
		s32 count;
	};
};

struct PolyParam
{
	PCW pcw;
	ISP_TSP isp;

	TSP tsp;
	TCW tcw;
};

const u32 PalFMT[4]=
{
	GU_PSM_5551,
	GU_PSM_5650,
	GU_PSM_4444,
	GU_PSM_8888,
};

Vertex ALIGN16  vertices[42*1024];
VertexList  ALIGN16 lists[8*1024];
PolyParam  ALIGN16 listModes[8*1024];

Vertex* curVTX=vertices;
VertexList* curLST=lists;
VertexList* TransLST=0;
PolyParam* curMod=listModes-1;
bool global_regd;
float vtx_min_Z;
float vtx_max_Z;

//no rendering .. yay (?)
namespace NORenderer
{
	char fps_text[512];

	struct VertexDecoder;
	FifoSplitter<VertexDecoder> TileAccel;


	#define ABGR8888(x) ((x&0xFF00FF00) |((x>>16)&0xFF) | ((x&0xFF)<<16))
	#define ABGR4444(x) ((x&0xF0F0) |((x>>8)&0xF) | ((x&0xF)<<8))
	#define ABGR0565(x) ((x&(0x3F<<5)) |((x>>11)&0x1F) | ((x&0x1F)<<11))
	#define ABGR1555(x) ((x&0x83E0) |((x>>10)&0x1F) | ((x&0x1F)<<10))

	#define ABGR4444_A(x) ((x)>>12)
	#define ABGR4444_R(x) ((x>>8)&0xF)
	#define ABGR4444_G(x) ((x>>4)&0xF)
	#define ABGR4444_B(x) ((x)&0xF)

	#define ABGR0565_R(x) ((x)>>11)
	#define ABGR0565_G(x) ((x>>5)&0x3F)
	#define ABGR0565_B(x) ((x)&0x1F)

	#define ABGR1555_A(x) ((x>>15))
	#define ABGR1555_R(x) ((x>>10)&0x1F)
	#define ABGR1555_G(x) ((x>>5)&0x1F)
	#define ABGR1555_B(x) ((x)&0x1F)


	#define colclamp(low,hi,val) {if (val<low) val=low ; if (val>hi) val=hi;}

	u32 YUV422(s32 Y,s32 Yu,s32 Yv)
	{
		s32 B = (76283*(Y - 16) + 132252*(Yu - 128))>>(16+3);//5
		s32 G = (76283*(Y - 16) - 53281 *(Yv - 128) - 25624*(Yu - 128))>>(16+2);//6
		s32 R = (76283*(Y - 16) + 104595*(Yv - 128))>>(16+3);//5

		colclamp(0,0x1F,B);
		colclamp(0,0x3F,G);
		colclamp(0,0x1F,R);

		return (B<<11) | (G<<5) | (R);
	}
	//use that someday
	void VBlank()
	{

	}

	//pixel convertors !
#define pixelcvt_start(name,x,y)  \
	struct name \
	{ \
	static const u32 xpp=x;\
	static const u32 ypp=y;	\
	__forceinline static void fastcall Convert(u16* pb,u32 pbw,u8* data) \
	{

#define pixelcvt_end } }
#define pixelcvt_next(name,x,y) pixelcvt_end;  pixelcvt_start(name,x,y)

	#define pixelcvt_startVQ(name,x,y)  \
	struct name \
	{ \
	static const u32 xpp=x;\
	static const u32 ypp=y;	\
	__forceinline static u32 fastcall Convert(u16* data) \
	{

#define pixelcvt_endVQ } }
#define pixelcvt_nextVQ(name,x,y) pixelcvt_endVQ;  pixelcvt_startVQ(name,x,y)

	inline void pb_prel(u16* dst,u32 x,u32 col)
	{
		dst[x]=col;
	}
	inline void pb_prel(u16* dst,u32 pbw,u32 x,u32 y,u32 col)
	{
		dst[x+pbw*y]=col;
	}
	//Non twiddled
	pixelcvt_start(conv565_PL,4,1)
	{
		//convert 4x1 565 to 4x1 8888
		u16* p_in=(u16*)data;
		//0,0
		pb_prel(pb,0,ABGR0565(p_in[0]));
		//1,0
		pb_prel(pb,1,ABGR0565(p_in[1]));
		//2,0
		pb_prel(pb,2,ABGR0565(p_in[2]));
		//3,0
		pb_prel(pb,3,ABGR0565(p_in[3]));
	}
	pixelcvt_next(conv1555_PL,4,1)
	{
		//convert 4x1 1555 to 4x1 8888
		u16* p_in=(u16*)data;
		//0,0
		pb_prel(pb,0,ABGR1555(p_in[0]));
		//1,0
		pb_prel(pb,1,ABGR1555(p_in[1]));
		//2,0
		pb_prel(pb,2,ABGR1555(p_in[2]));
		//3,0
		pb_prel(pb,3,ABGR1555(p_in[3]));
	}
	pixelcvt_next(conv4444_PL,4,1)
	{
		//convert 4x1 4444 to 4x1 8888
		u16* p_in=(u16*)data;
		//0,0
		pb_prel(pb,0,ABGR4444(p_in[0]));
		//1,0
		pb_prel(pb,1,ABGR4444(p_in[1]));
		//2,0
		pb_prel(pb,2,ABGR4444(p_in[2]));
		//3,0
		pb_prel(pb,3,ABGR4444(p_in[3]));
	}
	pixelcvt_next(convYUV_PL,4,1)
	{
		//convert 4x1 4444 to 4x1 8888
		u32* p_in=(u32*)data;


		s32 Y0 = (p_in[0]>>8) &255; //
		s32 Yu = (p_in[0]>>0) &255; //p_in[0]
		s32 Y1 = (p_in[0]>>24) &255; //p_in[3]
		s32 Yv = (p_in[0]>>16) &255; //p_in[2]

		//0,0
		pb_prel(pb,0,YUV422(Y0,Yu,Yv));
		//1,0
		pb_prel(pb,1,YUV422(Y1,Yu,Yv));

		//next 4 bytes
		p_in+=1;

		Y0 = (p_in[0]>>8) &255; //
		Yu = (p_in[0]>>0) &255; //p_in[0]
		Y1 = (p_in[0]>>24) &255; //p_in[3]
		Yv = (p_in[0]>>16) &255; //p_in[2]

		//0,0
		pb_prel(pb,2,YUV422(Y0,Yu,Yv));
		//1,0
		pb_prel(pb,3,YUV422(Y1,Yu,Yv));
	}
	pixelcvt_end;
	//twiddled
	pixelcvt_start(conv565_TW,2,2)
	{
		//convert 4x1 565 to 4x1 8888
		u16* p_in=(u16*)data;
		//0,0
		pb_prel(pb,pbw,0,0,ABGR0565(p_in[0]));
		//0,1
		pb_prel(pb,pbw,0,1,ABGR0565(p_in[1]));
		//1,0
		pb_prel(pb,pbw,1,0,ABGR0565(p_in[2]));
		//1,1
		pb_prel(pb,pbw,1,1,ABGR0565(p_in[3]));
	}
	pixelcvt_next(conv1555_TW,2,2)
	{
		//convert 4x1 1555 to 4x1 8888
		u16* p_in=(u16*)data;
		//0,0
		pb_prel(pb,pbw,0,0,ABGR1555(p_in[0]));
		//0,1
		pb_prel(pb,pbw,0,1,ABGR1555(p_in[1]));
		//1,0
		pb_prel(pb,pbw,1,0,ABGR1555(p_in[2]));
		//1,1
		pb_prel(pb,pbw,1,1,ABGR1555(p_in[3]));
	}
	pixelcvt_next(conv4444_TW,2,2)
	{
		//convert 4x1 4444 to 4x1 8888
		u16* p_in=(u16*)data;
		//0,0
		pb_prel(pb,pbw,0,0,ABGR4444(p_in[0]));
		//0,1
		pb_prel(pb,pbw,0,1,ABGR4444(p_in[1]));
		//1,0
		pb_prel(pb,pbw,1,0,ABGR4444(p_in[2]));
		//1,1
		pb_prel(pb,pbw,1,1,ABGR4444(p_in[3]));
	}
	pixelcvt_next(convYUV422_TW,2,2)
	{
		//convert 4x1 4444 to 4x1 8888
		u16* p_in=(u16*)data;


		s32 Y0 = (p_in[0]>>8) &255; //
		s32 Yu = (p_in[0]>>0) &255; //p_in[0]
		s32 Y1 = (p_in[2]>>8) &255; //p_in[3]
		s32 Yv = (p_in[2]>>0) &255; //p_in[2]

		//0,0
		pb_prel(pb,pbw,0,0,YUV422(Y0,Yu,Yv));
		//1,0
		pb_prel(pb,pbw,1,0,YUV422(Y1,Yu,Yv));

		//next 4 bytes
		//p_in+=2;

		Y0 = (p_in[1]>>8) &255; //
		Yu = (p_in[1]>>0) &255; //p_in[0]
		Y1 = (p_in[3]>>8) &255; //p_in[3]
		Yv = (p_in[3]>>0) &255; //p_in[2]

		//0,1
		pb_prel(pb,pbw,0,1,YUV422(Y0,Yu,Yv));
		//1,1
		pb_prel(pb,pbw,1,1,YUV422(Y1,Yu,Yv));
	}
	pixelcvt_end;
	//VQ PAL Stuff
	pixelcvt_startVQ(conv565_VQ,2,2)
	{
		u32 R=ABGR0565_R(data[0]) + ABGR0565_R(data[1]) + ABGR0565_R(data[2]) + ABGR0565_R(data[3]);
		u32 G=ABGR0565_G(data[0]) + ABGR0565_G(data[1]) + ABGR0565_G(data[2]) + ABGR0565_G(data[3]);
		u32 B=ABGR0565_B(data[0]) + ABGR0565_B(data[1]) + ABGR0565_B(data[2]) + ABGR0565_B(data[3]);
		R>>=2;
		G>>=2;
		B>>=2;

		return R | (G<<5) | (B<<11);
	}
	pixelcvt_nextVQ(conv1555_VQ,2,2)
	{
		u32 R=ABGR1555_R(data[0]) + ABGR1555_R(data[1]) + ABGR1555_R(data[2]) + ABGR1555_R(data[3]);
		u32 G=ABGR1555_G(data[0]) + ABGR1555_G(data[1]) + ABGR1555_G(data[2]) + ABGR1555_G(data[3]);
		u32 B=ABGR1555_B(data[0]) + ABGR1555_B(data[1]) + ABGR1555_B(data[2]) + ABGR1555_B(data[3]);
		u32 A=ABGR1555_A(data[0]) + ABGR1555_A(data[1]) + ABGR1555_A(data[2]) + ABGR1555_A(data[3]);
		R>>=2;
		G>>=2;
		B>>=2;
		A>>=2;

		return R | (G<<5) | (B<<10)  | (A<<15);

		//return ABGR1555(data[0]);
		/*
		//convert 4x1 1555 to 4x1 8888
		u16* p_in=(u16*)data;
		//0,0
		pb_prel(pb,pbw,0,0,ABGR1555(p_in[0]));
		//0,1
		pb_prel(pb,pbw,0,1,ABGR1555(p_in[1]));
		//1,0
		pb_prel(pb,pbw,1,0,ABGR1555(p_in[2]));
		//1,1
		pb_prel(pb,pbw,1,1,ABGR1555(p_in[3]));
		*/
	}
	pixelcvt_nextVQ(conv4444_VQ,2,2)
	{
		u32 R=ABGR4444_R(data[0]) + ABGR4444_R(data[1]) + ABGR4444_R(data[2]) + ABGR4444_R(data[3]);
		u32 G=ABGR4444_G(data[0]) + ABGR4444_G(data[1]) + ABGR4444_G(data[2]) + ABGR4444_G(data[3]);
		u32 B=ABGR4444_B(data[0]) + ABGR4444_B(data[1]) + ABGR4444_B(data[2]) + ABGR4444_B(data[3]);
		u32 A=ABGR4444_A(data[0]) + ABGR4444_A(data[1]) + ABGR4444_A(data[2]) + ABGR4444_A(data[3]);
		R>>=2;
		G>>=2;
		B>>=2;
		A>>=2;

		return R | (G<<4) | (B<<8)  | (A<<12);
		//return ABGR4444(data[0]);
		/*
		//convert 4x1 4444 to 4x1 8888
		u16* p_in=(u16*)data;
		//0,0
		pb_prel(pb,pbw,0,0,ABGR4444(p_in[0]));
		//0,1
		pb_prel(pb,pbw,0,1,ABGR4444(p_in[1]));
		//1,0
		pb_prel(pb,pbw,1,0,ABGR4444(p_in[2]));
		//1,1
		pb_prel(pb,pbw,1,1,ABGR4444(p_in[3]));
		*/
	}
	pixelcvt_nextVQ(convYUV422_VQ,2,2)
	{
		//convert 4x1 4444 to 4x1 8888
		u16* p_in=(u16*)data;


		s32 Y0 = (p_in[0]>>8) &255; //
		s32 Yu = (p_in[0]>>0) &255; //p_in[0]
		s32 Y1 = (p_in[2]>>8) &255; //p_in[3]
		s32 Yv = (p_in[2]>>0) &255; //p_in[2]

		return YUV422(16+((Y0-16)+(Y1-16))/2,Yu,Yv);
		/*
		//0,0
		pb_prel(pb,pbw,0,0,YUV422(Y0,Yu,Yv));
		//1,0
		pb_prel(pb,pbw,1,0,YUV422(Y1,Yu,Yv));

		//next 4 bytes
		//p_in+=2;

		Y0 = (p_in[1]>>8) &255; //
		Yu = (p_in[1]>>0) &255; //p_in[0]
		Y1 = (p_in[3]>>8) &255; //p_in[3]
		Yv = (p_in[3]>>0) &255; //p_in[2]

		//0,1
		pb_prel(pb,pbw,0,1,YUV422(Y0,Yu,Yv));
		//1,1
		pb_prel(pb,pbw,1,1,YUV422(Y1,Yu,Yv));*/
	}
	pixelcvt_endVQ;
/*
	pixelcvt_start(convPAL4_TW,4,4)
	{
		u8* p_in=(u8*)data;
		u32* pal=&palette_lut[palette_index];

		pb_prel(pb,pbw,0,0,pal[p_in[0]&0xF]);
		pb_prel(pb,pbw,0,1,pal[(p_in[0]>>4)&0xF]);p_in++;
		pb_prel(pb,pbw,1,0,pal[p_in[0]&0xF]);
		pb_prel(pb,pbw,1,1,pal[(p_in[0]>>4)&0xF]);p_in++;

		pb_prel(pb,pbw,0,2,pal[p_in[0]&0xF]);
		pb_prel(pb,pbw,0,3,pal[(p_in[0]>>4)&0xF]);p_in++;
		pb_prel(pb,pbw,1,2,pal[p_in[0]&0xF]);
		pb_prel(pb,pbw,1,3,pal[(p_in[0]>>4)&0xF]);p_in++;

		pb_prel(pb,pbw,2,0,pal[p_in[0]&0xF]);
		pb_prel(pb,pbw,2,1,pal[(p_in[0]>>4)&0xF]);p_in++;
		pb_prel(pb,pbw,3,0,pal[p_in[0]&0xF]);
		pb_prel(pb,pbw,3,1,pal[(p_in[0]>>4)&0xF]);p_in++;

		pb_prel(pb,pbw,2,2,pal[p_in[0]&0xF]);
		pb_prel(pb,pbw,2,3,pal[(p_in[0]>>4)&0xF]);p_in++;
		pb_prel(pb,pbw,3,2,pal[p_in[0]&0xF]);
		pb_prel(pb,pbw,3,3,pal[(p_in[0]>>4)&0xF]);p_in++;
	}
	pixelcvt_next(convPAL8_TW,2,4)
	{
		u8* p_in=(u8*)data;
		u32* pal=&palette_lut[palette_index];

		pb_prel(pb,pbw,0,0,pal[p_in[0]]);p_in++;
		pb_prel(pb,pbw,0,1,pal[p_in[0]]);p_in++;
		pb_prel(pb,pbw,1,0,pal[p_in[0]]);p_in++;
		pb_prel(pb,pbw,1,1,pal[p_in[0]]);p_in++;

		pb_prel(pb,pbw,0,2,pal[p_in[0]]);p_in++;
		pb_prel(pb,pbw,0,3,pal[p_in[0]]);p_in++;
		pb_prel(pb,pbw,1,2,pal[p_in[0]]);p_in++;
		pb_prel(pb,pbw,1,3,pal[p_in[0]]);p_in++;
	}
	pixelcvt_next(convPAL4_X_TW,4,4)
	{
		u8* p_in=(u8*)data;

		pb_prel(pb,pbw,0,0,(p_in[0]&0xF)<<4);
		pb_prel(pb,pbw,0,1,p_in[0]&0xF0);p_in++;
		pb_prel(pb,pbw,1,0,(p_in[0]&0xF)<<4);
		pb_prel(pb,pbw,1,1,p_in[0]&0xF0);p_in++;

		pb_prel(pb,pbw,0,2,(p_in[0]&0xF)<<4);
		pb_prel(pb,pbw,0,3,p_in[0]&0xF0);p_in++;
		pb_prel(pb,pbw,1,2,(p_in[0]&0xF)<<4);
		pb_prel(pb,pbw,1,3,p_in[0]&0xF0);p_in++;

		pb_prel(pb,pbw,2,0,(p_in[0]&0xF)<<4);
		pb_prel(pb,pbw,2,1,p_in[0]&0xF0);p_in++;
		pb_prel(pb,pbw,3,0,(p_in[0]&0xF)<<4);
		pb_prel(pb,pbw,3,1,p_in[0]&0xF0);p_in++;

		pb_prel(pb,pbw,2,2,(p_in[0]&0xF)<<4);
		pb_prel(pb,pbw,2,3,p_in[0]&0xF0);p_in++;
		pb_prel(pb,pbw,3,2,(p_in[0]&0xF)<<4);
		pb_prel(pb,pbw,3,3,p_in[0]&0xF0);p_in++;
	}
	pixelcvt_next(convPAL8_X_TW,2,4)
	{
		u8* p_in=(u8*)data;
#define COL ((p_in[0]&0xF)<<4) | ((p_in[0]&0xF0)<<6)
		pb_prel(pb,pbw,0,0,COL);p_in++;
		pb_prel(pb,pbw,0,1,COL);p_in++;
		pb_prel(pb,pbw,1,0,COL);p_in++;
		pb_prel(pb,pbw,1,1,COL);p_in++;

		pb_prel(pb,pbw,0,2,COL);p_in++;
		pb_prel(pb,pbw,0,3,COL);p_in++;
		pb_prel(pb,pbw,1,2,COL);p_in++;
		pb_prel(pb,pbw,1,3,COL);p_in++;
	}
	pixelcvt_end;

	*/

	//input : address in the yyyyyxxxxx format
	//output : address in the xyxyxyxy format
	//U : x resolution , V : y resolution
	//twidle works on 64b words
	u32 fastcall twiddle_razi(u32 x,u32 y,u32 x_sz,u32 y_sz)
	{
		//u32 rv2=twiddle_optimiz3d(raw_addr,U);
		u32 rv=0;//raw_addr & 3;//low 2 bits are directly passed  -> needs some misc stuff to work.However
		//Pvr internaly maps the 64b banks "as if" they were twidled :p

		//verify(x_sz==y_sz);
		u32 sh=0;
		x_sz>>=1;
		y_sz>>=1;
		while(x_sz!=0 || y_sz!=0)
		{
			if (y_sz)
			{
				u32 temp=y&1;
				rv|=temp<<sh;

				y_sz>>=1;
				y>>=1;
				sh++;
			}
			if (x_sz)
			{
				u32 temp=x&1;
				rv|=temp<<sh;

				x_sz>>=1;
				x>>=1;
				sh++;
			}
		}
		return rv;
	}

#define twop twiddle_razi
	u8 VramWork[1024*1024*2];
	//hanlder functions
	template<class PixelConvertor>
	void fastcall texture_TW(u8* p_in,u32 Width,u32 Height)
	{
//		u32 p=0;
		u8* pb=VramWork;
		//pb->amove(0,0);

		const u32 divider=PixelConvertor::xpp*PixelConvertor::ypp;

		for (u32 y=0;y<Height;y+=PixelConvertor::ypp)
		{
			for (u32 x=0;x<Width;x+=PixelConvertor::xpp)
			{
				u8* p = &p_in[(twop(x,y,Width,Height)/divider)<<3];
				PixelConvertor::Convert((u16*)pb,Width,p);

				//pb->rmovex(PixelConvertor::xpp);
				pb+=PixelConvertor::xpp*2;
			}
			//pb->rmovey(PixelConvertor::ypp);
			pb+=Width*(PixelConvertor::ypp-1)*2;
		}
		memcpy(p_in,VramWork,Width*Height*2);
	}

	template<class PixelConvertor>
	void fastcall texture_VQ(u8* p_in,u32 Width,u32 Height,u8* vq_codebook)
	{
		//p_in+=256*4*2;
//		u32 p=0;
		u8* pb=VramWork;
		//pb->amove(0,0);
		//Convert VQ cb to PAL8
		u16* pal_cb=(u16*)vq_codebook;
		for (u32 palidx=0;palidx<256;palidx++)
		{
			pal_cb[palidx]=PixelConvertor::Convert(&pal_cb[palidx*4]);;
		}
		//Height/=PixelConvertor::ypp;
		//Width/=PixelConvertor::xpp;
		const u32 divider=PixelConvertor::xpp*PixelConvertor::ypp;

		for (u32 y=0;y<Height;y+=PixelConvertor::ypp)
		{
			for (u32 x=0;x<Width;x+=PixelConvertor::xpp)
			{
				u8 p = p_in[twop(x,y,Width,Height)/divider];
				//PixelConvertor::Convert((u16*)pb,Width,&vq_codebook[p*8]);
				*pb=p;

				//pb->rmovex(PixelConvertor::xpp);
				pb+=1;
			}
			//pb->rmovey(PixelConvertor::ypp);
			//pb+=Width*(1-1);
		}
		//align up to 16 bytes
		u32 p_in_int=(u32)p_in;
		p_in_int&=~15;
		p_in=(u8*)p_in_int;

		memcpy(p_in,VramWork,Width*Height/divider);
	}


	void ARGB1555_(u8* praw,u32 w,u32 h)
	{
		u32 sz=w*h;
		u16* ptr=(u16*)praw;
		while(sz--)
			*ptr++=ABGR1555(*ptr);
	//	printf("TX ARGB1555_ 0x%08X;%d\n",praw,sz);
	}
	void ARGB565_(u8* praw,u32 w,u32 h)
	{
		u32 sz=w*h;
		u16* ptr=(u16*)praw;
		while(sz--)
			*ptr++=ABGR0565(*ptr);
	//	printf("TX ARGB565_ 0x%08X;%d\n",praw,sz);
	}
	void ARGB4444_(u8* praw,u32 w,u32 h)
	{
		u32 sz=w*h;
		u16* ptr=(u16*)praw;

		while(sz--)
			*ptr++=ABGR4444(*ptr);
	//	printf("TX ARGB4444_ 0x%08X;%d\n",praw,sz);
	}
	void ARGBYUV422_(u8* praw,u32 w,u32 h)
	{
		u32 sz=w*h*2;
		u16* ptr=(u16*)praw;

		while(sz)
		{
			//*ptr++=ABGR4444(*ptr);

			s32 Y0 = (ptr[0]>>8) &255; //
			s32 Yu = (ptr[0]>>0) &255; //p_in[0]
			s32 Y1 = (ptr[1]>>8) &255; //p_in[3]
			s32 Yv = (ptr[1]>>0) &255; //p_in[2]

			ptr[0]=YUV422(Y0,Yu,Yv);
			ptr[1]=YUV422(Y1,Yu,Yv);;

			ptr+=2;
			sz-=2;
		}
	}

	/*
#ifndef TARGET_PSP
#define GU_PSM_5551 0
#define GU_PSM_5650 1
#define GU_PSM_4444 2
#endif*/
	void SetupPaletteForTexture(u32 palette_index,u32 sz)
	{
		palette_index&=~(sz-1);
		u32 fmtpal=PAL_RAM_CTRL&3;

		if (fmtpal<3)
			palette_index>>=1;

		sceGuClutMode(PalFMT[fmtpal],0,0xFF,0);//or whatever
		sceGuClutLoad(sz/8,&palette_lut[palette_index]);
	}
	static void SetTextureParams(PolyParam* mod)
	{

		#define twidle_tex(format)\
			if (mod->tcw.NO_PAL.VQ_Comp)\
				{\
				vq_codebook=(u8*)&params.vram[sa];\
				sa+=256*4*2;\
				if (mod->tcw.NO_PAL.MipMapped){ /*int* p=0;*p=4;*/\
				sa+=MipPoint[mod->tsp.TexU];}\
				if (*(u32*)&params.vram[(sa&(~0x3))-0]!=0xDEADC0DE)	\
				{		\
					texture_VQ<conv##format##_VQ>/**/((u8*)&params.vram[sa],w,h,vq_codebook);	\
					*(u32*)&params.vram[(sa&(~0x3))-0]=0xDEADC0DE;\
				}\
				sa&=~15; /* ALIGN UP TO 16 BYTES!*/\
				texVQ=1;\
			}\
			else\
			{\
				if (mod->tcw.NO_PAL.MipMapped)\
				sa+=MipPoint[mod->tsp.TexU]<<3;\
				if (*(u32*)&params.vram[sa-0]!=0xDEADC0DE)	\
				{	\
					texture_TW<conv##format##_TW>/*TW*/((u8*)&params.vram[sa],w,h);\
					*(u32*)&params.vram[(sa&(~0x3))-0]=0xDEADC0DE;\
				}\
			}

		#define norm_text(format) \
		if (mod->tcw.NO_PAL.StrideSel) w=512; \
			if (mod->tcw.NO_PAL.StrideSel || *(u32*)&params.vram[sa-0]!=0xDEADC0DE)\
			{	\
			ARGB##format##_((u8*)&params.vram[sa],w,h);	\
				*(u32*)&params.vram[sa-0]=0xDEADC0DE;\
			}

			/*u32 sr;\
			if (mod->tcw.NO_PAL.StrideSel)\
				{sr=(TEXT_CONTROL&31)*32;}\
							else\
				{sr=w;}\
				format((u8*)&params.vram[sa],sr,h);*/

		u32 sa=(mod->tcw.NO_PAL.TexAddr<<3) & VRAM_MASK;
		u32 FMT;
		u32 texVQ=0;
		u8* vq_codebook;
		u32 w=8<<mod->tsp.TexU;
		u32 h=8<<mod->tsp.TexV;

		switch (mod->tcw.NO_PAL.PixelFmt)
		{
		case 0:
		case 7:
			//0	1555 value: 1 bit; RGB values: 5 bits each
			//7	Reserved	Regarded as 1555
			if (mod->tcw.NO_PAL.ScanOrder)
			{
				//verify(tcw.NO_PAL.VQ_Comp==0);
				norm_text(1555);
				//argb1555to8888(&pbt,(u16*)&params.vram[sa],w,h);
			}
			else
			{
				//verify(tsp.TexU==tsp.TexV);
				twidle_tex(1555);
			}
			FMT=GU_PSM_5551;
			break;

			//redo_argb:
			//1	565	 R value: 5 bits; G value: 6 bits; B value: 5 bits
		case 1:
			if (mod->tcw.NO_PAL.ScanOrder)
			{
				//verify(tcw.NO_PAL.VQ_Comp==0);
				norm_text(565);
				//(&pbt,(u16*)&params.vram[sa],w,h);
			}
			else
			{
				//verify(tsp.TexU==tsp.TexV);
				twidle_tex(565);
			}
			FMT=GU_PSM_5650;
			break;


			//2	4444 value: 4 bits; RGB values: 4 bits each
		case 2:
			if (mod->tcw.NO_PAL.ScanOrder)
			{
				//verify(tcw.NO_PAL.VQ_Comp==0);
				//argb4444to8888(&pbt,(u16*)&params.vram[sa],w,h);
				norm_text(4444);
			}
			else
			{
				twidle_tex(4444);
			}
			FMT=GU_PSM_4444;
			break;
			//3	YUV422 32 bits per 2 pixels; YUYV values: 8 bits each
		case 3:
			if (mod->tcw.NO_PAL.ScanOrder)
			{
				norm_text(YUV422);
				//norm_text(ANYtoRAW);
			}
			else
			{
				//it cant be VQ , can it ?
				//docs say that yuv can't be VQ ...
				//HW seems to support it ;p
				twidle_tex(YUV422);
			}
			FMT=GU_PSM_5650;//wha?
			break;
			//4	Bump Map	16 bits/pixel; S value: 8 bits; R value: 8 bits
		case 5:
			//5	4 BPP Palette	Palette texture with 4 bits/pixel
			verify(mod->tcw.PAL.VQ_Comp==0);
			if (mod->tcw.NO_PAL.MipMapped)
				sa+=MipPoint[mod->tsp.TexU]<<1;

			//SetupPaletteForTexture(mod->tcw.PAL.PalSelect<<4,16);
			/*palette_index = tcw.PAL.PalSelect<<4;
			pal_rev=pal_rev_16[tcw.PAL.PalSelect];
			if (settings.Emulation.PaletteMode<2)
			{
				PAL4to8888_TW(&pbt,(u8*)&params.vram[sa],w,h);
			}
			else
			{
				PAL4toX444_TW(&pbt,(u8*)&params.vram[sa],w,h);
			}*/
			FMT=GU_PSM_T4;//wha? the ?
			break;
		case 6:
			{
				//6	8 BPP Palette	Palette texture with 8 bits/pixel
				verify(mod->tcw.PAL.VQ_Comp==0);
				if (mod->tcw.NO_PAL.MipMapped)
					sa+=MipPoint[mod->tsp.TexU]<<2;

				//SetupPaletteForTexture(mod->tcw.PAL.PalSelect<<4,256);
				//

				/*palette_index = (tcw.PAL.PalSelect<<4)&(~0xFF);
				pal_rev=pal_rev_256[tcw.PAL.PalSelect>>4];
				if (settings.Emulation.PaletteMode<2)
				{
				PAL8to8888_TW(&pbt,(u8*)&params.vram[sa],w,h);
				}
				else
				{
				PAL8toX444_TW(&pbt,(u8*)&params.vram[sa],w,h);
				}*/
				FMT=GU_PSM_T8;//wha? the ? FUCK!
			}
			break;
		default:
			printf("Unhandled texture\n");
			//memset(temp_tex_buffer,0xFFEFCFAF,w*h*4);
		}

		if (texVQ)
		{
			//sceGuClutMode(GU_PSM_8888,0,0xff,0); // 32-bit palette
			//sceGuClutLoad((256/8),clut256); // upload 32*8 entries (256)
			//sceGuTexMode(GU_PSM_T8,0,0,0); // 8-bit image
			sceGuClutMode(FMT,0,0xFF,0);
			sceGuClutLoad(256/8,vq_codebook);
			FMT=GU_PSM_T8;
			w>>=1;
			h>>=1;
		}

		sceGuTexMode(FMT,0,0,0);
		sceGuTexImage(0, w>512?512:w, h>512?512:h, w,
			params.vram + sa );

	}
	union _ISP_BACKGND_T_type
	{
		struct
		{
			u32 tag_offset:3;
			u32 tag_address:21;
			u32 skip:3;
			u32 shadow:1;
			u32 cache_bypass:1;
		};
		u32 full;
	};
	union _ISP_BACKGND_D_type
	{
		u32 i;
		f32 f;
	};
	u32 vramlock_ConvOffset32toOffset64(u32 offset32)
	{
		//64b wide bus is archevied by interleaving the banks every 32 bits
		//so bank is Address<<3
		//bits <4 are <<1 to create space for bank num
		//bank 0 is mapped at 400000 (32b offset) and after
		u32 bank=((offset32>>22)&0x1)<<2;//bank will be used as uper offset too
		u32 lv=offset32&0x3; //these will survive
		offset32<<=1;
		//       |inbank offset    |       bank id        | lower 2 bits (not changed)
		u32 rv=  (offset32&(VRAM_MASK-7))|bank                  | lv;

		return rv;
	}
	f32 vrf(u32 addr)
	{
		return *(f32*)&params.vram[vramlock_ConvOffset32toOffset64(addr)];
	}
	u32 vri(u32 addr)
	{
		return *(u32*)&params.vram[vramlock_ConvOffset32toOffset64(addr)];
	}
	static f32 CVT16UV(u32 uv)
	{
		uv<<=16;
		return *(f32*)&uv;
	}
	void decode_pvr_vertex(u32 base,u32 ptr,Vertex* cv)
	{
		//ISP
		//TSP
		//TCW
		ISP_TSP isp;
		TSP tsp;
		TCW tcw;

		isp.full=vri(base);
		tsp.full=vri(base+4);
		tcw.full=vri(base+8);

		//XYZ
		//UV
		//Base Col
		//Offset Col

		//XYZ are _allways_ there :)
		cv->x=vrf(ptr);ptr+=4;
		cv->y=vrf(ptr);ptr+=4;
		cv->z=vrf(ptr);ptr+=4;

		if (isp.Texture)
		{	//Do texture , if any
			if (isp.UV_16b)
			{
				u32 uv=vri(ptr);
				cv->u	=	CVT16UV((u16)uv);
				cv->v	=	CVT16UV((u16)(uv>>16));
				ptr+=4;
			}
			else
			{
				cv->u=vrf(ptr);ptr+=4;
				cv->v=vrf(ptr);ptr+=4;
			}
		}

		//Color
		u32 col=vri(ptr);ptr+=4;
		cv->col=ABGR8888(col);
		if (isp.Offset)
		{
			//Intesity color (can be missing too ;p)
			u32 col=vri(ptr);ptr+=4;
		//	vert_packed_color_(cv->spc,col);
		}
	}

	void reset_vtx_state()
	{

		curVTX=vertices;
		curLST=lists;
		curMod=listModes-1;
		global_regd=false;
		vtx_min_Z=128*1024;//if someone uses more, i realy realy dont care
		vtx_max_Z=0;		//lower than 0 is invalid for pvr .. i wonder if SA knows that.
	}
#if _FULL_PVR_PIPELINE
	template <u32 Type,bool do_sort>
	__forceinline
	void SetGPState(PolyParam* gp,u32 cflip=0)
	{

		//pixel path
		if (gp->pcw.Texture)
		{
			sceGuEnable(GU_TEXTURE_2D);
			SetTextureParams(drawMod);

 /*
  * Set how textures are applied
  *
  * Key for the apply-modes:
  *   - Cv - Color value result
  *   - Ct - Texture color
  *   - Cf - Fragment color
  *   - Cc - Constant color (specified by sceGuTexEnvColor())
  *
  * Available apply-modes are: (TFX)
  *   - GU_TFX_MODULATE - Cv=Ct*Cf TCC_RGB: Av=Af TCC_RGBA: Av=At*Af
  *   - GU_TFX_DECAL - TCC_RGB: Cv=Ct,Av=Af TCC_RGBA: Cv=Cf*(1-At)+Ct*At Av=Af
  *   - GU_TFX_BLEND - Cv=(Cf*(1-Ct))+(Cc*Ct) TCC_RGB: Av=Af TCC_RGBA: Av=At*Af
  *   - GU_TFX_REPLACE - Cv=Ct TCC_RGB: Av=Af TCC_RGBA: Av=At
  *   - GU_TFX_ADD - Cv=Cf+Ct TCC_RGB: Av=Af TCC_RGBA: Av=At*Af
  *
  * The fields TCC_RGB and TCC_RGBA specify components that differ between
  * the two different component modes.
  *
  *   - GU_TFX_MODULATE - The texture is multiplied with the current diffuse fragment
  *   - GU_TFX_REPLACE - The texture replaces the fragment
  *   - GU_TFX_ADD - The texture is added on-top of the diffuse fragment
  *
  * Available component-modes are: (TCC)
  *   - GU_TCC_RGB - The texture alpha does not have any effect
  *   - GU_TCC_RGBA - The texture alpha is taken into account
  *
  * @param tfx - Which apply-mode to use
  * @param tcc - Which component-mode to use
  */
			//dev->SetRenderState(D3DRS_SPECULARENABLE,gp->pcw.Offset );
			switch(gp->tsp.ShadInstr)	// these should be correct, except offset
				{
					//PIXRGB = TEXRGB + OFFSETRGB
					//PIXA    = TEXA
				case 0:	// Decal
					sceGuTexFunc(GU_TFX_DECAL,GU_TCC_RGB);

					if (gp->tsp.IgnoreTexA)
					{
						//a=1
						//how to Ignore ?
					}
					break;

					//The texture color value is multiplied by the Shading Color value.
					//The texture ? value is substituted for the Shading a value.
					//PIXRGB = COLRGB x TEXRGB + OFFSETRGB
					//PIXA   = TEXA
				case 1:	// Modulate
					dev->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
					dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
					dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

					dev->SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);

					if (gp->tsp.IgnoreTexA)
					{
						//a=1
						dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
					}

					break;
					//The texture color value is blended with the Shading Color
					//value according to the texture a value.
					//PIXRGB = (TEXRGB x TEXA) +
					//(COLRGB x (1- TEXA) ) +
					//OFFSETRGB
					//PIXA   = COLA
				case 2:	// Decal Alpha
					if (gp->tsp.IgnoreTexA)
					{
						//Tex.a=1 , so Color = Tex
						dev->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
					}
					else
					{
						dev->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_BLENDTEXTUREALPHA);
					}
					dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
					dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

					dev->SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
					if(gp->tsp.UseAlpha)
					{
						dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
					}
					else
					{
						dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
					}
					break;

					//The texture color value is multiplied by the Shading Color value.
					//The texture a value is multiplied by the Shading a value.
					//PIXRGB= COLRGB x  TEXRGB + OFFSETRGB
					//PIXA   = COLA  x TEXA
				case 3:	// Modulate Alpha
					dev->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
					dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
					dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

					if(gp->tsp.UseAlpha)
					{
						if (gp->tsp.IgnoreTexA)
						{
							//a=Col.a
							dev->SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
						}
						else
						{
							//a=Text.a*Col.a
							dev->SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
						}
					}
					else
					{
						if (gp->tsp.IgnoreTexA)
						{
							//a= 1
							dev->SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTA_TFACTOR);
						}
						else
						{
							//a= Text.a*1
							dev->SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
						}
					}
					dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
					dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

					break;
				}

		}
		else
		{

			//Offset color is enabled olny if Texture is enabled ;)
			sceGuDisable(GU_TEXTURE_2D);
			//sceGuDisable(GU_SPECULAR); // more work
		}


		//misc

		if ( gp->tsp.FilterMode == 0 )
		{
			sceGuTexFilter(GU_NEAREST,GU_NEAREST);
		}
		else
		{
			sceGuTexFilter(GU_LINEAR,GU_LINEAR);
		}



		if (Type==ListType_Translucent)
		{
			//this needs more work actualy ;p
			sceGuBlendFunc(GU_ADD,SrcBlend[gp->tsp.SrcInstr],DstBlend[gp->tsp.DstInstr],0,0);
		}

		//no flipping ;|
		sceGuTexWrap(gp->tsp.ClampU,gp->tsp.ClampV);

		//set cull mode !
		sceGuFrontFace(gp->isp.CullMode&1);
		if (gp->isp.CullMode&2)
			sceGuEnable(GU_CULL_FACE);
		else
			sceGuDisable(GU_CULL_FACE);

		//set Z mode !
		if (Type==ListType_Opaque)
		{
			sceGuDepthFunc(Zfunction[gp->isp.DepthMode]);
		}
		else if (Type==ListType_Translucent)
		{
			if (do_sort)
				sceGuDepthFunc(Zfunction[6]);// : GEQ
			else
				sceGuDepthFunc(Zfunction[gp->isp.DepthMode]);
		}
		else
		{
			sceGuDepthFunc(Zfunction[6]);//PT : Same as trans i guess ;p
		}
		sceGuDepthMask(gp->isp.ZWriteDis);
	}
#endif
	ScePspFVector4 Transform(ScePspFMatrix4* mtx,ScePspFVector3* vec)
	{
		ScePspFVector4 rv;
		rv.x=mtx->x.x*vec->x + mtx->y.x*vec->y + mtx->z.x*vec->z + mtx->w.x;
		rv.y=mtx->x.y*vec->x + mtx->y.y*vec->y + mtx->z.y*vec->z + mtx->w.y;
		rv.z=mtx->x.z*vec->x + mtx->y.z*vec->y + mtx->z.z*vec->z + mtx->w.z;
		rv.w=mtx->x.w*vec->x + mtx->y.w*vec->y + mtx->z.w*vec->z + mtx->w.w;

		return rv;
	}

	#define VTX_TFX(x) (x)
	#define VTX_TFY(y) (y)

	#define PSP_DC_AR_COUNT 6
	//480*(480/272)=847.0588 ~ 847, but we'l use 848.

	float PSP_DC_AR[][2] =
	{
		{640,480},//FS, Streched
		{847,480},//FS, Aspect correct, Extra geom
		{614,460},//NTSC Safe text area (for the most part),Streched
		{812,460},//NTSC Safe text area, Aspect Correct, Extra geom
		{640,362},//Partial H, Apsect correct
		{742,420},//Partial H,Aspect correct, Extra geom
	};
	void palette_update()
	{
		switch(PAL_RAM_CTRL&3)
		{
		case 0:	//1555
		case 1: //565
		case 2: //4444
			{
				u16* p16=(u16*)palette_lut;
				for (int i=0;i<1024;i++)
				{
					p16[i]=(u16)PALETTE_RAM[i];
				}
			}
			break;

		case 3:	//8888
			for (int i=0;i<1024;i++)
			{
				palette_lut[i]=PALETTE_RAM[i];
			}
			break;
		}

	}

	#ifndef GU_SYNC_WHAT_DONE
		#define 	GU_SYNC_WHAT_DONE   (0)
		#define 	GU_SYNC_WHAT_QUEUED   (1)
		#define 	GU_SYNC_WHAT_DRAW   (2)
		#define 	GU_SYNC_WHAT_STALL   (3)
		#define 	GU_SYNC_WHAT_CANCEL   (4)
	#endif

	void DoRender()
	{
		float dc_width,dc_height;
		dc_width=PSP_DC_AR[settings.Enhancements.AspectRatioMode%PSP_DC_AR_COUNT][0];
		dc_height=PSP_DC_AR[settings.Enhancements.AspectRatioMode%PSP_DC_AR_COUNT][1];

		//wait for last frame to end
		sceGuSync(GU_SYNC_FINISH,GU_SYNC_WHAT_DONE);

#if HOST_SYS == SYS_PSP
		
		pspDebugScreenSetOffset((int)fbp0);
		pspDebugScreenSetXY(0,0);
		pspDebugScreenPrintf("%s %0.0f:%0.0f %0.2f",fps_text,dc_width,dc_height,dc_width/dc_height);

		SceCtrlData pad;
		memset(&pad,0,sizeof(pad));
		sceCtrlPeekBufferPositive(&pad, 1);
		if (pad.Buttons & PSP_CTRL_SELECT)
		{
			settings.Enhancements.AspectRatioMode++;
		}
#endif

		fbp0 = sceGuSwapBuffers();

		//--BG poly
		u32 param_base=PARAM_BASE & 0xF00000;
		_ISP_BACKGND_D_type bg_d;
		_ISP_BACKGND_T_type bg_t;

		bg_d.i=ISP_BACKGND_D & ~(0xF);
		bg_t.full=ISP_BACKGND_T;

		bool PSVM=FPU_SHAD_SCALE&0x100; //double parameters for volumes

		//Get the strip base
		u32 strip_base=param_base + bg_t.tag_address*4;
		//Calculate the vertex size
		u32 strip_vs=3 + bg_t.skip;
		u32 strip_vert_num=bg_t.tag_offset;

		if (PSVM && bg_t.shadow)
		{
			strip_vs+=bg_t.skip;//2x the size needed :p
		}
		strip_vs*=4;
		//Get vertex ptr
		u32 vertex_ptr=strip_vert_num*strip_vs+strip_base +3*4;
		//now , all the info is ready :p

		Vertex BGTest;

		decode_pvr_vertex(strip_base,vertex_ptr,&BGTest);


		sceGuStart(GU_DIRECT,list);

		sceGuClearColor(BGTest.col);
		sceGuClearDepth(0);
		sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);


		sceGumMatrixMode(GU_PROJECTION);

		//i create the matrix by hand :p
		ScePspFVector3 v;
		ScePspFMatrix4 mtx;
		

		/*
		Post transform : - i dont know if its [], () or [), i'l bet [) ~
		x : [-1 ..  1)
		y : [ 1 .. -1)
		z : [ 1 .. -1)
*/
			/*
		That gets scaled/translated by viewport, then cliped with offset/scissor (is buffer size relevant to this?)
		*/
		/*
			PowerVR coordinates :
			x : [0,640)
			y : [0,480)
			z : [+inf,0)
		*/
		
		/*
		if (settings.Enhancements.AspectRatioMode==0)
		{
			dc_width=640;
			dc_height=480;
		}
		else
		{


			dc_width=640;
			dc_height=362;
		}*/

		/*
			Perspecive correction :
			3D world coords -> matrix mult -> Clip Coordinates
			Clip Coordinates -> perspective divide (divide xyz by w)-> Normalised Device Coordinates.These can be interpolated linearly

			To interpolate values correctly you need to do

			A=start,B=end
			Aw=w @ start,Bw=w @ end

			//go to linear space
			Ap=A/Aw
			Bp=B/Bw

			//Interpolate on linear space
			Il=Interpolate(Ap,Bp,blend);
			Iw=Interpolate

			//go back to perspective space
			I=Il*Iw;

			|Xr|   |Xx Xy Xz Xw|   |Xv|
			|Yr| = |Yx Yy Yz Yw| x |Yv|
			|Zr|   |Zx Zy Zz Zw|   |Zv|
			|Wr|   |Wx Wy Wz Ww|   | 1|

			Xr= Xx*Xv + Xy+Yv + Xz+Zv + Xw;
			Yr= Yx*Xv + Yy+Yv + Yz+Zv + Yw;
			Zr= Zx*Xv + Zy+Yv + Zz+Zv + Zw;
			Wr= Wx*Xv + Wy+Yv + Wz+Zv + Ww;

			Device Space :	|Xr/Wr|
							|Yr/Wr|
							|Zr/Wr|

			We want :
			Xr=fX*W;
			Yr=fY*W;
			Zr= Some nice, correct Z value.We can only use linear equations here, good for shadows/intersections, bad for Z buffer precition
			Wr=fully mapped 1/invW for perspective !

			->
			Xx=1;X*=0;
			Yy=1;Y*=0;
			Zz=(Z scale);Zw=Z Offset;Z*=0;
			Wz=1;W*=0;

			v2.x=(v.x-320)/320*v2.z;		//-> (v.x-320)*v2.z/320    -> v.x*v2.z/320 - 320*v2.z/320       -> v.x*v2.z/320 - v2.z		 -> v.x*v2.z/320 + (-1)*v2.z
			v2.y=(v.y-240)/(-240)*v2.z;		//-> (v.y-240)*v2.z/(-240) -> v.y*v2.z/(-240) - 240*v2.z/(-240) -> v.y*v2.z/(-240) - (-v2.z) -> v.y*v2.z/(-240) + v2.z

			Zbuffer'=(x*w+y)/w -> x*w/w + y/w -> x + (1/y)*w
		*/

		/*Setup the matrix*/
		/*For x*/
		mtx.x.x=(2.f/dc_width);
		mtx.y.x=0;
		mtx.z.x=-(640/dc_width);
		mtx.w.x=0;

		/*For y*/
		mtx.x.y=0;
		mtx.y.y=-(2.f/dc_height);
		mtx.z.y=(480/dc_height);
		mtx.w.y=0;

		/*For z*/

		//
		//ZBuffer=Zc/Wc
		//Zc=Zv*A+B;	where A=Zscale, B=Zoffset on matrix
		//Wc=Zv;
		//Zbuffer
		//=(Zv*A+B)/Zv
		//=(Zv*A)/Zv + B/Zv
		//=A+B/Zv
		//
		//we want Zb=(1/Zv)*SCL-OFS ->
		//A=-OFS
		//B=SCL
		//OFS : (-1)+(1/Zmax)*SCL

		if (vtx_min_Z<=0.001)
			vtx_min_Z=0.001;
		if (vtx_max_Z<0 || vtx_max_Z>128*1024)
			vtx_max_Z=1;
		float normal_min=vtx_min_Z;
		float normal_max=vtx_max_Z;

		vtx_max_Z*=1.001;//to not clip vtx_max verts
		//vtx_min_Z*=0.990;

		float SCL=-2/(1/vtx_min_Z-1/vtx_max_Z);
		if (SCL<-20)
			SCL=-20;
		mtx.x.z=0;
		mtx.y.z=0;
		mtx.z.z=-((-1)+(1/normal_max)*SCL);
		mtx.w.z=SCL;

		/*For w*/
		mtx.x.w=0;
		mtx.y.w=0;
		mtx.z.w=1;
		mtx.w.w=0;

		vtx_max_Z=normal_max;


		//Load the matrix to gu
		sceGumLoadMatrix(&mtx);

		//clear out other matrixes
		sceGumMatrixMode(GU_VIEW);
		sceGumLoadIdentity();

		sceGumMatrixMode(GU_MODEL);
		sceGumLoadIdentity();
		//push it to the hardware :)
		sceGumUpdateMatrix();

/*
		ScePspFVector3 v2;

		v2.z=vtx_max_Z;
		v2.x=VTX_TFX(128)*v2.z;		//-> (v.x-320)*v2.z/320    -> v.x*v2.z/320 - 320*v2.z/320       -> v.x*v2.z/320 - v2.z		 -> v.x*v2.z/320 + (-1)*v2.z
		v2.y=VTX_TFY(64)*v2.z;		//-> (v.y-240)*v2.z/(-240) -> v.y*v2.z/(-240) - 240*v2.z/(-240) -> v.y*v2.z/(-240) - (-v2.z) -> v.y*v2.z/(-240) + v2.z

		ScePspFVector4 tfx_2=Transform(&mtx,&v2);
		ScePspFVector3 fin_2={tfx_2.x/tfx_2.w,tfx_2.y/tfx_2.w,tfx_2.z/tfx_2.w};
*/
		// Draw triangle
//		u32 lstcount=(curLST-lists);
		Vertex* drawVTX=vertices;
		VertexList* drawLST=lists;
		PolyParam* drawMod=listModes;

		const VertexList* const crLST=curLST;//hint to the compiler that sceGUM cant edit this value !
#if HOST_SYS == SYS_PSP
		sceKernelDcacheWritebackAll();
#endif
		sceGuDisable(GU_BLEND);
		sceGuDisable(GU_ALPHA_TEST);
	//	sceGuDisable(GU_DEPTH_TEST);

		for (;drawLST!=crLST;drawLST++)
		{
			if (drawLST==TransLST)
			{
				//enable blending
				sceGuEnable(GU_BLEND);
				//set blending mode
				sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
				//Disable depth writes
				//sceGuDepthMask(GU_TRUE);
				sceGuEnable(GU_ALPHA_TEST);
				sceGuAlphaFunc(GU_GREATER,0,0xFF);
			}
			s32 count=drawLST->count;
			if (count<0)
			{
				if (drawMod->pcw.Texture)
				{
					//
					sceGuEnable(GU_TEXTURE_2D);
					SetTextureParams(drawMod);

				}
				else
				{
					sceGuDisable(GU_TEXTURE_2D);
				}
				drawMod++;
				count&=0x7FFF;
			}

			sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,count,0,drawVTX);

			drawVTX+=count;
		}

		reset_vtx_state();


		sceGuFinish();
	}

	void StartRender()
	{
		u32 VtxCnt=curVTX-vertices;
		VertexCount+=VtxCnt;

		render_end_pending_cycles= VtxCnt*15;
		if (render_end_pending_cycles<50000)
			render_end_pending_cycles=50000;

		if (FB_W_SOF1 & 0x1000000)
			return;

		DoRender();

		FrameCount++;
	}
	void EndRender()
	{
	}


	//Vertex Decoding-Converting
	struct VertexDecoder
	{
		//list handling
		__forceinline
		static void StartList(u32 ListType)
		{
			if (ListType==ListType_Translucent)
				TransLST=curLST;
		}
		__forceinline
		static void EndList(u32 ListType)
		{

		}

		static u32 FLCOL(float* col)
		{
			u32 A=col[0]*255;
			u32 R=col[1]*255;
			u32 G=col[2]*255;
			u32 B=col[3]*255;
			if (A>255)
				A=255;
			if (R>255)
				R=255;
			if (G>255)
				G=255;
			if (B>255)
				B=255;

			return (A<<24) | (B<<16) | (G<<8) | R;
		}
		static u32 INTESITY(float inte)
		{
			u32 C=inte*255;
			if (C>255)
				C=255;
			return (0xFF<<24) | (C<<16) | (C<<8) | (C);
		}

		//Polys
#define glob_param_bdc  \
			if ( (curVTX-vertices)>38*1024) reset_vtx_state(); \
			if (!global_regd)	curMod++; \
			global_regd=true;			\
			curMod->pcw=pp->pcw;		\
			curMod->isp=pp->isp;		\
			curMod->tsp=pp->tsp;		\
			curMod->tcw=pp->tcw;		\




		__forceinline
		static void fastcall AppendPolyParam0(TA_PolyParam0* pp)
		{
			glob_param_bdc;
		}
		__forceinline
		static void fastcall AppendPolyParam1(TA_PolyParam1* pp)
		{
			glob_param_bdc;
		}
		__forceinline
		static void fastcall AppendPolyParam2A(TA_PolyParam2A* pp)
		{
			glob_param_bdc;
		}
		__forceinline
		static void fastcall AppendPolyParam2B(TA_PolyParam2B* pp)
		{

		}
		__forceinline
		static void fastcall AppendPolyParam3(TA_PolyParam3* pp)
		{
			glob_param_bdc;
		}
		__forceinline
		static void fastcall AppendPolyParam4A(TA_PolyParam4A* pp)
		{
			glob_param_bdc;
		}
		__forceinline
		static void fastcall AppendPolyParam4B(TA_PolyParam4B* pp)
		{

		}

		//Poly Strip handling
		//UPDATE SPRITES ON EDIT !
		__forceinline
		static void StartPolyStrip()
		{
			curLST->ptr=curVTX;
		}

		__forceinline
		static void EndPolyStrip()
		{
			curLST->count=(curVTX-curLST->ptr);
			if (global_regd)
			{
				curLST->count|=0x80000000;
				global_regd=false;
			}
			curLST++;
		}

#define vert_base(dst,_x,_y,_z) /*VertexCount++;*/ \
		float W=1.0f/_z; \
		curVTX[dst].x=VTX_TFX(_x)*W; \
		curVTX[dst].y=VTX_TFY(_y)*W; \
		if (W<vtx_min_Z)	\
			vtx_min_Z=W;	\
		else if (W>vtx_max_Z)	\
			vtx_max_Z=W;	\
		curVTX[dst].z=W; /*Linearly scaled later*/

		//Poly Vertex handlers
#define vert_cvt_base vert_base(0,vtx->xyz[0],vtx->xyz[1],vtx->xyz[2])


		//(Non-Textured, Packed Color)
		__forceinline
		static void AppendPolyVertex0(TA_Vertex0* vtx)
		{
			vert_cvt_base;
			curVTX->col=ABGR8888(vtx->BaseCol);

			curVTX++;
		}

		//(Non-Textured, Floating Color)
		__forceinline
		static void AppendPolyVertex1(TA_Vertex1* vtx)
		{
			vert_cvt_base;
			curVTX->col=FLCOL(&vtx->BaseA);

			curVTX++;
		}

		//(Non-Textured, Intensity)
		__forceinline
		static void AppendPolyVertex2(TA_Vertex2* vtx)
		{
			vert_cvt_base;
			curVTX->col=INTESITY(vtx->BaseInt);

			curVTX++;
		}

		//(Textured, Packed Color)
		__forceinline
		static void AppendPolyVertex3(TA_Vertex3* vtx)
		{
			vert_cvt_base;
			curVTX->col=ABGR8888(vtx->BaseCol);

			curVTX->u=vtx->u;
			curVTX->v=vtx->v;

			curVTX++;
		}

		//(Textured, Packed Color, 16bit UV)
		__forceinline
		static void AppendPolyVertex4(TA_Vertex4* vtx)
		{
			vert_cvt_base;
			curVTX->col=ABGR8888(vtx->BaseCol);

			curVTX->u=CVT16UV(vtx->u);
			curVTX->v=CVT16UV(vtx->v);

			curVTX++;
		}

		//(Textured, Floating Color)
		__forceinline
		static void AppendPolyVertex5A(TA_Vertex5A* vtx)
		{
			vert_cvt_base;

			curVTX->u=vtx->u;
			curVTX->v=vtx->v;
		}
		__forceinline
		static void AppendPolyVertex5B(TA_Vertex5B* vtx)
		{
			curVTX->col=FLCOL(&vtx->BaseA);
			curVTX++;
		}

		//(Textured, Floating Color, 16bit UV)
		__forceinline
		static void AppendPolyVertex6A(TA_Vertex6A* vtx)
		{
			vert_cvt_base;

			curVTX->u=CVT16UV(vtx->u);
			curVTX->v=CVT16UV(vtx->v);
		}
		__forceinline
		static void AppendPolyVertex6B(TA_Vertex6B* vtx)
		{
			curVTX->col=FLCOL(&vtx->BaseA);
			curVTX++;
		}

		//(Textured, Intensity)
		__forceinline
		static void AppendPolyVertex7(TA_Vertex7* vtx)
		{
			vert_cvt_base;
			curVTX->u=vtx->u;
			curVTX->v=vtx->v;

			curVTX->col=INTESITY(vtx->BaseInt);

			curVTX++;
		}

		//(Textured, Intensity, 16bit UV)
		__forceinline
		static void AppendPolyVertex8(TA_Vertex8* vtx)
		{
			vert_cvt_base;
			curVTX->col=INTESITY(vtx->BaseInt);

			curVTX->u=CVT16UV(vtx->u);
			curVTX->v=CVT16UV(vtx->v);

			curVTX++;
		}

		//(Non-Textured, Packed Color, with Two Volumes)
		__forceinline
		static void AppendPolyVertex9(TA_Vertex9* vtx)
		{
			vert_cvt_base;
			curVTX->col=ABGR8888(vtx->BaseCol0);

			curVTX++;
		}

		//(Non-Textured, Intensity,	with Two Volumes)
		__forceinline
		static void AppendPolyVertex10(TA_Vertex10* vtx)
		{
			vert_cvt_base;
			curVTX->col=INTESITY(vtx->BaseInt0);

			curVTX++;
		}

		//(Textured, Packed Color,	with Two Volumes)
		__forceinline
		static void AppendPolyVertex11A(TA_Vertex11A* vtx)
		{
			vert_cvt_base;

			curVTX->u=vtx->u0;
			curVTX->v=vtx->v0;

			curVTX->col=ABGR8888(vtx->BaseCol0);

		}
		__forceinline
		static void AppendPolyVertex11B(TA_Vertex11B* vtx)
		{
			curVTX++;
		}

		//(Textured, Packed Color, 16bit UV, with Two Volumes)
		__forceinline
		static void AppendPolyVertex12A(TA_Vertex12A* vtx)
		{
			vert_cvt_base;

			curVTX->u=CVT16UV(vtx->u0);
			curVTX->v=CVT16UV(vtx->v0);

			curVTX->col=ABGR8888(vtx->BaseCol0);
		}
		__forceinline
		static void AppendPolyVertex12B(TA_Vertex12B* vtx)
		{
			curVTX++;
		}

		//(Textured, Intensity,	with Two Volumes)
		__forceinline
		static void AppendPolyVertex13A(TA_Vertex13A* vtx)
		{
			vert_cvt_base;
			curVTX->u=vtx->u0;
			curVTX->v=vtx->v0;
			curVTX->col=INTESITY(vtx->BaseInt0);
		}
		__forceinline
		static void AppendPolyVertex13B(TA_Vertex13B* vtx)
		{
			curVTX++;
		}

		//(Textured, Intensity, 16bit UV, with Two Volumes)
		__forceinline
		static void AppendPolyVertex14A(TA_Vertex14A* vtx)
		{
			vert_cvt_base;
			curVTX->u=CVT16UV(vtx->u0);
			curVTX->v=CVT16UV(vtx->v0);
			curVTX->col=INTESITY(vtx->BaseInt0);
		}
		__forceinline
		static void AppendPolyVertex14B(TA_Vertex14B* vtx)
		{
			curVTX++;
		}

		//Sprites
		__forceinline
		static void AppendSpriteParam(TA_SpriteParam* spr)
		{
			TA_SpriteParam* pp=spr;
			glob_param_bdc;
		}

		//Sprite Vertex Handlers
		/*
		__forceinline
		static void AppendSpriteVertex0A(TA_Sprite0A* sv)
		{

		}
		__forceinline
		static void AppendSpriteVertex0B(TA_Sprite0B* sv)
		{

		}
		*/
		#define sprite_uv(indx,u_name,v_name) \
		curVTX[indx].u	=	CVT16UV(sv->u_name);\
		curVTX[indx].v	=	CVT16UV(sv->v_name);
		__forceinline
		static void AppendSpriteVertexA(TA_Sprite1A* sv)
		{

			StartPolyStrip();
			curVTX[0].col=0xFFFFFFFF;
			curVTX[1].col=0xFFFFFFFF;
			curVTX[2].col=0xFFFFFFFF;
			curVTX[3].col=0xFFFFFFFF;

			{
			vert_base(2,sv->x0,sv->y0,sv->z0);
			}
			{
			vert_base(3,sv->x1,sv->y1,sv->z1);
			}

			curVTX[1].x=sv->x2;
		}
		__forceinline
		static void AppendSpriteVertexB(TA_Sprite1B* sv)
		{

			{
			vert_base(1,curVTX[1].x,sv->y2,sv->z2);
			}
			{
			vert_base(0,sv->x3,sv->y3,sv->z2);
			}

			sprite_uv(2, u0,v0);
			sprite_uv(3, u1,v1);
			sprite_uv(1, u2,v2);
			sprite_uv(0, u0,v2);//or sprite_uv(u2,v0); ?

			curVTX+=4;
//			VertexCount+=4;

			//EndPolyStrip();
			curLST->count=4;
			if (global_regd)
			{
				curLST->count|=0x80000000;
				global_regd=false;
			}
			curLST++;
		}

		//ModVolumes
		__forceinline
		static void AppendModVolParam(TA_ModVolParam* modv)
		{

		}

		//ModVol Strip handling
		__forceinline
		static void StartModVol(TA_ModVolParam* param)
		{

		}
		__forceinline
		static void ModVolStripEnd()
		{

		}

		//Mod Volume Vertex handlers
		__forceinline
		static void AppendModVolVertexA(TA_ModVolA* mvv)
		{

		}
		__forceinline
		static void AppendModVolVertexB(TA_ModVolB* mvv)
		{

		}
		__forceinline
		static void SetTileClip(u32 xmin,u32 ymin,u32 xmax,u32 ymax)
		{
		}
		__forceinline
		static void TileClipMode(u32 mode)
		{

		}
		//Misc
		__forceinline
		static void ListCont()
		{
		}
		__forceinline
		static void ListInit()
		{
			//reset_vtx_state();
		}
		__forceinline
		static void SoftReset()
		{
			//reset_vtx_state();
		}
	};
	//Setup related

	//Misc setup
	void SetFpsText(char* text)
	{
		strcpy(fps_text,text);
		//if (!IsFullscreen)
		{
			//SetWindowText((HWND)emu.GetRenderTarget(), fps_text);
		}
	}
	bool InitRenderer()
	{
		sceGuInit();

	#if HOST_SYS == SYS_PSP
		pspDebugScreenInit(); //?
		static SceUID DVE_MOD = -1;

		/*
			Enable all the vram
		*/
		int vram_size=pspSetVramSize(4*1024*1024);
		printf("PSP GeEDRAM size : %08X\n",vram_size);

		/* try to init Video Manager PRX */
		DVE_MOD = pspSdkLoadStartModule("dvemgr.prx", PSP_MEMORY_PARTITION_KERNEL);
		printf("dvemgr.prx --> ---> 0x%08X\n",DVE_MOD);
		if (DVE_MOD>=0)
		{
			if (pspDveMgrCheckVideoOut())
			{
				PSP_720_480=true;
				//2 , 0x1d1 -> interlace, composite
				//0 , 0x1d1 -> interlace, component
				//0 , 0x1d2 -> progressive, component
				pspDveMgrSetVideoOut(0, 0x1d2, 720, 480, 1, 15, 0);
			}
		}
	#else
		PSP_720_480=false;
	#endif
		// Setup GU
		if (PSP_720_480)
		{
			BUF_WIDTH=768;//i wonder if this works ?
			SCR_WIDTH=720;
			SCR_HEIGHT=480;
		}

		fbp0 = getStaticVramBuffer(BUF_WIDTH,SCR_HEIGHT,GU_PSM_8888);
		fbp1 = getStaticVramBuffer(BUF_WIDTH,SCR_HEIGHT,GU_PSM_8888);
		zbp = getStaticVramBuffer(BUF_WIDTH,SCR_HEIGHT,GU_PSM_4444);


		sceGuStart(GU_DIRECT,list);
		sceGuDrawBuffer(GU_PSM_8888,fbp0,BUF_WIDTH);
		sceGuDispBuffer(SCR_WIDTH,SCR_HEIGHT,fbp1,BUF_WIDTH);
		sceGuDepthBuffer(zbp,BUF_WIDTH);
		sceGuOffset(2048 - (SCR_WIDTH/2),2048 - (SCR_HEIGHT/2));
		sceGuViewport(2048,2048,SCR_WIDTH,SCR_HEIGHT);
		sceGuDepthRange(65535,0);
		sceGuScissor(0,0,SCR_WIDTH,SCR_HEIGHT);
		sceGuEnable(GU_SCISSOR_TEST);
		sceGuEnable(GU_DEPTH_TEST);
		sceGuDepthMask(GU_FALSE);
		sceGuDepthFunc(GU_GEQUAL);
		sceGuFrontFace(GU_CW);
		sceGuDisable(GU_CULL_FACE);
		sceGuShadeModel(GU_SMOOTH);
		sceGuDisable(GU_TEXTURE_2D);
		sceGuTexFunc( GU_TFX_MODULATE, GU_TCC_RGBA );	// Apply image as a decal (NEW)
		//sceGuTexFunc( GU_TFX_DECAL, GU_TCC_RGB );	// Apply image as a decal (NEW)
		sceGuTexFilter( GU_LINEAR, GU_LINEAR );		// Linear filtering (Good Quality) (NEW)
		sceGuTexScale( 1.0f, 1.0f );                    // No scaling
		sceGuTexOffset( 0.0f, 0.0f );

		sceGuFinish();
		sceGuSync(0,0);

//		sceDisplayWaitVblankStart();
		sceGuDisplay(1);

		return TileAccel.Init();
	}

	void TermRenderer()
	{
		#if HOST_SYS == SYS_PSP
		sceGuTerm();
		if (PSP_720_480)
		{
			pspDveMgrSetVideoOut(0, 0, 480, 272, 1, 15, 0);
		}
		#endif
		TileAccel.Term();
	}

	void ResetRenderer(bool Manual)
	{
		TileAccel.Reset(Manual);
		VertexCount=0;
		FrameCount=0;
	}

	bool ThreadStart()
	{
		return true;
	}

	void ThreadEnd()
	{

	}
	void ListCont()
	{
		TileAccel.ListCont();
	}
	void ListInit()
	{
		TileAccel.ListInit();
	}
	void SoftReset()
	{
		TileAccel.SoftReset();
	}

	void VramLockedWrite(vram_block* bl)
	{

	}
}
using namespace NORenderer;

#if HOST_SYS==SYS_PSP
void pspguCrear()
{
	sceGuStart(GU_DIRECT,list);
	sceGuClearColor(0);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0,0);

	pspDebugScreenSetOffset((int)fbp0);
}
void pspguWaitVblank()
{
	sceDisplayWaitVblankStart();
	fbp0 = sceGuSwapBuffers();
}
#endif

#endif