float dc_width=640;
float dc_height=480;

#include "../../config.h"
#include "plugins/plugin_manager.h"

#ifndef SUPPORT_X11
#define SUPPORT_X11 0==1
#else
#define SUPPORT_X11 1==1
#endif

#if HOST_OS == OS_LINUX && SUPPORT_X11
	#include "X11/Xlib.h"
	#include "X11/Xutil.h"
	#define WINDOW_WIDTH	640
	#define WINDOW_HEIGHT	480
#elif HOST_OS == OS_WINDOWS
	#include <windows.h>
#endif

#include "nullRend.h"

#if REND_API == REND_GLES2

#if HOST_OS == OS_WINDOWS
#ifndef GL_NV_draw_path
//IMGTEC GLES emulation
#pragma comment(lib,"libEGL.lib")
#pragma comment(lib,"libGLESv2.lib")
#else /* NV gles emulation*/
#pragma comment(lib,"libGLES20.lib")

#endif

	LRESULT CALLBACK WndProc2(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
			/*
				Here we are handling 2 system messages: screen saving and monitor power.
				They are especially relevent on mobile devices.
			*/
			case WM_SYSCOMMAND:
			{
				switch (wParam)
				{
					case SC_SCREENSAVE:					// Screensaver trying to start ?
					case SC_MONITORPOWER:				// Monitor trying to enter powersave ?
					return 0;							// Prevent this from happening
				}
				break;
			}
			// Handles the close message when a user clicks the quit icon of the window
			case WM_CLOSE:
				PostQuitMessage(0);
				return 1;

			default:
				break;
		}

		// Calls the default window procedure for messages we did not handle
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	// Windows class name to register
	#define	WINDOW_CLASS _T("PVRShellClass")

	// Width and height of the window
	#define WINDOW_WIDTH	640
	#define WINDOW_HEIGHT	480
#endif

#ifdef _PS3
	#include <PSGL/psgl.h>
	#include <PSGL/psglu.h>
	#include <Cg/cg.h>

	#include <sys/process.h>
#include <sys/spu_initialize.h>
#include <sys/paths.h>
#include <sysutil/sysutil_sysparam.h>

#include <PSGL/psgl.h>
#include <PSGL/psglu.h>

#include <cell/dbgfont.h>

#include <stdarg.h>
#include <cell/dbgfont.h>
#include <cell/sysmodule.h>

GLuint 		gfxWidth = 1280;
GLuint 		gfxHeight = 720;
GLfloat     gfxAspectRatio = 16.0f/9.0f;

void gfxCheckCgError(int line);
#else
#ifndef _ANDROID
	#include <EGL/egl.h>
#endif
	#include <GLES2/gl2.h>
#endif

#include "regs.h"

struct
{
	struct
	{
#if !defined(_PS3) && !defined(_ANDROID)
		EGLDisplay  Display;
		EGLConfig   Config;
		EGLSurface  Surface;
		EGLContext  Context;
#else
		//right
#endif
	} setup;
	struct
	{
		GLuint vbo;
	} res;
} gl;

#ifndef _ANDROID
bool TestEGLError(const char* pszLocation);
#endif

void gfxCheckGlError(int line);

#ifdef _PS3
	typedef CGprogram program_t;
	CGparameter uiProgramMatrix;
	CGparameter uiTextureTarget;

	CGparameter vtx_pos,vtx_basecol,vtx_offscol,vtx_uv;
#else
	typedef GLuint program_t;
	GLuint uiProgramMatrix[512];			//matrix id
#endif

program_t uiFragShader, uiVertShader;      // Used to hold the fragment and vertex shader handles
program_t uiProgramObject[512];                 // Used to hold the program handle


u32 GetShaderNum(u32 UseAlpha,u32 Texture,u32 IgnoreTexA,u32 ShadInstr,u32 Offset)
{
	u32 rv=0;
	rv|=UseAlpha;rv<<=1;
	rv|=Texture;rv<<=1;
	rv|=IgnoreTexA;rv<<=2;
	rv|=ShadInstr;rv<<=1;
	rv|=Offset;

	return rv;
}




using namespace TASplitter;

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

#define ABGR8888(x) (x)

#pragma pack(push,1)

struct Vertex
{
	float x, y, z;
	float u,v;
    unsigned int col;
	unsigned int ofs;
};

#pragma pack(pop)


struct VertexList
{
	union
	{
		struct
		{
#if HOST_ENDIAN==ENDIAN_LITTLE
			u16 count;
			u16 flags;
#else
			u16 flags;
			u16 count;
#endif
		};
		u32 flags_count;
	};
};

struct PolyParam
{
	PCW pcw;
	ISP_TSP isp;

	TSP tsp;
	TCW tcw;
};

struct DisplayList
{
	u32 vtxnum;
	VertexList* start,* end;
	PolyParam* pp;

	void reset()
	{
		vtxnum=0;
		start=end=0;
		pp=0;
	}

	void begin(u32 vtxnum,VertexList* start,PolyParam* pp)
	{
		reset();

		this->vtxnum=vtxnum;
		this->start=start;
		this->pp=pp;
	}
	void finish(VertexList* end)
	{
		this->end=end;
	}
	bool valid() const { return end!=0 && start!=0; }
};

struct
{
	//Data
	struct
	{
		Vertex ALIGN16  vertices[42*1024];
		VertexList  ALIGN16 lists[8*1024];
		PolyParam  ALIGN16 pparams[8*1024];
		float vtx_min_Z;
		float vtx_max_Z;

		void reset()
		{
			vtx_min_Z=128*1024;//if someone uses more, i realy realy dont care
			vtx_max_Z=0;		//lower than 0 is invalid for pvr .. i wonder if SA knows that.
		}
	} data;

	//decoder (TASplitter) running state
	struct
	{
		Vertex* vtx;
		VertexList* lst;
		PolyParam* pp;

		Vertex* first_vtx;

		void reset(Vertex* vtx,VertexList* lst,PolyParam* pp)
		{
			this->vtx=vtx;
			this->lst=lst;
			this->pp=pp;
			first_vtx=0;
		}
	} dec;

	//Lists
	DisplayList op,pt,tr;

	//reset the context
	void reset()
	{
		data.reset();
		dec.reset(data.vertices,data.lists,data.pparams);
		op.reset();
		pt.reset();
		tr.reset();
	}
} ta_ctx;

struct TexCache
{
	struct TexCacheEntry
	{
	    GLuint format;
		GLuint texture;
		u32 sa;

		void Init(GLuint tex)
		{
			texture = tex;
			sa=0xFFFFFFFF;

			glBindTexture(GL_TEXTURE_2D, texture);

			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		void Term()
		{
			glDeleteTextures(1,&texture);
		}

		void Generate(TCW tcw,TSP tsp);
	};
	TexCacheEntry cache[512];
	void Init()
	{
		GLuint t[512];
		glGenTextures(512,t);

		for (u32 i=0;i<512;i++)
		{
			cache[i].Init(t[i]);
		}
	}

	void Term()
	{
		for (u32 i=0;i<512;i++)
		{
			cache[i].Term();
		}
	}
	TexCacheEntry* GetFree()
	{
		for (u32 i=0;i<512;i++)
		{
			if (cache[i].sa==0xFFFFFFFF)
				return &cache[i];
		}
		u32 ff=rand()%512;
		printf("rand %d\n",ff);
		return &cache[ff];
	}

