#include "config.h"

#include "gxRend.h"
#if REND_API == REND_WII || 1
#include <gccore.h>
#include <malloc.h>

#define DEFAULT_FIFO_SIZE	(256*1024)

#include "regs.h"

using namespace TASplitter;

#define DEFAULT_FIFO_SIZE	(256*1024)

u8* vram_buffer;

static void *frameBuffer[2] = { NULL, NULL};
static GXRModeObj *rmode;
static u8 gp_fifo[DEFAULT_FIFO_SIZE] __attribute__((aligned(32)));

#define ABGR8888(x) ((x&0xFF00FF00) |((x>>16)&0xFF) | ((x&0xFF)<<16))
/*
	FMT: 1555 DTP    -> RGB5A3 DP
		 4444 DTP    -> RGB5A3 DP

		 8888 P      -> 8888 P
		 565  DTP    -> 565 DP
		 YUV  DT     -> ? 565 is possibe but LQ ...

*/

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

struct TextureCacheDesc
{
	GXTexObj tex;
	GXTlutObj pal;
	u32 addr;
	bool has_pal;
};
void VBlank() { }

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


	char fps_text[512];

	struct VertexDecoder;
	FifoSplitter<VertexDecoder> TileAccel;

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
	u32 vramlock_ConvOffset32toOffset64(u32 offset32);
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
		cv->col=col;//ABGR8888(col);
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

	#define VTX_TFX(x) (x)
	#define VTX_TFY(y) (y)

	//input : address in the yyyyyxxxxx format
	//output : address in the xyxyxyxy format
	//U : x resolution , V : y resolution
	//twidle works on 64b words
	u32 fastcall twop(u32 x,u32 y,u32 x_sz,u32 y_sz)
	{
		u32 rv=0;

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
	
	//hanlder functions

	u32 GX_TexOffs(u32 x,u32 y,u32 w)
	{
		w/=4;
		u32 xs=x&3;
		x>>=2;
		u32 ys=y&3;
		y>>=2;

		return (y*w+x)*16 + (ys*4+xs);
	}
	/*
	void fastcall texture_TW(u8* p_out,u8* p_in,u32 Width,u32 Height)
	{
		u16* dst=(u16*)p_out;
		const u32 divider=2*2;

		for (u32 y=0;y<Height;y+=2)
		{
			for (u32 x=0;x<Width;x+=2)
			{
				u8* p = &p_in[((twop(x,y,Width,Height)/divider)<<3)];
				u16* src=(u16*)p;

				dst[GX_TexOffs(x,y,Width)]=(src[1]);
				dst[GX_TexOffs(x+1,y,Width)]=(src[0]);
				dst[GX_TexOffs(x,y+1,Width)]=(src[3]);
				dst[GX_TexOffs(x+1,y+1,Width)]=(src[2]);
			}
		}
	}

	/*
	void SetTextureParams(PolyParam* mod)
	{
		//whuzz!
		GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);

		u32 tex_addr=(mod->tcw.NO_PAL.TexAddr<<3)&VRAM_MASK;
		u32* ptex=(u32*)&params.vram[tex_addr];
		TextureCacheDesc* pbuff=((TextureCacheDesc*)&vram_buffer[tex_addr*2])-1;

		if (*ptex!=0xDEADBEEF || pbuff->addr!=tex_addr)
		{
			u32* dst=(u32*)&pbuff[1];
			u32 sz=(8<<mod->tsp.TexU) * (8<<mod->tsp.TexV)*2;

			if (mod->tcw.NO_PAL.ScanOrder)
				memcpy(dst,ptex,sz);
			else
				texture_TW((u8*)dst,(u8*)ptex,8<<mod->tsp.TexU,8<<mod->tsp.TexV);

			//setup ..
			GX_InitTexObj(&pbuff->tex,dst,8<<mod->tsp.TexU,8<<mod->tsp.TexV
				,PVR2GX[mod->tcw.NO_PAL.PixelFmt],GX_REPEAT,GX_REPEAT,GX_FALSE);

			printf("Texture:%d %d %dx%d %08X --> %08X\n",mod->tcw.NO_PAL.PixelFmt,mod->tcw.NO_PAL.ScanOrder,8<<mod->tsp.TexU,8<<mod->tsp.TexV,tex_addr,dst);
			pbuff->addr=tex_addr;
			*ptex=0xDEADBEEF;
		}

		GX_LoadTexObj(&pbuff->tex,GX_TEXMAP0);
	}
	*/

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

	#define MAKE_555A3

	#define MAKE_565(r,g,b,a) 
	#define MAKE_1555(r,g,b,a)
	#define MAKE_4444(r,g,b,a)

	#define ABGR8888(x) ((x&0xFF00FF00) |((x>>16)&0xFF) | ((x&0xFF)<<16))
	#define ABGR4444(x) ((x&0xF0F0) |((x>>8)&0xF) | ((x&0xF)<<8))
	#define ABGR0565(x) ((x&(0x3F<<5)) |((x>>11)&0x1F) | ((x&0x1F)<<11))
	#define ABGR1555(x) ((x&0x83E0) |((x>>10)&0x1F) | ((x&0x1F)<<10))




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

	//pixel convertors !
#define pixelcvt_start(name,xa,yb)  \
	struct name \
	{ \
	static const u32 xpp=xa;\
	static const u32 ypp=yb;	\
	__forceinline static void fastcall Convert(u16* pb,u32 x,u32 y,u32 pbw,u8* data) \
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

	inline void pb_prel(u16* dst,u32 pbw,u32 x,u32 y,u32 col)
	{
		dst[GX_TexOffs(x,y,pbw)]=col;
	}
	//Non twiddled
	pixelcvt_start(conv565_PL,4,1)
	{
		//convert 4x1 565 to 4x1 8888
		u16* p_in=(u16*)data;
		//0,0
		pb_prel(pb,pbw,x+0,y,ABGR0565(p_in[0]));
		//1,0
		pb_prel(pb,pbw,x+1,y,ABGR0565(p_in[1]));
		//2,0
		pb_prel(pb,pbw,x+2,y,ABGR0565(p_in[2]));
		//3,0
		pb_prel(pb,pbw,x+3,y,ABGR0565(p_in[3]));
	}
	pixelcvt_next(conv1555_PL,4,1)
	{
		//convert 4x1 1555 to 4x1 8888
		u16* p_in=(u16*)data;
		//0,0
		pb_prel(pb,pbw,x+0,y,ABGR1555(p_in[0]));
		//1,0
		pb_prel(pb,pbw,x+1,y,ABGR1555(p_in[1]));
		//2,0
		pb_prel(pb,pbw,x+2,y,ABGR1555(p_in[2]));
		//3,0
		pb_prel(pb,pbw,x+3,y,ABGR1555(p_in[3]));
	}
	pixelcvt_next(conv4444_PL,4,1)
	{
		//convert 4x1 4444 to 4x1 8888
		u16* p_in=(u16*)data;
		//0,0
		pb_prel(pb,pbw,x+0,y,ABGR4444(p_in[0]));
		//1,0
		pb_prel(pb,pbw,x+1,y,ABGR4444(p_in[1]));
		//2,0
		pb_prel(pb,pbw,x+2,y,ABGR4444(p_in[2]));
		//3,0
		pb_prel(pb,pbw,x+3,y,ABGR4444(p_in[3]));
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
		pb_prel(pb,pbw,x+0,y,YUV422(Y0,Yu,Yv));
		//1,0
		pb_prel(pb,pbw,x+1,y,YUV422(Y1,Yu,Yv));

		//next 4 bytes
		p_in+=1;

		Y0 = (p_in[0]>>8) &255; //
		Yu = (p_in[0]>>0) &255; //p_in[0]
		Y1 = (p_in[0]>>24) &255; //p_in[3]
		Yv = (p_in[0]>>16) &255; //p_in[2]

		//0,0
		pb_prel(pb,pbw,x+2,y,YUV422(Y0,Yu,Yv));
		//1,0
		pb_prel(pb,pbw,x+3,y,YUV422(Y1,Yu,Yv));
	}
	pixelcvt_end;
	//twiddled
	pixelcvt_start(conv565_TW,2,2)
	{
		//convert 4x1 565 to 4x1 8888
		u16* p_in=(u16*)data;
		//0,0
		pb_prel(pb,pbw,x+0,y+0,ABGR0565(p_in[0]));
		//0,1
		pb_prel(pb,pbw,x+0,y+1,ABGR0565(p_in[1]));
		//1,0
		pb_prel(pb,pbw,x+1,y+0,ABGR0565(p_in[2]));
		//1,1
		pb_prel(pb,pbw,x+1,y+1,ABGR0565(p_in[3]));
	}
	pixelcvt_next(conv1555_TW,2,2)
	{
		//convert 4x1 1555 to 4x1 8888
		u16* p_in=(u16*)data;
		//0,0
		pb_prel(pb,pbw,x+0,y+0,ABGR1555(p_in[0]));
		//0,1
		pb_prel(pb,pbw,x+0,y+1,ABGR1555(p_in[1]));
		//1,0
		pb_prel(pb,pbw,x+1,y+0,ABGR1555(p_in[2]));
		//1,1
		pb_prel(pb,pbw,x+1,y+1,ABGR1555(p_in[3]));
	}
	pixelcvt_next(conv4444_TW,2,2)
	{
		//convert 4x1 4444 to 4x1 8888
		u16* p_in=(u16*)data;
		//0,0
		pb_prel(pb,pbw,x+0,y+0,ABGR4444(p_in[0]));
		//0,1
		pb_prel(pb,pbw,x+0,y+1,ABGR4444(p_in[1]));
		//1,0
		pb_prel(pb,pbw,x+1,y+0,ABGR4444(p_in[2]));
		//1,1
		pb_prel(pb,pbw,x+1,y+1,ABGR4444(p_in[3]));
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
		pb_prel(pb,pbw,x+0,y+0,YUV422(Y0,Yu,Yv));
		//1,0
		pb_prel(pb,pbw,x+1,y+0,YUV422(Y1,Yu,Yv));

		//next 4 bytes
		//p_in+=2;

		Y0 = (p_in[1]>>8) &255; //
		Yu = (p_in[1]>>0) &255; //p_in[0]
		Y1 = (p_in[3]>>8) &255; //p_in[3]
		Yv = (p_in[3]>>0) &255; //p_in[2]

		//0,1
		pb_prel(pb,pbw,x+0,y+1,YUV422(Y0,Yu,Yv));
		//1,1
		pb_prel(pb,pbw,x+1,y+1,YUV422(Y1,Yu,Yv));
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

		return YUV422(16+((Y0-16)+(Y1-16))/2.0f,Yu,Yv);
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
	u8* VramWork;
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
				PixelConvertor::Convert((u16*)pb,x,y,Width,p);

				//pb->rmovex(PixelConvertor::xpp);
				//pb+=PixelConvertor::xpp*2;
			}
			//pb->rmovey(PixelConvertor::ypp);
			//pb+=Width*(PixelConvertor::ypp-1)*2;
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
				//*pb=p;
				pb[GX_TexOffs(x,y,Width)]=p;
				//pb->rmovex(PixelConvertor::xpp);
				//pb+=1;
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

	template<int type>
	void Plannar(u8* praw,u32 w,u32 h)
	{
		u16* ptr=(u16*)praw;
		u16* dst=(u16*)VramWork;
		u32 x,y;

		for (y=0;y<h;y++)
		{
			for (x=0;x<w;x++)
			{
				u32 col=*ptr++;
				switch(type)
				{
					case 1555:
						dst[GX_TexOffs(x,y,w)]=ABGR1555(col);
						break;
					case 565:
						dst[GX_TexOffs(x,y,w)]=ABGR0565(col);
						break;
					case 4444:
						dst[GX_TexOffs(x,y,w)]=ABGR4444(col);
						break;
					case 422:
						{
							s32 Y0 = (col>>8) &255; //
							s32 Yu = (col>>0) &255; //p_in[0]
							u32 col=*ptr++;
							s32 Y1 = (col>>8) &255; //p_in[3]
							s32 Yv = (col>>0) &255; //p_in[2]

							dst[GX_TexOffs(x,y,w)]=YUV422(Y0,Yu,Yv);
							x++;
							dst[GX_TexOffs(x,y,w)]=YUV422(Y1,Yu,Yv);
						}
						break;
				}
			}
		}
	}


	void SetupPaletteForTexture(u32 palette_index,u32 sz)
	{
		palette_index&=~(sz-1);
		u32 fmtpal=PAL_RAM_CTRL&3;

		if (fmtpal<3)
			palette_index>>=1;

		//sceGuClutMode(PalFMT[fmtpal],0,0xFF,0);//or whatever
		//sceGuClutLoad(sz/8,&palette_lut[palette_index]);
	}

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
	#define twidle_tex(format)\
			if (mod->tcw.NO_PAL.VQ_Comp)\
				{\
				vq_codebook=(u8*)&params.vram[tex_addr];\
				tex_addr+=256*4*2;\
				if (mod->tcw.NO_PAL.MipMapped){ /*int* p=0;*p=4;*/\
				tex_addr+=MipPoint[mod->tsp.TexU];}\
				texture_VQ<conv##format##_VQ>/**/((u8*)&params.vram[tex_addr],w,h,vq_codebook);	\
				texVQ=1;\
			}\
			else\
			{\
				if (mod->tcw.NO_PAL.MipMapped)\
				tex_addr+=MipPoint[mod->tsp.TexU]<<3;\
				texture_TW<conv##format##_TW>/*TW*/((u8*)&params.vram[tex_addr],w,h); \
			}

		#define norm_text(format) \
		if (mod->tcw.NO_PAL.StrideSel) w=512; \
			Plannar<format>((u8*)&params.vram[tex_addr],w,h);	\

			/*u32 sr;\
			if (mod->tcw.NO_PAL.StrideSel)\
				{sr=(TEXT_CONTROL&31)*32;}\
							else\
				{sr=w;}\
				format((u8*)&params.vram[tex_addr],sr,h);*/

	int TexUV(u32 flip,u32 clamp)
	{
		if (clamp)
			return GX_CLAMP;
		else if (flip)
			return GX_MIRROR;
		else
			return GX_REPEAT;
	}

	static void SetTextureParams(PolyParam* mod)
	{
		GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);

		u32 tex_addr=(mod->tcw.NO_PAL.TexAddr<<3)&VRAM_MASK;
		u32* ptex=(u32*)&params.vram[tex_addr];
		TextureCacheDesc* pbuff=((TextureCacheDesc*)&vram_buffer[tex_addr*2])-1;

		u32 FMT;
		u32 texVQ=0;
		u8* vq_codebook;
		u32 w=8<<mod->tsp.TexU;
		u32 h=8<<mod->tsp.TexV;

#if 0	//old code
		if (*ptex!=0xDEADBEEF || pbuff->addr!=tex_addr)
		{
			u32* dst=(u32*)&pbuff[1];
			u32 sz=(8<<mod->tsp.TexU) * (8<<mod->tsp.TexV)*2;

			if (mod->tcw.NO_PAL.ScanOrder)
				memcpy(dst,ptex,sz);
			else
				texture_TW((u8*)dst,(u8*)ptex,8<<mod->tsp.TexU,8<<mod->tsp.TexV);

			//setup ..

			printf("Texture:%d %d %dx%d %08X --> %08X\n",mod->tcw.NO_PAL.PixelFmt,mod->tcw.NO_PAL.ScanOrder,8<<mod->tsp.TexU,8<<mod->tsp.TexV,tex_addr,dst);
			pbuff->addr=tex_addr;
			*ptex=0xDEADBEEF;
		}
#endif
		if (*ptex!=0xDEADBEEF || pbuff->addr!=tex_addr || (mod->tcw.NO_PAL.StrideSel&&mod->tcw.NO_PAL.ScanOrder))
		{
			u32* dst=(u32*)&pbuff[1];
			VramWork=(u8*)dst;
			pbuff->has_pal=false;
			pbuff->addr=tex_addr;

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
				}
				else
				{
					//verify(tsp.TexU==tsp.TexV);
					twidle_tex(1555);
				}
				FMT=GX_TF_RGB5A3;
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
				FMT=GX_TF_RGB565;
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
				FMT=GX_TF_RGB5A3;
				break;
				//3	YUV422 32 bits per 2 pixels; YUYV values: 8 bits each
			case 3:
				if (mod->tcw.NO_PAL.ScanOrder)
				{
					norm_text(422);
					//norm_text(ANYtoRAW);
				}
				else
				{
					//it cant be VQ , can it ?
					//docs say that yuv can't be VQ ...
					//HW seems to support it ;p
					twidle_tex(YUV422);
				}
				FMT=GX_TF_RGB565;//wha?
				break;
				//4	Bump Map	16 bits/pixel; S value: 8 bits; R value: 8 bits
			case 5:
				//5	4 BPP Palette	Palette texture with 4 bits/pixel
				verify(mod->tcw.PAL.VQ_Comp==0);
				if (mod->tcw.NO_PAL.MipMapped)
					tex_addr+=MipPoint[mod->tsp.TexU]<<1;

				SetupPaletteForTexture(mod->tcw.PAL.PalSelect<<4,16);

				FMT=GX_TF_I4;//wha? the ?
				break;
			case 6:
				{
					//6	8 BPP Palette	Palette texture with 8 bits/pixel
					verify(mod->tcw.PAL.VQ_Comp==0);
					if (mod->tcw.NO_PAL.MipMapped)
						tex_addr+=MipPoint[mod->tsp.TexU]<<2;

					SetupPaletteForTexture(mod->tcw.PAL.PalSelect<<4,256);

					FMT=GX_TF_I8;//wha? the ? FUCK!
				}
				break;
			default:
				printf("Unhandled texture\n");
				//memset(temp_tex_buffer,0xFFEFCFAF,w*h*4);
			}

			if (texVQ)
			{
				GX_InitTlutObj(&pbuff->pal,vq_codebook,FMT,256);
				FMT=GX_TF_I8;
				w>>=1;
				h>>=1;
				pbuff->has_pal=true;
			}

//			sceGuTexMode(FMT,0,0,0);
//			sceGuTexImage(0, w>512?512:w, h>512?512:h, w,
//				params.vram + sa );

			GX_InitTexObj(&pbuff->tex,dst,w,h,FMT,TexUV(mod->tsp.FlipU,mod->tsp.ClampU),
				TexUV(mod->tsp.FlipV,mod->tsp.ClampV),GX_FALSE);
			*ptex=0xDEADBEEF;
			printf("Texture:%d %d %dx%d %08X --> %08X\n",mod->tcw.NO_PAL.PixelFmt,mod->tcw.NO_PAL.ScanOrder,8<<mod->tsp.TexU,8<<mod->tsp.TexV,tex_addr,dst);
		}

		GX_LoadTexObj(&pbuff->tex,GX_TEXMAP0);

		if (pbuff->has_pal)
			GX_LoadTlut(&pbuff->pal,GX_TLUT0);

	}

	void DoRender()
	{
		float dc_width,dc_height;
		dc_width=640;
		dc_height=480;

		VIDEO_SetBlack(FALSE);
		GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
		GX_InvVtxCache();
		GX_InvalidateTexAll();

		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

		GX_SetNumChans(1);
		GX_SetNumTexGens(1);

		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

		GX_ClearVtxDesc();
		GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
		GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

		GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);


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

		GX_SetCopyClear((GXColor&)BGTest.col,0x00000000);

		GX_SetZMode(GX_TRUE, GX_GEQUAL, GX_TRUE);
		GX_SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
		GX_SetAlphaUpdate(GX_TRUE);
		GX_SetColorUpdate(GX_TRUE);

		/*
		x'=x*xx + y*xy + z* xz + xw
		y'=x*yx + y*yy + z* yz + yw
		z'=x*zx + y*zy + z* zz + zw
		w'=x*wx + y*wy + z* wz + ww

		cx=x'/w'
		cy=y'/w'
		cz=z'/w'

		invW  = [+inf,0]
		w     = [0,+inf]

		post transform : -1 1
		w'=w
		z = w
		z' = A*w+B

		z'/w = A + B/w

		A + B*invW

		mapped to [0 1]
		min(invW)=1/max(W)
		max(invW)=1/min(W)

		A + B * max(invW) = vnear
		A + B * min(invW) = vfar

		A=-B*max(invW) + vnear

		-B*max(invW) + B*min(invW) = vfar
		B*(min(invW)-max(invW))=vfar
		B=vfar/(min(invW)-max(invW))

	*/

		//sanitise values
		if (vtx_min_Z<=0.001)
			vtx_min_Z=0.001;
		if (vtx_max_Z<0 || vtx_max_Z>128*1024)
			vtx_max_Z=1;

		//extend range
		vtx_max_Z*=1.001;//to not clip vtx_max verts
		//vtx_min_Z*=0.999;

		//convert to [-1 .. 0]
		float p6=-1/(1/vtx_max_Z-1/vtx_min_Z);
		float p5=p6/vtx_min_Z;
		

		Mtx44 mtx =
		{
			{(2.f/dc_width)  ,0                ,+(640/dc_width)              ,0}  ,
			{0               ,-(2.f/dc_height) ,-(480/dc_height)             ,0}  ,
			{0               ,0                ,p5							 ,p6}  ,
			{0               ,0                ,-1                           ,0}
		};


		//load the matrix to GX
		GX_LoadProjectionMtx(mtx, GX_PERSPECTIVE);


		//clear out other matrixes
		Mtx modelview;
		guMtxIdentity(modelview);
		GX_LoadPosMtxImm(modelview,GX_PNMTX0);

		Vertex* drawVTX=vertices;
		VertexList* drawLST=lists;
		PolyParam* drawMod=listModes;

		const VertexList* const crLST=curLST;//hint to the compiler that sceGUM cant edit this value !

		GX_SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);

