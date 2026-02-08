#include "../../config.h"


#include "regs.h"
#if REND_API == REND_SOFT
	#if HOST_OS == OS_WINDOWS
	#include <windows.h>
	#endif
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
};

#pragma pack(pop)


struct VertexList
{
	union
	{
		struct
		{
			u16 count;
			u16 flags;
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
		Vertex ALIGN16  vertices[4*1024];
		VertexList  ALIGN16 lists[1*1024];
		PolyParam  ALIGN16 pparams[1*1024];
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
	    u32 format;
		u32 texture;
		u32 sa;

		void Init(u32 tex)
		{
			texture = tex;
			sa=0xFFFFFFFF;

			//glBindTexture(GL_TEXTURE_2D, texture);

			//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		void Term()
		{
			//glDeleteTextures(1,&texture);
		}

		void Generate(TCW tcw,TSP tsp);
	};
	TexCacheEntry cache[512];
	void Init()
	{
		//u32 t[512];
		//glGenTextures(512,t);

		//for (u32 i=0;i<512;i++)
		//{
		//	cache[i].Init(t[i]);
		//}
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
			//glBindTexture(GL_TEXTURE_2D, e->texture);
		}
		else
		{
			//if not, get a free one
			e=GetFree();
			//and bind/fill it
			e->Generate(tcw,tsp);
		}
	}
};

TexCache Textures;



//bool DecodeAndLoad(u8* ptex, u32 texid, TCW tcw, TSP tsp);




//I LOVE C++ HAHAHA *NOT*
void TexCache::TexCacheEntry::Generate(TCW tcw,TSP tsp)
{
	u32 sa=(tcw.NO_PAL.TexAddr<<3) & VRAM_MASK;
	this->sa=sa;

	u16* ptr=(u16*)&params.vram[sa];

    //if(!DecodeAndLoad((u8*)ptr, texture, tcw, tsp))
        printf("Error, failed to decode texture!\n");

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
	cv->col=ABGR8888(col);
	if (isp.Offset)
	{
		//Intesity color (can be missing too ;p)
		u32 col=vri(ptr);ptr+=4;
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
void DrawLists(const DisplayList& tl)
{
	if (!tl.valid())
		return;
	const PolyParam* pp=tl.pp;
	u32 vtx=tl.vtxnum;

	for (const VertexList* lst=tl.start;lst!=tl.end;lst++)
	{
		s32 flags_count=lst->flags_count;
		u32 count=(u16)flags_count;
		if (flags_count<0)
		{
			if (pp->pcw.Texture)
			{
				//glUseProgram(uiProgramObject[0]);
				//glEnable(GL_TEXTURE_2D);
				//SetTextureParams(pp);
			}
			else
			{
				//glUseProgram(uiProgramObject[1]);
				//glDisable(GL_TEXTURE_2D);
			}
			pp++;
		}
/*
		glDrawArrays(GL_TRIANGLE_STRIP, vtx, count);
		if (!TestEGLError("glDrawArrays"))
		{
			printf("Error, glDrawArrays() failed!\n");
		}
*/
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

	float dc_width=640;
    float dc_height=480;

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
	//printf("The Matrix\n");
	//float* mtx=TheMatrix;
    //for (int j=0;j<4;j++,mtx+=4)
    //    printf("%5.2f | %5.2f  | %5.2f  | %5.2f \n",mtx[0],mtx[1],mtx[2],mtx[3]);

//    glUniformMatrix4fv( glGetUniformLocation(uiProgramObject[0], "myPMVMatrix"),
//                        1, GL_FALSE, TheMatrix);
	// Draw triangle

	u8 bR=BGTest.col;
	u8 bG=BGTest.col/256;
	u8 bB=BGTest.col/256/256;
	u8 bA=BGTest.col/256/256/256;
/*
	glClearColor(bR/255.0, bG/255.0, bB/255.0, bA/255.0);  //BGTest.col);
    glClearDepthf(1.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glDisable(GL_BLEND);
//      glDisable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	//glDepthRangef(0,1);
*/
    #define VERTEX_ARRAY 0
    #define COLOR_ARRAY 1
	#define TEXCOORD_ARRAY 2
/*
	GLint SamplerLocation=glGetUniformLocation(uiProgramObject[0], "myPMVMatrix");
	glUniform1i(SamplerLocation,0);

	glBindBuffer(GL_ARRAY_BUFFER, gl.res.vbo);
	glBufferData(GL_ARRAY_BUFFER,(u8*)ta_ctx.dec.vtx-(u8*)ta_ctx.data.vertices,ta_ctx.data.vertices,GL_STREAM_DRAW);

	glEnableVertexAttribArray(VERTEX_ARRAY);
	glVertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), &((Vertex*)0)->x);	//they had to bring the ptr uglyness to es too eh ?

	glEnableVertexAttribArray(COLOR_ARRAY);
	glVertexAttribPointer(COLOR_ARRAY, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), &((Vertex*)0)->col);

	glEnableVertexAttribArray(TEXCOORD_ARRAY);
	glVertexAttribPointer(TEXCOORD_ARRAY, 2, GL_FLOAT, GL_TRUE, sizeof(Vertex), &((Vertex*)0)->u);
*/


	DrawLists(ta_ctx.op);
/*
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDepthMask(GL_TRUE);
	//glEnable(GL_ALPHA_TEST);
	//glAlphaFunc(GL_GREATER,0,0xFF);
*/
	DrawLists(ta_ctx.pt);

	DrawLists(ta_ctx.tr);

	ta_ctx.reset();

//	eglSwapBuffers(gl.setup.Display, gl.setup.Surface);
}

void StartRender()
{
	u32 VtxCnt=ta_ctx.dec.vtx-ta_ctx.data.vertices;
	VertexCount+=VtxCnt;

	render_end_pending_cycles= VtxCnt*145;
	if (render_end_pending_cycles<500000)
		render_end_pending_cycles=500000;

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
}


//Vertex Decoding-Converting
struct VertexDecoder
{
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
			ta_ctx.tr.finish(ta_ctx.dec.lst);
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

		ta_ctx.dec.vtx++;
	}

	//(Textured, Intensity, 16bit UV)
	__forceinline
	static void AppendPolyVertex8(TA_Vertex8* vtx)
	{
		vert_cvt_base;
		ta_ctx.dec.vtx->col=INTESITY(vtx->BaseInt);

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


bool InitRenderer()
{
	Textures.Init();
	return TileAccel.Init();
}

//use that someday
void VBlank()
{

}

void TermRenderer()
{

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