	void BindTexture(TCW tcw,TSP tsp)
	{
		u32 sa=(tcw.NO_PAL.TexAddr<<3) & VRAM_MASK;

		u16* ptr=(u16*)&params.vram[sa];
		TexCacheEntry* e=&cache[ptr[3]&511];

		//found the texture ?
		if (*(u32*)ptr==0xDEADC0DE && e->sa==sa)
		{
			glBindTexture(GL_TEXTURE_2D, e->texture); gfxCheckGlError(__LINE__);
		}
		else
		{
			//if not, get a free one
			e=GetFree();
			//and bind/fill it
			e->Generate(tcw,tsp);
		}
#if _PS3
		//printf("Texture %08X : %08X\n",uiTextureTarget,e->texture);
		cgGLSetTextureParameter(uiTextureTarget,e->texture); gfxCheckCgError(__LINE__);
		cgGLEnableTextureParameter(uiTextureTarget); gfxCheckCgError(__LINE__);
#endif
	}
};

TexCache Textures;



#define DECODE_TEX
#ifdef  DECODE_TEX

bool DecodeAndLoad(u8* ptex, u32 texid, TCW tcw, TSP tsp);

#endif




//I LOVE C++ HAHAHA *NOT*
void TexCache::TexCacheEntry::Generate(TCW tcw,TSP tsp)
{
	u32 sa=(tcw.NO_PAL.TexAddr<<3) & VRAM_MASK;
	this->sa=sa;

	u16* ptr=(u16*)&params.vram[sa];

#ifdef  DECODE_TEX
    if(!DecodeAndLoad((u8*)ptr, texture, tcw, tsp))
        printf("Error, failed to decode texture!\n");
#else
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,8<<tsp.TexU,8<<tsp.TexV,0,GL_RGBA,format,ptr);
#endif

	u32 text=this-Textures.cache;
	printf("mktexture %d\n",text);

	*(u32*)ptr=0xDEADC0DE;
	ptr[3]=text;
}

char fps_text[512];

struct VertexDecoder;
FifoSplitter<VertexDecoder> TileAccel;

static void SetTextureParams(const PolyParam* mod)
{
	Textures.BindTexture(mod->tcw,mod->tsp);
	//what more ?

    //Sorry, no texturing for now
}

union _ISP_BACKGND_T_type
{
	struct
	{
#if HOST_ENDIAN == ENDIAN_LITTLE
		u32 tag_offset:3;
		u32 tag_address:21;
		u32 skip:3;
		u32 shadow:1;
		u32 cache_bypass:1;
		u32 rsvd:3;
#else
		u32 rsvd:3;
		u32 cache_bypass:1;
		u32 shadow:1;
		u32 skip:3;
		u32 tag_address:21;
		u32 tag_offset:3;
#endif
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
	cv->col=ABGR8888(col);
	if (isp.Offset)
	{
		//Intesity color (can be missing too ;p)
		cv->ofs=ABGR8888(vri(ptr));ptr+=4;
	//	vert_packed_color_(cv->spc,col);
	}
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

u8 codeblock[1024];
u16 *pCB = (u16*)codeblock;


void SetShader(u32 id);
void SetupScene(float TheMatrix[]);
#ifndef _ANDROID
void SwapBuffersNow();
#endif
template<int mode>
void DrawLists(const DisplayList& tl)
{
	u32 last_program=0xFFFFFFFF;

	if (!tl.valid())
		return;
#ifdef _PS3
	if (mode!=0)
	{
		glEnable(GL_ALPHA_TEST); gfxCheckGlError(__LINE__);
		if (mode==1)
		{
			glAlphaFunc(GL_GEQUAL,PT_ALPHA_REF/255.0f); gfxCheckGlError(__LINE__);
		}
		else
		{
			glAlphaFunc(GL_GEQUAL,1/255.f); gfxCheckGlError(__LINE__);
		}
	}
	else
	{
		glDisable(GL_ALPHA_TEST); gfxCheckGlError(__LINE__);
	}
#else
	//fix alpha test
#endif
	const PolyParam* pp=tl.pp;
	u32 vtx=tl.vtxnum;

	for (const VertexList* lst=tl.start;lst!=tl.end;lst++)
	{
		//printf("PRIM\n");
		s32 flags_count=lst->flags_count;
		u32 count=(u16)flags_count;
		if (flags_count<0)
		{
			u32 progid=GetShaderNum(pp->tsp.UseAlpha,pp->pcw.Texture,pp->tsp.IgnoreTexA,pp->tsp.ShadInstr,pp->pcw.Offset);
			if (progid!=last_program)
			{
				SetShader(progid);
				last_program=progid;
			}
			if (pp->pcw.Texture)
			{
#ifdef _PS3
					uiTextureTarget=cgGetNamedParameter(uiProgramObject[last_program], "texImg"); gfxCheckCgError(__LINE__);
#endif
					SetTextureParams(pp);
			}

			pp++;
		}

		glDrawArrays(GL_TRIANGLE_STRIP, vtx, count); gfxCheckGlError(__LINE__);
		gfxCheckGlError(__LINE__);

		vtx+=count;
	}
}
void DoRender()
{
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

	/*
        For the maths look on the psp side
		PowerVR coordinates :
		x : [0,640)
		y : [0,480)
		z : [+inf,0)
	*/


	//Setup the matrix

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

	//sanitise the values
	if (ta_ctx.data.vtx_min_Z<=0.001)
		ta_ctx.data.vtx_min_Z=0.001;
	if (ta_ctx.data.vtx_max_Z<0 || ta_ctx.data.vtx_max_Z>128*1024)
		ta_ctx.data.vtx_max_Z=1;

	//add some extra range to avoid cliping border cases
	ta_ctx.data.vtx_min_Z*=0.998;
	ta_ctx.data.vtx_max_Z*=1.002;

	float vnear=0;
	float vfar =1;

	float max_invW=1/ta_ctx.data.vtx_min_Z;
	float min_invW=1/ta_ctx.data.vtx_max_Z;

	float B=vfar/(min_invW-max_invW);
	float A=-B*max_invW+vnear;

    float TheMatrix[] =
    {
        (2.f/dc_width)  ,0                ,-(640/dc_width)              ,0  ,
        0               ,-(2.f/dc_height) ,(480/dc_height)              ,0  ,
        0               ,0                ,A							,B  ,
        0               ,0                ,1                            ,0
    };

//this is useful for debugging
//	printf("The Matrix %5.2f %5.2f\n",max_invW,min_invW);
//	float* mtx=TheMatrix;
//    for (int j=0;j<4;j++,mtx+=4)
//        printf("%5.2f | %5.2f  | %5.2f  | %5.2f \n",mtx[0],mtx[1],mtx[2],mtx[3]);
	SetupScene(TheMatrix);

	// Draw triangle

	u8 bB=BGTest.col;
	u8 bG=BGTest.col/256;
	u8 bR=BGTest.col/256/256;
	u8 bA=BGTest.col/256/256/256;

	glDepthMask(GL_TRUE); gfxCheckGlError(__LINE__);
	glClearColor(bR/255.f, bG/255.f, bB/255.f, bA/255.f);  gfxCheckGlError(__LINE__);
    glClearDepthf(1.f); gfxCheckGlError(__LINE__);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); gfxCheckGlError(__LINE__);

    glDisable(GL_BLEND); gfxCheckGlError(__LINE__);
//      glDisable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST); gfxCheckGlError(__LINE__);
	glDepthFunc(GL_LEQUAL); gfxCheckGlError(__LINE__);
	glDepthMask(GL_TRUE); gfxCheckGlError(__LINE__);
	//glDepthRangef(0,1);



#ifdef _PS3
	/*
	glEnableClientState(GL_VERTEX_ARRAY); gfxCheckGlError(__LINE__);
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &((Vertex*)0)->x); gfxCheckGlError(__LINE__);

	glClientActiveTexture (GL_TEXTURE0); gfxCheckGlError(__LINE__);
	glEnableClientState (GL_TEXTURE_COORD_ARRAY); gfxCheckGlError(__LINE__);
	glTexCoordPointer (4, GL_BYTE, sizeof(Vertex), &((Vertex*)0)->col); gfxCheckGlError(__LINE__);
	
	glClientActiveTexture (GL_TEXTURE1); gfxCheckGlError(__LINE__);
	glEnableClientState (GL_TEXTURE_COORD_ARRAY); gfxCheckGlError(__LINE__);
	glTexCoordPointer (4, GL_BYTE, sizeof(Vertex), &((Vertex*)0)->ofs); gfxCheckGlError(__LINE__);

	glClientActiveTexture (GL_TEXTURE2); gfxCheckGlError(__LINE__);
	glEnableClientState (GL_TEXTURE_COORD_ARRAY); gfxCheckGlError(__LINE__);
	glTexCoordPointer (2, GL_FLOAT, sizeof(Vertex), &((Vertex*)0)->u); gfxCheckGlError(__LINE__);
	*/