//		GX_SetAlphaCompare(GX_GREATER,0,GX_AOP,GX_ALWAYS,0);

		GX_SetTevOp(GX_TEVSTAGE0,GX_MODULATE);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

		for (;drawLST!=crLST;drawLST++)
		{
			if (drawLST==TransLST)
			{
				//enable blending & blending mode
				GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);

				//setup alpha compare
			}

			s32 count=drawLST->count;
			if (count<0)
			{
				if (drawMod->pcw.Texture)
				{

					SetTextureParams(drawMod);
				}
				else
				{
					GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
				}
				
				drawMod++;
				count&=0x7FFF;
			}

			if (count)
			{
				GX_Begin(GX_TRIANGLESTRIP,GX_VTXFMT0,count);
				while(count--)
				{
					GX_Position3f32(drawVTX->x,drawVTX->y,-drawVTX->z);
					GX_Color1u32(HOST_TO_LE32(drawVTX->col));
					GX_TexCoord2f32(drawVTX->u,drawVTX->v);
					drawVTX++;
				}
				GX_End();
			}

			//sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,count,0,drawVTX);

//			drawVTX+=count;
		}

		reset_vtx_state();

		static int fb=1;
		GX_DrawDone();
		GX_CopyDisp(frameBuffer[fb],GX_TRUE);

		VIDEO_SetNextFramebuffer(frameBuffer[fb]);

		VIDEO_Flush();
		//VIDEO_WaitVSync(); //why bother ?
		//fb ^= 1;		// flip framebuffer
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
		printf(text);
		//if (!IsFullscreen)
		{
			//SetWindowText((HWND)emu.GetRenderTarget(), fps_text);
		}
	}
	bool InitRenderer()
	{
        GXColor background = {0, 0, 0, 0xff};

		VIDEO_Init();

		rmode = VIDEO_GetPreferredMode(NULL);
		
        switch (rmode->viTVMode >> 2)
        {
               case VI_NTSC: // 480 lines (NTSC 60hz)
                       break;
               case VI_PAL: // 576 lines (PAL 50hz)
                       rmode = &TVPal576IntDfScale;
                       rmode->xfbHeight = 480;
                       rmode->viYOrigin = (VI_MAX_HEIGHT_PAL - 480)/2;
                       rmode->viHeight = 480;
                       break;
               default: // 480 lines (PAL 60Hz)
                       break;
        }

        VIDEO_Configure(rmode);

		int fb = 0;

		// allocate 2 framebuffers for double buffering
		frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
		frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

		VIDEO_Configure(rmode);
		VIDEO_SetNextFramebuffer(frameBuffer[fb]);
		VIDEO_SetBlack(TRUE);
		VIDEO_Flush();
		VIDEO_WaitVSync();
		if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

		fb ^= 1;
		
		// setup the fifo and then init the flipper
		memset(gp_fifo,0,DEFAULT_FIFO_SIZE);

		GX_Init(gp_fifo,DEFAULT_FIFO_SIZE);

		// clears the bg to color and clears the z buffer
		//GX_SetCopyClear(background, 0x00ffffff);

		// other gx setup
		GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
		f32 yscale = GX_GetYScaleFactor(rmode->efbHeight,rmode->xfbHeight);
		u32 xfbHeight = GX_SetDispCopyYScale(yscale);
		GX_SetScissor(0,0,rmode->fbWidth,rmode->efbHeight);
		GX_SetDispCopySrc(0,0,rmode->fbWidth,rmode->efbHeight);
		GX_SetDispCopyDst(rmode->fbWidth,xfbHeight);
		GX_SetCopyFilter(rmode->aa,rmode->sample_pattern,GX_TRUE,rmode->vfilter);
		GX_SetFieldMode(rmode->field_rendering,((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

		if (rmode->aa)
			GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
		else
			GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);


		GX_SetCullMode(GX_CULL_NONE);
		GX_CopyDisp(frameBuffer[fb],GX_TRUE);
		GX_SetDispCopyGamma(GX_GM_1_0);


		// setup the vertex descriptor
		// tells the flipper to expect direct data


		return TileAccel.Init();
	}

	void TermRenderer()
	{
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

#include <vector>
#include <string>
using namespace std;


int GetFile(char *szFileName, char *szParse=0,u32 flags=0)
{
	if (FILE* f=fopen("/boot.cdi","rb"))
	{
		fclose(f);
		strcpy(szFileName,"/boot.cdi");
		return 1;
	}
	else
		return 0;
}
#endif