	cgGLSetParameterPointer(vtx_pos,3,GL_FLOAT,sizeof(Vertex),&ta_ctx.data.vertices->x);  gfxCheckCgError(__LINE__);
	cgGLEnableClientState(vtx_pos); gfxCheckCgError(__LINE__);

	cgGLSetParameterPointer(vtx_basecol,4,GL_UNSIGNED_BYTE,sizeof(Vertex),&ta_ctx.data.vertices->col); gfxCheckCgError(__LINE__);
	cgGLEnableClientState(vtx_basecol); gfxCheckCgError(__LINE__);

	cgGLSetParameterPointer(vtx_offscol,4,GL_UNSIGNED_BYTE,sizeof(Vertex),&ta_ctx.data.vertices->ofs); gfxCheckCgError(__LINE__);
	cgGLEnableClientState(vtx_offscol); gfxCheckCgError(__LINE__);

	cgGLSetParameterPointer(vtx_uv,2,GL_FLOAT,sizeof(Vertex),&ta_ctx.data.vertices->u); gfxCheckCgError(__LINE__);
	cgGLEnableClientState(vtx_uv); gfxCheckCgError(__LINE__);


#else
	glBindBuffer(GL_ARRAY_BUFFER, gl.res.vbo); gfxCheckGlError(__LINE__);
	glBufferData(GL_ARRAY_BUFFER,(u8*)ta_ctx.dec.vtx-(u8*)ta_ctx.data.vertices,ta_ctx.data.vertices,GL_STREAM_DRAW); gfxCheckGlError(__LINE__);

	#define VERTEX_ARRAY 0
    #define COLOR_ARRAY 1
	#define TEXCOORD_ARRAY 2
	#define COLOR2_ARRAY 3

	glEnableVertexAttribArray(VERTEX_ARRAY);
	glVertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), &((Vertex*)0)->x);	//they had to bring the ptr uglyness to es too eh ?

	glEnableVertexAttribArray(COLOR_ARRAY);
	glVertexAttribPointer(COLOR_ARRAY, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), &((Vertex*)0)->col);

	glEnableVertexAttribArray(COLOR2_ARRAY);
	glVertexAttribPointer(COLOR2_ARRAY, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), &((Vertex*)0)->ofs);

	glEnableVertexAttribArray(TEXCOORD_ARRAY);
	glVertexAttribPointer(TEXCOORD_ARRAY, 2, GL_FLOAT, GL_TRUE, sizeof(Vertex), &((Vertex*)0)->u);
#endif

	DrawLists<0>(ta_ctx.op);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	DrawLists<1>(ta_ctx.pt);

	DrawLists<2>(ta_ctx.tr);

	ta_ctx.reset();

#ifndef _ANDROID
	SwapBuffersNow();
	TestEGLError("DoRender");
#endif
}

void StartRender()
{
	u32 VtxCnt=ta_ctx.dec.vtx-ta_ctx.data.vertices;
	VertexCount+=VtxCnt;

	render_end_pending_cycles= VtxCnt*15;
	if (render_end_pending_cycles<50000)
		render_end_pending_cycles=50000;


	if (FB_W_SOF1 & 0x1000000)
		return;

	DoRender();

	FrameCount++;

	#if HOST_OS == OS_WINDOWS
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// If the message is WM_QUIT, exit the while loop
			if (msg.message == WM_QUIT)
				break;

			// Translate the message and dispatch it to WindowProc()
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	#endif
}
void EndRender()
{
#ifdef _ANDROID
	sh4_cpu.Stop();
#endif
}


extern u16 kcode[4];
extern u8 rt[4],lt[4];

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

float vjoy_pos[11][2]=
{
	{24+0,24+64},
	{24+64,24+0},
	{24+128,24+64},
	{24+64,24+128},

	{440+0,280+64},
	{440+64,280+0},
	{440+128,280+64},
	{440+64,280+128},

	{320-32,360+32},
	{24+0,360+32},
	{24+64+24,360+32},
};
//Vertex Decoding-Converting
struct VertexDecoder
{
	static void DrawButton(float* xy, u32 state)
	{
		TA_Vertex1 vtx;
		float x=xy[0];
		float y=xy[1];

		if (state)
			vtx.BaseA=0.3f;
		else
			vtx.BaseA=0.6f;

		vtx.BaseR=0.5f;
		vtx.BaseG=0.5f;
		vtx.BaseB=0.5f;
		vtx.xyz[2]=0.5;

		StartPolyStrip();
		vtx.xyz[0]=x; vtx.xyz[1]=y;
		AppendPolyVertex1(&vtx);
		vtx.xyz[0]=x+64; vtx.xyz[1]=y;
		AppendPolyVertex1(&vtx);
		vtx.xyz[0]=x; vtx.xyz[1]=y+64;
		AppendPolyVertex1(&vtx);
		vtx.xyz[0]=x+64; vtx.xyz[1]=y+64;
		AppendPolyVertex1(&vtx);
		EndPolyStrip();
	}

	static void OSD_HOOK()
	{
		TA_PolyParam0 pp;
		
		pp.pcw.full=0;
		
		pp.isp.full=0;
		pp.isp.DepthMode=7;
		pp.isp.ZWriteDis=1;
		pp.isp.CullMode=0;
		
		//these aren't fully set, and it works. It'l breaky if i ever implement more of the pvr pipeline :p
		pp.tsp.full=0;
		pp.tsp.UseAlpha=1;
		pp.tcw.full=0;
		
		AppendPolyParam0(&pp);
		

		DrawButton(vjoy_pos[0],kcode[0]&key_CONT_DPAD_LEFT);
		DrawButton(vjoy_pos[1],kcode[0]&key_CONT_DPAD_UP);
		DrawButton(vjoy_pos[2],kcode[0]&key_CONT_DPAD_RIGHT);
		DrawButton(vjoy_pos[3],kcode[0]&key_CONT_DPAD_DOWN);

		DrawButton(vjoy_pos[4],kcode[0]&key_CONT_X);
		DrawButton(vjoy_pos[5],kcode[0]&key_CONT_Y);
		DrawButton(vjoy_pos[6],kcode[0]&key_CONT_B);
		DrawButton(vjoy_pos[7],kcode[0]&key_CONT_A);

		DrawButton(vjoy_pos[8],kcode[0]&key_CONT_START);

		DrawButton(vjoy_pos[9],lt[0]<64);

		DrawButton(vjoy_pos[10],rt[0]<64);

		ta_ctx.dec.first_vtx=0;
		ta_ctx.dec.pp++;
	}
	//list handling
	__forceinline
	static void StartList(u32 ListType)
	{
		if (ListType==ListType_Opaque)
			ta_ctx.op.begin(ta_ctx.dec.vtx-ta_ctx.data.vertices,ta_ctx.dec.lst,ta_ctx.dec.pp);
		else if (ListType==ListType_Punch_Through)
			ta_ctx.pt.begin(ta_ctx.dec.vtx-ta_ctx.data.vertices,ta_ctx.dec.lst,ta_ctx.dec.pp);
		else if (ListType==ListType_Translucent)
			ta_ctx.tr.begin(ta_ctx.dec.vtx-ta_ctx.data.vertices,ta_ctx.dec.lst,ta_ctx.dec.pp);
	}
	__forceinline
	static void EndList(u32 ListType)
	{
		if (ta_ctx.dec.first_vtx)
		{
			ta_ctx.dec.first_vtx=0;
			ta_ctx.dec.pp++;
		}

		if (ListType==ListType_Opaque)
			ta_ctx.op.finish(ta_ctx.dec.lst);
		else if (ListType==ListType_Punch_Through)
			ta_ctx.pt.finish(ta_ctx.dec.lst);
		else if (ListType==ListType_Translucent)
		{
			OSD_HOOK();
			ta_ctx.tr.finish(ta_ctx.dec.lst);
		}
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

		return (A<<24) | (R<<16) | (G<<8) | B;
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
		if ( (ta_ctx.dec.vtx-ta_ctx.data.vertices)>38*1024) ta_ctx.reset(); \
		if (ta_ctx.dec.first_vtx)	ta_ctx.dec.pp++; \
		ta_ctx.dec.first_vtx=0;			\
		ta_ctx.dec.pp->pcw=pp->pcw;		\
		ta_ctx.dec.pp->isp=pp->isp;		\
		ta_ctx.dec.pp->tsp=pp->tsp;		\
		ta_ctx.dec.pp->tcw=pp->tcw;




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
		if (!ta_ctx.dec.first_vtx)
			ta_ctx.dec.lst->flags=0x8000;
		else
			ta_ctx.dec.lst->flags=0;

		ta_ctx.dec.first_vtx=ta_ctx.dec.vtx;
	}

	__forceinline
	static void EndPolyStrip()
	{
		ta_ctx.dec.lst->count=(ta_ctx.dec.vtx-ta_ctx.dec.first_vtx);
		ta_ctx.dec.lst++;
	}

#define vert_base(dst,_x,_y,_z) /*VertexCount++;*/ \
	float W=1.0f/_z; \
	ta_ctx.dec.vtx[dst].x=VTX_TFX(_x)*W; \
	ta_ctx.dec.vtx[dst].y=VTX_TFY(_y)*W; \
	if (W<ta_ctx.data.vtx_min_Z)	\
		ta_ctx.data.vtx_min_Z=W;	\
	else if (W>ta_ctx.data.vtx_max_Z)	\
		ta_ctx.data.vtx_max_Z=W;	\
	ta_ctx.dec.vtx[dst].z=W; /*Linearly scaled later*/

	//Poly Vertex handlers
#define vert_cvt_base vert_base(0,vtx->xyz[0],vtx->xyz[1],vtx->xyz[2])


	//(Non-Textured, Packed Color)
	__forceinline
	static void AppendPolyVertex0(TA_Vertex0* vtx)
	{
		vert_cvt_base;
		ta_ctx.dec.vtx->col=ABGR8888(vtx->BaseCol);

		ta_ctx.dec.vtx++;
	}

	//(Non-Textured, Floating Color)
	__forceinline
	static void AppendPolyVertex1(TA_Vertex1* vtx)
	{
		vert_cvt_base;
		ta_ctx.dec.vtx->col=FLCOL(&vtx->BaseA);

		ta_ctx.dec.vtx++;
	}

	//(Non-Textured, Intensity)
	__forceinline
	static void AppendPolyVertex2(TA_Vertex2* vtx)
	{
		vert_cvt_base;
		ta_ctx.dec.vtx->col=INTESITY(vtx->BaseInt);

		ta_ctx.dec.vtx++;
	}

	//(Textured, Packed Color)
	__forceinline
	static void AppendPolyVertex3(TA_Vertex3* vtx)
	{
		vert_cvt_base;
		ta_ctx.dec.vtx->col=ABGR8888(vtx->BaseCol);
		ta_ctx.dec.vtx->ofs=ABGR8888(vtx->OffsCol);

		ta_ctx.dec.vtx->u=vtx->u;
		ta_ctx.dec.vtx->v=vtx->v;

		ta_ctx.dec.vtx++;
	}

	//(Textured, Packed Color, 16bit UV)
	__forceinline
	static void AppendPolyVertex4(TA_Vertex4* vtx)
	{
		vert_cvt_base;
		ta_ctx.dec.vtx->col=ABGR8888(vtx->BaseCol);
		ta_ctx.dec.vtx->ofs=ABGR8888(vtx->OffsCol);

		ta_ctx.dec.vtx->u=CVT16UV(vtx->u);
		ta_ctx.dec.vtx->v=CVT16UV(vtx->v);

		ta_ctx.dec.vtx++;
	}

	//(Textured, Floating Color)
	__forceinline
	static void AppendPolyVertex5A(TA_Vertex5A* vtx)
	{
		vert_cvt_base;

		ta_ctx.dec.vtx->u=vtx->u;
		ta_ctx.dec.vtx->v=vtx->v;
	}
	__forceinline
	static void AppendPolyVertex5B(TA_Vertex5B* vtx)
	{
		ta_ctx.dec.vtx->col=FLCOL(&vtx->BaseA);
		ta_ctx.dec.vtx->ofs=FLCOL(&vtx->OffsA);
		ta_ctx.dec.vtx++;
	}

	//(Textured, Floating Color, 16bit UV)
	__forceinline
	static void AppendPolyVertex6A(TA_Vertex6A* vtx)
	{
		vert_cvt_base;

		ta_ctx.dec.vtx->u=CVT16UV(vtx->u);
		ta_ctx.dec.vtx->v=CVT16UV(vtx->v);
	}
	__forceinline
	static void AppendPolyVertex6B(TA_Vertex6B* vtx)
	{
		ta_ctx.dec.vtx->col=FLCOL(&vtx->BaseA);
		ta_ctx.dec.vtx->ofs=FLCOL(&vtx->OffsA);
		ta_ctx.dec.vtx++;
	}

	//(Textured, Intensity)
	__forceinline
	static void AppendPolyVertex7(TA_Vertex7* vtx)
	{
		vert_cvt_base;
		ta_ctx.dec.vtx->u=vtx->u;
		ta_ctx.dec.vtx->v=vtx->v;

		ta_ctx.dec.vtx->col=INTESITY(vtx->BaseInt);
		ta_ctx.dec.vtx->ofs=INTESITY(vtx->OffsInt);

		ta_ctx.dec.vtx++;
	}

	//(Textured, Intensity, 16bit UV)
	__forceinline
	static void AppendPolyVertex8(TA_Vertex8* vtx)
	{
		vert_cvt_base;
		ta_ctx.dec.vtx->col=INTESITY(vtx->BaseInt);
		ta_ctx.dec.vtx->ofs=INTESITY(vtx->OffsInt);

		ta_ctx.dec.vtx->u=CVT16UV(vtx->u);
		ta_ctx.dec.vtx->v=CVT16UV(vtx->v);

		ta_ctx.dec.vtx++;
	}

	//(Non-Textured, Packed Color, with Two Volumes)
	__forceinline
	static void AppendPolyVertex9(TA_Vertex9* vtx)
	{
		vert_cvt_base;
		ta_ctx.dec.vtx->col=ABGR8888(vtx->BaseCol0);

		ta_ctx.dec.vtx++;
	}

	//(Non-Textured, Intensity,	with Two Volumes)
	__forceinline
	static void AppendPolyVertex10(TA_Vertex10* vtx)
	{
		vert_cvt_base;
		ta_ctx.dec.vtx->col=INTESITY(vtx->BaseInt0);

		ta_ctx.dec.vtx++;
	}

	//(Textured, Packed Color,	with Two Volumes)
	__forceinline
	static void AppendPolyVertex11A(TA_Vertex11A* vtx)
	{
		vert_cvt_base;

		ta_ctx.dec.vtx->u=vtx->u0;
		ta_ctx.dec.vtx->v=vtx->v0;

		ta_ctx.dec.vtx->col=ABGR8888(vtx->BaseCol0);
		ta_ctx.dec.vtx->ofs=ABGR8888(vtx->OffsCol0);

	}
	__forceinline
	static void AppendPolyVertex11B(TA_Vertex11B* vtx)
	{
		ta_ctx.dec.vtx++;
	}

	//(Textured, Packed Color, 16bit UV, with Two Volumes)
	__forceinline
	static void AppendPolyVertex12A(TA_Vertex12A* vtx)
	{
		vert_cvt_base;

		ta_ctx.dec.vtx->u=CVT16UV(vtx->u0);
		ta_ctx.dec.vtx->v=CVT16UV(vtx->v0);

		ta_ctx.dec.vtx->col=ABGR8888(vtx->BaseCol0);
		ta_ctx.dec.vtx->ofs=ABGR8888(vtx->OffsCol0);
	}
	__forceinline
	static void AppendPolyVertex12B(TA_Vertex12B* vtx)
	{
		ta_ctx.dec.vtx++;
	}

	//(Textured, Intensity,	with Two Volumes)
	__forceinline
	static void AppendPolyVertex13A(TA_Vertex13A* vtx)
	{
		vert_cvt_base;
		ta_ctx.dec.vtx->u=vtx->u0;
		ta_ctx.dec.vtx->v=vtx->v0;
		ta_ctx.dec.vtx->col=INTESITY(vtx->BaseInt0);
		ta_ctx.dec.vtx->ofs=INTESITY(vtx->OffsInt0);
	}
	__forceinline
	static void AppendPolyVertex13B(TA_Vertex13B* vtx)
	{
		ta_ctx.dec.vtx++;
	}

	//(Textured, Intensity, 16bit UV, with Two Volumes)
	__forceinline
	static void AppendPolyVertex14A(TA_Vertex14A* vtx)
	{
		vert_cvt_base;
		ta_ctx.dec.vtx->u=CVT16UV(vtx->u0);
		ta_ctx.dec.vtx->v=CVT16UV(vtx->v0);
		ta_ctx.dec.vtx->col=INTESITY(vtx->BaseInt0);
		ta_ctx.dec.vtx->ofs=INTESITY(vtx->OffsInt0);
	}
	__forceinline
	static void AppendPolyVertex14B(TA_Vertex14B* vtx)
	{
		ta_ctx.dec.vtx++;
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
	ta_ctx.dec.vtx[indx].u	=	CVT16UV(sv->u_name);\
	ta_ctx.dec.vtx[indx].v	=	CVT16UV(sv->v_name);
	__forceinline
	static void AppendSpriteVertexA(TA_Sprite1A* sv)
	{

		StartPolyStrip();
		ta_ctx.dec.vtx[0].col=0xFFFFFFFF;
		ta_ctx.dec.vtx[1].col=0xFFFFFFFF;
		ta_ctx.dec.vtx[2].col=0xFFFFFFFF;
		ta_ctx.dec.vtx[3].col=0xFFFFFFFF;

		{
		vert_base(2,sv->x0,sv->y0,sv->z0);
		}
		{
		vert_base(3,sv->x1,sv->y1,sv->z1);
		}

		ta_ctx.dec.vtx[1].x=sv->x2;
	}
	__forceinline
	static void AppendSpriteVertexB(TA_Sprite1B* sv)
	{

		{
		vert_base(1,ta_ctx.dec.vtx[1].x,sv->y2,sv->z2);
		}
		{
		vert_base(0,sv->x3,sv->y3,sv->z2);
		}

		sprite_uv(2, u0,v0);
		sprite_uv(3, u1,v1);
		sprite_uv(1, u2,v2);
		sprite_uv(0, u0,v2);//or sprite_uv(u2,v0); ?

		ta_ctx.dec.vtx+=4;

		EndPolyStrip();
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

#ifndef _PS3
bool SetFragShader(const char *pszFragShader)
{
    if(glIsShader(uiFragShader))
        glDeleteShader(uiFragShader);

    if(NULL == pszFragShader)
        return false;

    uiFragShader = glCreateShader(GL_FRAGMENT_SHADER);  gfxCheckGlError(__LINE__);
    glShaderSource(uiFragShader, 1, (const char**)&pszFragShader, NULL); gfxCheckGlError(__LINE__);
    glCompileShader(uiFragShader);  gfxCheckGlError(__LINE__);

    GLint bShaderCompiled;
    glGetShaderiv(uiFragShader, GL_COMPILE_STATUS, &bShaderCompiled);


	int i32InfoLogLength, i32CharsWritten;
	glGetShaderiv(uiFragShader, GL_INFO_LOG_LENGTH, &i32InfoLogLength);
	if (i32InfoLogLength >0 && !bShaderCompiled)
	{
		char* pszInfoLog = new char[i32InfoLogLength];
		glGetShaderInfoLog(uiFragShader, i32InfoLogLength, &i32CharsWritten, pszInfoLog);
		if (!bShaderCompiled)
		{
			printf("Failed to compile fragment shader: %s\n", pszInfoLog);
			return false;
		}
		else
			printf("Compiled fragment shader: %s\n", pszInfoLog);
		delete [] pszInfoLog;
	}
	else
		printf("Compiled Frag Shader!\n");

	return glIsShader(uiFragShader);
}

bool SetVertShader(const char *pszVertShader)
{
    if(glIsShader(uiVertShader))
        glDeleteShader(uiVertShader);

    if(NULL == pszVertShader)
        return false;

    uiVertShader = glCreateShader(GL_VERTEX_SHADER);  gfxCheckGlError(__LINE__);
    glShaderSource(uiVertShader, 1, (const char**)&pszVertShader, NULL);  gfxCheckGlError(__LINE__);
    glCompileShader(uiVertShader);  gfxCheckGlError(__LINE__);

    GLint bShaderCompiled;
	glGetShaderiv(uiVertShader, GL_COMPILE_STATUS, &bShaderCompiled);


	int i32InfoLogLength, i32CharsWritten;
	glGetShaderiv(uiVertShader, GL_INFO_LOG_LENGTH, &i32InfoLogLength);
	if (i32InfoLogLength>0 && !bShaderCompiled)
	{
		char* pszInfoLog = new char[i32InfoLogLength];
		glGetShaderInfoLog(uiVertShader, i32InfoLogLength, &i32CharsWritten, pszInfoLog);
		if (!bShaderCompiled)
		{
			printf("Failed to compile vertex shader: %s\n", pszInfoLog);
			return false;
		}
		else
			printf("Compiled vertex shader: %s\n", pszInfoLog);
		delete [] pszInfoLog;
	}
	else
		printf("Compiled VTX shader !\n");

	return glIsShader(uiVertShader);
}

bool SetProgram(const u32 program)
{
    if(glIsProgram(uiProgramObject[program]))
        glDeleteProgram(uiProgramObject[program]);

    uiProgramObject[program] = glCreateProgram();  gfxCheckGlError(__LINE__);
	glAttachShader(uiProgramObject[program], uiFragShader); gfxCheckGlError(__LINE__);
    glAttachShader(uiProgramObject[program], uiVertShader); gfxCheckGlError(__LINE__);

	printf("glAttachShader: %08X | %08X | %08X\n",uiProgramObject[program],uiFragShader,uiVertShader);

    // Bind the custom vertex attribute "myVertex" to location VERTEX_ARRAY
    glBindAttribLocation(uiProgramObject[program], VERTEX_ARRAY, "vtxPos");
    glBindAttribLocation(uiProgramObject[program], COLOR_ARRAY, "vtxColor");
	glBindAttribLocation(uiProgramObject[program], TEXCOORD_ARRAY, "vtxUV");
    glBindAttribLocation(uiProgramObject[program], COLOR2_ARRAY, "vtxColor2");

    glLinkProgram(uiProgramObject[program]);

    GLint bLinked;
    glGetProgramiv(uiProgramObject[program], GL_LINK_STATUS, &bLinked);

    
	int ui32InfoLogLength, ui32CharsWritten;
	glGetProgramiv(uiProgramObject[program], GL_INFO_LOG_LENGTH, &ui32InfoLogLength);

	if (ui32InfoLogLength >0 && !bLinked)
	{
		char* pszInfoLog = new char[ui32InfoLogLength];
		glGetProgramInfoLog(uiProgramObject[program], ui32InfoLogLength, &ui32CharsWritten, pszInfoLog);
		if (!bLinked)
		{
			printf("Failed to link program(%d): %s\n",program, pszInfoLog);
			return false;
		}
		else
			printf("linked program: %s\n", pszInfoLog);
		delete [] pszInfoLog;
	}
	else
		printf("linked program(%d)!\n", program);
	

    glUseProgram(uiProgramObject[program]);

	GLint SamplerLocation=glGetUniformLocation(uiProgramObject[program], "texImg");
	uiProgramMatrix[program]=glGetUniformLocation(uiProgramObject[program], "Projection");

	if (SamplerLocation!=-1)
		glUniform1i(SamplerLocation,0);

    return glIsProgram(uiProgramObject[program]);
}



// Fragment and vertex shaders code
    const char* pszFragShader = " \
#define pp_UseAlpha %d \n\
#define pp_Texture %d \n\
#define pp_IgnoreTexA %d \n\
#define pp_ShadInstr %d \n\
#define pp_Offset %d \n\
#define cc_AlphaTest %d \n\
uniform sampler2D	texImg; \n\
varying lowp vec4 in_color; \n\
varying lowp vec4 in_offscol; \n\
varying mediump vec2 in_uv; \n\
void main (void) \n\
{            \n\
    lowp vec4 color=in_color.bgra; \n\
	#if pp_UseAlpha==0 \n\
		color.a=1.0; \n\
	#endif \n\
	 \n\
	 #if pp_Texture==1 \n\
		 \n\
		 lowp vec4 texcol=texture2D( texImg,in_uv); \n\
		 \n\
		#if pp_IgnoreTexA==1 \n\
			texcol.a=1.0;	 \n\
		#endif \n\
		 \n\
		#if pp_ShadInstr==0 \n\
			color.rgb=texcol.rgb; \n\
			color.a=texcol.a; \n\
		#elif  pp_ShadInstr==1 \n\
			color.rgb*=texcol.rgb; \n\
			color.a=texcol.a; \n\
		#elif  pp_ShadInstr==2 \n\
			color.rgb=mix(color.rgb,texcol.rgb,texcol.a); \n\
		#elif  pp_ShadInstr==3 \n\
			color*=texcol; \n\
		#endif \n\
	 \n\
		#if pp_Offset==1 \n\
		//	color.rgb+=in_offscol.rgb; \n\
		#endif \n\
		 \n\
	#endif \n\
	if (cc_AlphaTest==1 && color.a<=0.0078125) discard;\n\
	gl_FragColor=color; \n\
}";

    const char* pszVertShader = "\
			uniform highp mat4    Projection; \n\
            attribute highp vec4    vtxPos; \n\
            attribute lowp vec4    vtxColor; \n\
			attribute mediump vec2    vtxUV; \n\
			attribute lowp vec4    vtxColor2; \n\
            varying lowp vec4 in_color; \n\
			varying mediump vec2 in_uv; \n\
			varying lowp vec4 in_offscol; \n\
            void main(void) \n\
            { \n\
				in_color=vtxColor; \n\
				in_offscol=vtxColor2; \n\
				in_uv=vtxUV; \n\
                gl_Position = vtxPos * Projection; \n\
            }";

void gfxCheckGlError(int line)
{
//#ifdef _DEBUG
	GLenum  err = glGetError ();

	if (err != GL_NO_ERROR)
	{
		printf ("GL error:%d at line %d \n", err, line);
		//exit (0);
	}
//#endif
}
bool CompileShader(u32 HasAlphaTest,u32 UseAlpha,u32 Texture,u32 IgnoreTexA,u32 ShadInstr,u32 Offset)
{
	char temp[8192];
	
	sprintf(temp,pszFragShader,UseAlpha,Texture,IgnoreTexA,ShadInstr,Offset,HasAlphaTest);

	if(!SetFragShader(temp))
		return false;
	
	if(!SetProgram(HasAlphaTest*64+GetShaderNum(UseAlpha,Texture,IgnoreTexA,ShadInstr,Offset)))
		return false;

	return true;
}
void SetShader(u32 id)
{
	glUseProgram(uiProgramObject[id]);
}
bool CompileShaders()
{
	if(!SetVertShader(pszVertShader))
		return false;

	for (u32 HasAlphaTest=0;HasAlphaTest<2;HasAlphaTest++)
	{
		for (u32 UseAlpha=0;UseAlpha<2;UseAlpha++)
		{
			for (u32 Texture=0;Texture<2;Texture++)
			{
				bool fst=true;
				for (u32 IgnoreTexA=0;IgnoreTexA<2;IgnoreTexA++)
				{
					for (u32 ShadInstr=0;ShadInstr<4;ShadInstr++)
					{
						for (u32 Offset=0;Offset<2;Offset++)
						{
							if (Texture==0 && !fst)
								uiProgramObject[HasAlphaTest*64+GetShaderNum(UseAlpha,Texture,IgnoreTexA,ShadInstr,Offset)]=uiProgramObject[HasAlphaTest*64+GetShaderNum(UseAlpha,0,0,0,0)];
							else
								if (!CompileShader(HasAlphaTest,UseAlpha,Texture,IgnoreTexA,ShadInstr,Offset))
									return false;
							fst=false;
						}
					}
				}
			}
		}
	}
	
	return true;
}
#include "config/config.h"

#ifndef _ANDROID
bool CreateGLWindow(EGLNativeDisplayType *disp,EGLNativeWindowType* wind)
{
    #if HOST_OS == OS_LINUX && SUPPORT_X11
		if (cfgLoadInt("pvr","nox11",0)==0)
		{
			// X11 variables
			Window				x11Window	= 0;
			Display*			x11Display	= 0;
			long				x11Screen	= 0;
			XVisualInfo*		x11Visual	= 0;
			Colormap			x11Colormap	= 0;

			/*
			Step 0 - Create a NativeWindowType that we can use it for OpenGL ES output
			*/
			Window					sRootWindow;
			XSetWindowAttributes	sWA;
			unsigned int			ui32Mask;
			int						i32Depth;

			printf("Creating X11 window ..\n");

			// Initializes the display and screen
			x11Display = XOpenDisplay( 0 );
			if (!x11Display)
			{
				printf("Error: Unable to open X display\n");
				goto cleanup;
			}
			printf("Creating X11 window ..1\n");
			x11Screen = XDefaultScreen( x11Display );

			printf("Creating X11 window ..2\n");
			// Gets the window parameters
			sRootWindow = RootWindow(x11Display, x11Screen);
			i32Depth = DefaultDepth(x11Display, x11Screen);
			printf("Creating X11 window ..3\n");
			x11Visual = new XVisualInfo;
			XMatchVisualInfo( x11Display, x11Screen, i32Depth, TrueColor, x11Visual);
			printf("Creating X11 window ..4\n");
			if (!x11Visual)
			{
				printf("Error: Unable to acquire visual\n");
				goto cleanup;
			}
			x11Colormap = XCreateColormap( x11Display, sRootWindow, x11Visual->visual, AllocNone );
			sWA.colormap = x11Colormap;

			printf("Creating X11 window ..5\n");
			// Add to these for handling other events
			sWA.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask;
			ui32Mask = CWBackPixel | CWBorderPixel | CWEventMask | CWColormap;

			// Creates the X11 window
			x11Window = XCreateWindow( x11Display, RootWindow(x11Display, x11Screen), 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
				0, CopyFromParent, InputOutput, CopyFromParent, ui32Mask, &sWA);
			printf("Creating X11 window ..6\n");
			XMapWindow(x11Display, x11Window);
			printf("Creating X11 window ..7\n");
			XFlush(x11Display);

			printf("Created X11 window %08x:%08x",x11Window,x11Display);
			//(EGLNativeDisplayType)x11Display;
			*disp=(EGLNativeDisplayType)x11Display;
			*wind=(EGLNativeWindowType)x11Window;
		}
		else
			printf("Not creating X11 window ..\n");
        return true;

    cleanup:
        return false;
    #elif HOST_OS == OS_WINDOWS
		// Register the windows class
		WNDCLASS sWC;
		sWC.style = CS_HREDRAW | CS_VREDRAW;
		sWC.lpfnWndProc = WndProc2;
		sWC.cbClsExtra = 0;
		sWC.cbWndExtra = 0;
		sWC.hInstance = (HINSTANCE)GetModuleHandle(0);
		sWC.hIcon = 0;
		sWC.hCursor = 0;
		sWC.lpszMenuName = 0;
		sWC.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
		sWC.lpszClassName = WINDOW_CLASS;
		unsigned int nWidth = WINDOW_WIDTH;
		unsigned int nHeight = WINDOW_HEIGHT;

		ATOM registerClass = RegisterClass(&sWC);
		if (!registerClass)
		{
			MessageBox(0, _T("Failed to register the window class"), _T("Error"), MB_OK | MB_ICONEXCLAMATION);
		}

		// Create the eglWindow
		RECT	sRect;
		SetRect(&sRect, 0, 0, nWidth, nHeight);
		AdjustWindowRectEx(&sRect, WS_CAPTION | WS_SYSMENU, false, 0);
		HWND hWnd = CreateWindow( WINDOW_CLASS, VER_FULLNAME, WS_VISIBLE | WS_SYSMENU,
				 0, 0, nWidth, nHeight, NULL, NULL, sWC.hInstance, NULL);

		// Get the associated device context
		HDC hDC = GetDC(hWnd);
		*disp=(EGLNativeDisplayType)hDC;
        *wind=(EGLNativeWindowType)hWnd;
		return true;
    #else
        //leave default values
        return true;
    #endif
}
bool SetupEGL2()
{
	printf("Setting up gles2\n");
	 EGLNativeDisplayType disp=(EGLNativeDisplayType)0;
    EGLNativeWindowType wind=(EGLNativeWindowType)0;

    CreateGLWindow(&disp,&wind);

    gl.setup.Display = eglGetDisplay(disp);

	if(gl.setup.Display == EGL_NO_DISPLAY)
		gl.setup.Display = eglGetDisplay((EGLNativeDisplayType) EGL_DEFAULT_DISPLAY);

	printf("eglInitialize!\n");
    EGLint iMajorVersion, iMinorVersion;
    if (!eglInitialize(gl.setup.Display, &iMajorVersion, &iMinorVersion))
    {
            printf("Error: eglInitialize() failed.\n");
            goto cleanup;
    }

    eglBindAPI(EGL_OPENGL_ES_API);

    EGLint pi32ConfigAttribs[7];
    pi32ConfigAttribs[0] = EGL_SURFACE_TYPE;
    pi32ConfigAttribs[1] = EGL_WINDOW_BIT;
    pi32ConfigAttribs[2] = EGL_RENDERABLE_TYPE;
    pi32ConfigAttribs[3] = EGL_OPENGL_ES2_BIT;
    pi32ConfigAttribs[4] = EGL_DEPTH_SIZE;
	pi32ConfigAttribs[5] = 24;
	pi32ConfigAttribs[6] = EGL_NONE;

    EGLint pi32ContextAttribs[3];
    pi32ContextAttribs[0] = EGL_CONTEXT_CLIENT_VERSION;
    pi32ContextAttribs[1] = 2;
    pi32ContextAttribs[2] = EGL_NONE;

    int iConfigs;
    if (!eglChooseConfig(gl.setup.Display, pi32ConfigAttribs, &gl.setup.Config, 1, &iConfigs) || (iConfigs != 1))
    {
            printf("Error: eglChooseConfig() failed.\n");
            goto cleanup;
    }

    gl.setup.Surface = eglCreateWindowSurface(gl.setup.Display, gl.setup.Config, wind, NULL);

    if (!TestEGLError("eglCreateWindowSurface"))
    {
            goto cleanup;
    }

    gl.setup.Context = eglCreateContext(gl.setup.Display, gl.setup.Config, NULL, pi32ContextAttribs);

    if (!TestEGLError("eglCreateContext"))
    {
            goto cleanup;
    }

    eglMakeCurrent(gl.setup.Display, gl.setup.Surface, gl.setup.Surface, gl.setup.Context);

    if (!TestEGLError("eglMakeCurrent"))
    {
            goto cleanup;
    }
	return true;
cleanup:
	return false;
}
#endif

#ifndef _ANDROID
void killEGL2()
{
    eglMakeCurrent(gl.setup.Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) ;
    eglTerminate(gl.setup.Display);
}
#endif
void SetupScene(float TheMatrix[])
{
	glUniformMatrix4fv( uiProgramMatrix[0],1, GL_FALSE, TheMatrix);
	for(u32 progid=0;uiProgramObject[progid]!=0;progid++)
	{
		glUseProgram(uiProgramObject[progid]);
		glUniformMatrix4fv( uiProgramMatrix[progid],1, GL_FALSE, TheMatrix);
	}
}
#ifndef _ANDROID
void SwapBuffersNow()
{
	eglSwapBuffers(gl.setup.Display, gl.setup.Surface);
}
bool TestEGLError(const char* pszLocation)
{
        /*
                eglGetError returns the last error that has happened using egl,
                not the status of the last called function. The user has to
                check after every single egl call or at least once every frame.
        */
        EGLint iErr = eglGetError();
        if (iErr != EGL_SUCCESS)
        {
                printf("%s failed (%d).\n", pszLocation, iErr);
                return false;
        }

        return true;
}
#endif
#else
void gfxCheckCgError(int line)
{
	CGerror err = cgGetError ();

	if (err != CG_NO_ERROR)
	{
		printf ("CG error:%d at line %d %s\n", err, line, cgGetErrorString (err));
		//exit (0);
	}
}
void gfxCheckGlError(int line)
{
	GLenum  err = glGetError ();

	if (err != GL_NO_ERROR)
	{
		printf ("GL error:%d at line %d \n", err, line);
		//exit (0);
	}
}

bool SetupEGL2()
{
	// Load required prx modules.
	int ret = cellSysmoduleLoadModule(CELL_SYSMODULE_GCM_SYS);
	switch( ret )
	{
	    case CELL_OK:
	      // The module is successfully loaded,
		break;

	    case CELL_SYSMODULE_ERROR_DUPLICATED:
	      // The module was already loaded,
		break;

	    case CELL_SYSMODULE_ERROR_UNKNOWN:
	    case CELL_SYSMODULE_ERROR_FATAL:
		printf("!! Failed to load CELL_SYSMODULE_GCM_SYS\n" ); 
		printf("!! Exiting Program \n" ); 
		return false;
	}

	
	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_FS);
	switch( ret )
	{
	    case CELL_OK:
	      // The module is successfully loaded,
		break;

	    case CELL_SYSMODULE_ERROR_DUPLICATED:
	      // The module was already loaded,
		break;

	    case CELL_SYSMODULE_ERROR_UNKNOWN:
	    case CELL_SYSMODULE_ERROR_FATAL:
		printf("!! Failed to load CELL_SYSMODULE_FS\n" ); 
		printf("!! Exiting Program \n" ); 
		return false;
	}

	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_USBD);
	switch( ret )
	{
	    case CELL_OK:
	      // The module is successfully loaded,
		break;

	    case CELL_SYSMODULE_ERROR_DUPLICATED:
	      // The module was already loaded,
		break;

	    case CELL_SYSMODULE_ERROR_UNKNOWN:
	    case CELL_SYSMODULE_ERROR_FATAL:
		printf("!! Failed to load CELL_SYSMODULE_USBD\n" ); 
		printf("!! Exiting Program \n" ); 
		return false;
	}

	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_IO);
	switch( ret )
	{
	    case CELL_OK:
	      // The module is successfully loaded,
		break;

	    case CELL_SYSMODULE_ERROR_DUPLICATED:
	      // The module was already loaded,
		break;

	    case CELL_SYSMODULE_ERROR_UNKNOWN:
	    case CELL_SYSMODULE_ERROR_FATAL:
		printf("!! Failed to load CELL_SYSMODULE_IO\n" ); 
		printf("!! Exiting Program \n" ); 
		return false;
	}

	sys_spu_initialize(6, 1);

	// First, initialize PSGL
	// Note that since we initialized the SPUs ourselves earlier we should
	// make sure that PSGL doesn't try to do so as well.
	PSGLinitOptions initOpts={
        enable: PSGL_INIT_MAX_SPUS | PSGL_INIT_INITIALIZE_SPUS | PSGL_INIT_HOST_MEMORY_SIZE,
		maxSPUs: 1,
		initializeSPUs: false,
		// We're not specifying values for these options, the code is only here
		// to alleviate compiler warnings.
		persistentMemorySize: 0,
		transientMemorySize: 0,
		errorConsole: 0,
		fifoSize: 0,	
		hostMemorySize: 128* 1024*1024,  // 128 mbs for host memory 
	};

	psglInit(&initOpts);

	static PSGLdevice* device=NULL;
	//device=psglCreateDeviceAuto(GL_ARGB_SCE,GL_DEPTH_COMPONENT24,GL_MULTISAMPLING_NONE_SCE);
	//device=psglCreateDeviceAuto(GL_ARGB_SCE,GL_DEPTH_COMPONENT24,GL_MULTISAMPLING_2X_DIAGONAL_CENTERED_SCE);
	device=psglCreateDeviceAuto(GL_ARGB_SCE,GL_DEPTH_COMPONENT24,GL_MULTISAMPLING_NONE_SCE);
	
	if ( !device )
	{
		printf("!! Failed to init the device \n" ); 
		printf("!! Exiting Program \n" ); 
		exit(1); 
	}
	psglGetDeviceDimensions(device,&gfxWidth,&gfxHeight);

	printf("gfxInitGraphics::PSGL Device Initialized Width %d Height %d \n",gfxWidth, gfxHeight ); 	

    gfxAspectRatio = psglGetDeviceAspectRatio(device);

	// Now create a PSGL context
	PSGLcontext *pContext=psglCreateContext();

	if (pContext==NULL) {
		printf("Error creating PSGL context\n");
		exit(-1);
	}

	// Make this context current for the device we initialized
	psglMakeCurrent(pContext, device);

	// Reset the context
	psglResetCurrentContext();
  
	glViewport(0, 0, gfxWidth, gfxHeight);
	glScissor(0, 0, gfxWidth, gfxHeight);
	glClearColor(255.f, 0.f, 0.f, 1.f);
	glEnable(GL_DEPTH_TEST);

	// PSGL doesn't clear the screen on startup, so let's do that here.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	psglSwap();
	return true;
}
void killEGL2() { }
bool CompileShaders()
{
	CGcontext context = cgCreateContext(); gfxCheckCgError(__LINE__);
	char name[32];

	cgGLEnableProfile(cgGLGetLatestProfile(CG_GL_VERTEX)); gfxCheckCgError(__LINE__);
	cgGLEnableProfile(cgGLGetLatestProfile(CG_GL_FRAGMENT)); gfxCheckCgError(__LINE__);

	uiVertShader = cgCreateProgramFromFile(context, CG_BINARY, 
				GetEmuPath("pvr.cgelf"), 
				cgGLGetLatestProfile(CG_GL_VERTEX), 
				"vertex", NULL); gfxCheckCgError(__LINE__);

	vtx_pos=	cgGetNamedParameter(uiVertShader, "position"); gfxCheckCgError(__LINE__);
	vtx_basecol=cgGetNamedParameter(uiVertShader, "base"); gfxCheckCgError(__LINE__);
	vtx_offscol=cgGetNamedParameter(uiVertShader, "offs"); gfxCheckCgError(__LINE__);
	vtx_uv=		cgGetNamedParameter(uiVertShader, "uv"); gfxCheckCgError(__LINE__);

	uiProgramMatrix = cgGetNamedParameter(uiVertShader, "Projection"); gfxCheckCgError(__LINE__);
	//uiTextureTarget = cgGetNamedParameter(uiProgramObject[last_program], "texImg"); gfxCheckCgError(__LINE__);
	
	for (u32 UseAlpha=0;UseAlpha<2;UseAlpha++)
	{
		for (u32 Texture=0;Texture<2;Texture++)
		{
			bool fst=true;
			for (u32 IgnoreTexA=0;IgnoreTexA<2;IgnoreTexA++)
			{
				for (u32 ShadInstr=0;ShadInstr<4;ShadInstr++)
				{
					for (u32 Offset=0;Offset<2;Offset++)
					{
						u32 sid=GetShaderNum(UseAlpha,Texture,IgnoreTexA,ShadInstr,Offset);
						if (Texture==0 && !fst)
							uiProgramObject[sid]=uiProgramObject[GetShaderNum(UseAlpha,0,0,0,0)];
						else	
						{
							sprintf(name,"pixel_%d",sid);
							uiProgramObject[sid] = cgCreateProgramFromFile(context, CG_BINARY, 
										GetEmuPath("pvr.cgelf"), 
										cgGLGetLatestProfile(CG_GL_FRAGMENT), 
										name, NULL); gfxCheckCgError(__LINE__);
						}
						fst=false;
					}
				}
			}
		}
	}

	return true;
}
void SetShader(u32 id)
{
	cgGLBindProgram(uiVertShader); gfxCheckCgError(__LINE__);
	cgGLBindProgram(uiProgramObject[id]); gfxCheckCgError(__LINE__);
}
void SwapBuffersNow()
{
	psglSwap(); gfxCheckGlError(__LINE__);
}
void SetupScene(float TheMatrix[])
{
	glMatrixMode(GL_MODELVIEW);	gfxCheckGlError(__LINE__);
	glLoadMatrixf((GLfloat*)TheMatrix);    gfxCheckGlError(__LINE__);
	cgGLSetStateMatrixParameter (uiProgramMatrix, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);	gfxCheckCgError (__LINE__);
}
bool TestEGLError(const char* pszLocation)
{
	gfxCheckGlError(__LINE__);
	return true;
}
#endif
bool InitRenderer()
{

#ifndef _ANDROID
	if (!SetupEGL2()) goto cleanup;
#endif
	if (!CompileShaders()) goto cleanup;


    //glViewport(0, 0, w, h);

    // Clear the screen to white for a test
    glClearColor(0.f, 0.f, 1.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
#ifndef _ANDROID
	SwapBuffersNow();
#endif
    //glCullFace(GL_FRONT_AND_BACK);
    glDisable(GL_CULL_FACE);

	glGenBuffers(1, &gl.res.vbo);

	Textures.Init();
	return TileAccel.Init();

cleanup:
    TermRenderer();
    return 0;

}

//use that someday
void VBlank()
{

}

void TermRenderer()
{
	SetProgram(NULL);
    SetFragShader(NULL);
    SetVertShader(NULL);
#ifndef _ANDROID
	killEGL2();
#endif
	TileAccel.Term();
}

void ResetRenderer(bool Manual)
{
	ta_ctx.reset();
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









#endif




