#include "config.h"
#include "types.h"

#if REND_API==REND_GLES2


#ifdef _PS3
	#include <PSGL/psgl.h>
	#include <PSGL/psglu.h>
	#include <Cg/cg.h>
#else
	#ifndef _ANDROID
		#include <EGL/egl.h>
	#endif
	#include <GLES2/gl2.h>
#endif
#include "../../config.h"


#include "regs.h"
//input : address in the yyyyyxxxxx format
//output : address in the xyxyxyxy format
//U : x resolution , V : y resolution
//twidle works on 64b words
u32 fastcall untwiddle(u32 x,u32 y,u32 x_sz,u32 y_sz)
{
	u32 rv=0;//Pvr internaly maps the 64b banks "as if" they were twidled

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

u8 VramWork[1024*1024*2];
u8 vram32[1024*1024*4];

//hanlder functions
void fastcall texture_TW(u8* p_in,u32 Width,u32 Height)
{
	u16* pb=(u16*)VramWork;

	const u32 xpp=2;
	const u32 ypp=2;
	const u32 divider=xpp*ypp;

	for (u32 y=0;y<Height;y+=ypp)
	{
		for (u32 x=0;x<Width;x+=xpp)
		{
			u16* p = (u16*)&p_in[(untwiddle(x,y,Width,Height)/divider)<<3];
			
			pb[0]=		*host_ptr_xor(&p[0]);
			pb[1]=		*host_ptr_xor(&p[2]);
			pb[Width]=	*host_ptr_xor(&p[1]);
			pb[Width+1]=*host_ptr_xor(&p[3]);
			
			pb+=xpp;
		}
		pb+=Width*(ypp-1);
	}
}

void fastcall texture_VQ(u8* p_in,u32 Width,u32 Height,u16* vq_codebook)
{
	u16* pb=(u16*)VramWork;
	
	const u32 xpp=2;
	const u32 ypp=2;
	const u32 divider=xpp*ypp;

	for (u32 y=0;y<Height;y+=ypp)
	{
		for (u32 x=0;x<Width;x+=xpp)
		{
			u32 idx=*host_ptr_xor(&p_in[untwiddle(x,y,Width,Height)/divider])*4;
			
			pb[0]=		*host_ptr_xor(&vq_codebook[idx + 0]);
			pb[1]=		*host_ptr_xor(&vq_codebook[idx + 2]);
			pb[Width]=	*host_ptr_xor(&vq_codebook[idx + 1]);
			pb[Width+1]=*host_ptr_xor(&vq_codebook[idx + 3]);

			pb+=xpp;
		}
		pb+=Width*(ypp-1);
	}
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

#define ARGB1555( word )	\
	( ((word&0x8000)?(0xFF000000):(0)) | ((word&0x7C00)>>7) | ((word&0x3E0)<<6) | ((word&0x1F)<<19) )

#define ARGB565( word )		\
	( 0xFF000000 | ((word&0xF800)>>8) | ((word&0x7E0)<<5) | ((word&0x1F)<<19) )

#define ARGB4444( word )	\
	( ((word&0xF000)<<16) | ((word&0xF00)>>4) | ((word&0xF0)<<8) | ((word&0xF)<<20) )

#define ARGB8888( dword )	\
	( (dword&0xFF00FF00) | ((dword>>16)&0xFF) | ((dword&0xFF)<<16) )

void SetRepeatMode(GLuint dir,u32 clamp,u32 mirror)
{
	if (clamp)
		glTexParameteri (GL_TEXTURE_2D, dir, GL_CLAMP_TO_EDGE);
	else 
		glTexParameteri (GL_TEXTURE_2D, dir, mirror?GL_MIRRORED_REPEAT : GL_REPEAT);
}

void gfxCheckGlError(int line);

bool DecodeAndLoad(u8* ptex, u32 texid, TCW tcw, TSP tsp)
{
	u32 w=8<<tsp.TexU;
	u32 h=8<<tsp.TexV;
	
	switch (tcw.NO_PAL.PixelFmt)
	{
	case 0:	//1555
	case 1: //565
	case 2: //4444
	case 7: //1555
		if (tcw.NO_PAL.ScanOrder)
		{
			if (tcw.NO_PAL.StrideSel) 
				w=512;
			//memcpy(VramWork,ptex,w*h*2);
			u32 texbytes=w*h*2;
			u16* pout=(u16*)VramWork;
			u16* pin=(u16*)ptex;
			while(texbytes)
			{
				*pout++=*host_ptr_xor(pin++);
				texbytes-=2;
			}

		}
		else
		{
			if (tcw.NO_PAL.VQ_Comp)
			{
				u16* vq_codebook=(u16*)ptex;
				ptex+=256*4*2;
				if (tcw.NO_PAL.MipMapped)
					ptex+=MipPoint[tsp.TexU];
				texture_VQ(ptex,w,h,vq_codebook);
			}
			else
			{
				if (tcw.NO_PAL.MipMapped)
					ptex+=MipPoint[tsp.TexU]<<3;
				texture_TW(ptex,w,h);
			}
		}
		break;

	case 3://3	YUV422 32 bits per 2 pixels; YUYV values: 8 bits each
	case 4://4	Bump Map	16 bits/pixel; S value: 8 bits; R value: 8 bits
	case 5://5	4 BPP Palette	Palette texture with 4 bits/pixel
	case 6://6	8 BPP Palette	Palette texture with 8 bits/pixel
	default:
		printf("Unhandled texture\n");
		memset(VramWork,0xFFEFCFAF,w*h*4);
	}

    glBindTexture(GL_TEXTURE_2D, texid);
	gfxCheckGlError(__LINE__);
	//set texture repeat mode
	SetRepeatMode(GL_TEXTURE_WRAP_S,tsp.ClampU,tsp.FlipU);
	SetRepeatMode(GL_TEXTURE_WRAP_T,tsp.ClampV,tsp.FlipV);
	gfxCheckGlError(__LINE__);
	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
	gfxCheckGlError(__LINE__);
    if( tsp.FilterMode != 0 ) 
	{
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	//GL_LINEAR_MIPMAP_LINEAR for nicer looking :>
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } 
	else 
	{
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
	gfxCheckGlError(__LINE__);

	//if (fmt2==GL_UNSIGNED_SHORT_5_5_5_1)
	{
		u16* ps=(u16*)VramWork;
		u32* pb=(u32*)vram32;
		u32 sz=w*h;
		while(sz)
		{
			switch(tcw.NO_PAL.PixelFmt)
			{
			case 2: //4444
				*pb=ARGB4444(*ps);
				break;

			case 0: //1555
			case 7:
				*pb=ARGB1555(*ps);
				break;

			case 1:	//565
				*pb=ARGB565(*ps);
				break;
			}
			//*pb=sz;//rand();//(*pb>>1)|(*pb<<15);
			ps++;
			pb++;
			sz--;
		}
	}
	/*
	char temp[128];
	sprintf(temp,"textures/texture_%x_%dx%d_%d_.linear",(u32)ptex,w,h,tcw.NO_PAL.PixelFmt);
	char* pp=GetEmuPath(temp);
	FILE* f=fopen(pp,"wb");
	if (f)
	{
	fwrite(VramWork,1,w*h*2,f);
	fclose(f);
	}

	sprintf(temp,"textures/texture_%x_%dx%d_%d_.dec",(u32)ptex,w,h,tcw.NO_PAL.PixelFmt);
	pp=GetEmuPath(temp);
	f=fopen(pp,"wb");
	if (f)
	{
	fwrite(vram32,1,w*h*4,f);
	fclose(f);
	}
	*/
#ifdef _PS3
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8 , w, h, 0, GL_RGBA , GL_UNSIGNED_INT_8_8_8_8, vram32);
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA , w, h, 0, GL_RGBA , GL_UNSIGNED_BYTE, vram32);
	gfxCheckGlError(__LINE__);
	if (tcw.NO_PAL.MipMapped)
			glGenerateMipmap(GL_TEXTURE_2D);
	gfxCheckGlError(__LINE__);
#endif
	return true;
}











////////////////////////////////////////////////////////////////////////////////////////


#if 0


////////////////////////////////////////////////////////////////////////////////////////





void TexDecYUV_TW(PolyParam *pp, TexEntry *te)	// *FIXME* uh detwiddle it?
{
	s32 R=0, G=0, B=0;
	s32 Y0, Yu, Y1, Yv;

	u32 texU = pp->param0.tsp.TexU+3;
	for(u32 p=0; p<(te->Width * te->Height); p+=2)
	{
		u16 YUV0 = *(u16 *)(emuIf.vram + te->Start + (twop(p, texU)<<1) );
		u16 YUV1 = *(u16 *)(emuIf.vram + te->Start + (twop(p+2, texU)<<1) );

		Y0 = YUV0>>8 &255;//(s32)*(u8*)(emuIf.vram + te->Start + (twop(p + 0, texU)<<1));
		Yu = YUV0>>0 &255;//(s32)*(u8*)(emuIf.vram + te->Start + (twop(p + 1, texU)<<1));
		Y1 = YUV1>>8 &255;//(s32)*(u8*)(emuIf.vram + te->Start + (twop(p + 2, texU)<<1));
		Yv = YUV1>>0 &255;//(s32)*(u8*)(emuIf.vram + te->Start + (twop(p + 3, texU)<<1));

		B = (76283*(Y0 - 16) + 132252*(Yu - 128))>>16;
		G = (76283*(Y0 - 16) - 53281 *(Yv - 128) - 25624*(Yu - 128))>>16;
		R = (76283*(Y0 - 16) + 104595*(Yv - 128))>>16;

		pTempTex[p] = 0xFF000000;
		pTempTex[p] |= ((R>0xFF)?0xFF:(R<0)?0:R);
		pTempTex[p] |= ((G>0xFF)?0xFF:(G<0)?0:G) << 8;
		pTempTex[p] |= ((B>0xFF)?0xFF:(B<0)?0:B) << 16;

		B = (76283*(Y0 - 16) + 132252*(Yu - 128))>>16;
		G = (76283*(Y0 - 16) - 53281 *(Yv - 128) - 25624*(Yu - 128))>>16;
		R = (76283*(Y0 - 16) + 104595*(Yv - 128))>>16;

		pTempTex[p+1] = 0xFF000000;
		pTempTex[p+1] |= ((R>0xFF)?0xFF:(R<0)?0:R);
		pTempTex[p+1] |= ((G>0xFF)?0xFF:(G<0)?0:G) << 8;
		pTempTex[p+1] |= ((B>0xFF)?0xFF:(B<0)?0:B) << 16;
	}

	te->End = te->Start + te->Width * te->Height * 2;
	TexGen(pp,te);
}

void TexDecYUV(PolyParam *pp, TexEntry *te)
{
	s32 R=0, G=0, B=0;
	s32 Y0, Yu, Y1, Yv;

	for(u32 p=0; p<(te->Width * te->Height); p+=2)
	{
		Yu = (s32)*(u8*)(emuIf.vram + te->Start + (p<<1) + 0);
		Y0 = (s32)*(u8*)(emuIf.vram + te->Start + (p<<1) + 1);
		Yv = (s32)*(u8*)(emuIf.vram + te->Start + (p<<1) + 2);
		Y1 = (s32)*(u8*)(emuIf.vram + te->Start + (p<<1) + 3);

		B = (76283*(Y0 - 16) + 132252*(Yu - 128))>>16;
		G = (76283*(Y0 - 16) - 53281 *(Yv - 128) - 25624*(Yu - 128))>>16;
		R = (76283*(Y0 - 16) + 104595*(Yv - 128))>>16;

		pTempTex[p] = 0xFF000000;
		pTempTex[p] |= ((R>0xFF)?0xFF:(R<0)?0:R);
		pTempTex[p] |= ((G>0xFF)?0xFF:(G<0)?0:G) << 8;
		pTempTex[p] |= ((B>0xFF)?0xFF:(B<0)?0:B) << 16;

		B = (76283*(Y0 - 16) + 132252*(Yu - 128))>>16;
		G = (76283*(Y0 - 16) - 53281 *(Yv - 128) - 25624*(Yu - 128))>>16;
		R = (76283*(Y0 - 16) + 104595*(Yv - 128))>>16;

		pTempTex[p+1] = 0xFF000000;
		pTempTex[p+1] |= ((R>0xFF)?0xFF:(R<0)?0:R);
		pTempTex[p+1] |= ((G>0xFF)?0xFF:(G<0)?0:G) << 8;
		pTempTex[p+1] |= ((B>0xFF)?0xFF:(B<0)?0:B) << 16;
	}

	te->End = te->Start + te->Width * te->Height;
	TexGen(pp,te);
}

void TexDecYUV_SR(PolyParam *pp, TexEntry *te)
{
	s32 R=0, G=0, B=0;
	s32 Y0, Yu, Y1, Yv;
	u32 Stride = (32 * (*pTEXT_CONTROL &31));

	for(u32 y=0; y<te->Height; y++)
	{
		u32 x=0;
		for(x=0; x<Stride; x+=2)
		{
			u32 p = (y*Stride+x);
			u32 t = (y*te->Width+x);

			Yu = (s32)*(u8*)(emuIf.vram + te->Start + (p<<1) + 0);
			Y0 = (s32)*(u8*)(emuIf.vram + te->Start + (p<<1) + 1);
			Yv = (s32)*(u8*)(emuIf.vram + te->Start + (p<<1) + 2);
			Y1 = (s32)*(u8*)(emuIf.vram + te->Start + (p<<1) + 3);

			B = (76283*(Y0 - 16) + 132252*(Yu - 128))>>16;
			G = (76283*(Y0 - 16) - 53281 *(Yv - 128) - 25624*(Yu - 128))>>16;
			R = (76283*(Y0 - 16) + 104595*(Yv - 128))>>16;

			pTempTex[t] = 0xFF000000;
			pTempTex[t] |= ((R>0xFF)?0xFF:(R<0)?0:R);
			pTempTex[t] |= ((G>0xFF)?0xFF:(G<0)?0:G) << 8;
			pTempTex[t] |= ((B>0xFF)?0xFF:(B<0)?0:B) << 16;

			B = (76283*(Y0 - 16) + 132252*(Yu - 128))>>16;
			G = (76283*(Y0 - 16) - 53281 *(Yv - 128) - 25624*(Yu - 128))>>16;
			R = (76283*(Y0 - 16) + 104595*(Yv - 128))>>16;

			pTempTex[t+1] = 0xFF000000;
			pTempTex[t+1] |= ((R>0xFF)?0xFF:(R<0)?0:R);
			pTempTex[t+1] |= ((G>0xFF)?0xFF:(G<0)?0:G) << 8;
			pTempTex[t+1] |= ((B>0xFF)?0xFF:(B<0)?0:B) << 16;
		}
#ifdef DEBUG_LIB
		for(; x<te->Width; x++) {
			u32 t = (y*te->Width+x);
			pTempTex[t] = 0xFF00FF00;
		}
#endif //DEBUG_LIB
	}

	te->End = te->Start + te->Width * te->Height;
	TexGen(pp,te);
}

/*	The color format for a palette format texture is specified by the PAL_RAM_CTRL register,
*	and can be selected from among four types: 1555, 565, 4444, and 8888.
*	When the color format is 8888 mode, texture filtering performance is reduced by half.
*/

void TexDecPAL4(PolyParam *pp, TexEntry *te)
{
	u32 palselect = *((u32*)&pp->param0.tcw) >> 21 & 0x3F;

	u16 * pPAL = &((u16*)TA_PalT)[palselect<<5];

	// decode this bullshit first
	u32 texU = pp->param0.tsp.TexU+3;

	switch(*pPAL_RAM_CTRL &3)
	{
	case 0:	// argb155
		for(u32 y=0; y<te->Height; y+=2)
		{
			for(u32 x=0; x<te->Width; x+=2)
			{
				u16 iColor = *(u16*)(emuIf.vram + te->Start + (twop(((y>>1)*te->Width+(x>>1)),texU)<<1));

				pTempTex[y*te->Width+x]			= ARGB1555(pPAL[(iColor        & 0x0f)*2]);
				pTempTex[y*te->Width+x+1]		= ARGB1555(pPAL[((iColor >>  8) & 0x0f)*2]);
				pTempTex[(y+1)*te->Width+x]		= ARGB1555(pPAL[((iColor >>  4) & 0x0f)*2]);
				pTempTex[(y+1)*te->Width+x+1]	= ARGB1555(pPAL[((iColor >> 12) & 0x0f)*2]);
			}
		}
		break;

	case 1:	// rgb565
		for(u32 y=0; y<te->Height; y+=2)
		{
			for(u32 x=0; x<te->Width; x+=2)
			{
				u16 iColor = *(u16*)(emuIf.vram + te->Start + (twop(((y>>1)*te->Width+(x>>1)),texU)<<1));

				pTempTex[y*te->Width+x]			= ARGB565(pPAL[( iColor        & 0x0f)*2]);
				pTempTex[y*te->Width+x+1]		= ARGB565(pPAL[((iColor >>  8) & 0x0f)*2]);
				pTempTex[(y+1)*te->Width+x]		= ARGB565(pPAL[((iColor >>  4) & 0x0f)*2]);
				pTempTex[(y+1)*te->Width+x+1]	= ARGB565(pPAL[((iColor >> 12) & 0x0f)*2]);
			}
		}
		break;

	case 2:	// argb4444
		{
			for(u32 y=0; y<te->Height; y+=2)
			{
				for(u32 x=0; x<te->Width; x+=2)
				{
					u16 iColor = *(u16*)(emuIf.vram + te->Start + (twop(((y>>1)*te->Width+(x>>1)),texU)<<1));

					pTempTex[y*te->Width+x]			= ARGB4444(pPAL[( iColor        & 0x0f)*2]);
					pTempTex[y*te->Width+x+1]		= ARGB4444(pPAL[((iColor >>  8) & 0x0f)*2]);
					pTempTex[(y+1)*te->Width+x]		= ARGB4444(pPAL[((iColor >>  4) & 0x0f)*2]);
					pTempTex[(y+1)*te->Width+x+1]	= ARGB4444(pPAL[((iColor >> 12) & 0x0f)*2]);
				}
			}
		}

		break;

	case 3:	// argb8888
		for(u32 y=0; y<te->Height; y+=2)
		{
			for(u32 x=0; x<te->Width; x+=2)
			{
				u16 iColor = *(u16*)(emuIf.vram + te->Start + (twop(((y>>1)*te->Width+(x>>1)),texU)<<1));

				pTempTex[y*te->Width+x]			= ARGB8888(*(u32*)&pPAL[( iColor        & 0x0f)*2]);
				pTempTex[y*te->Width+x+1]		= ARGB8888(*(u32*)&pPAL[((iColor >>  8) & 0x0f)*2]);
				pTempTex[(y+1)*te->Width+x]		= ARGB8888(*(u32*)&pPAL[((iColor >>  4) & 0x0f)*2]);
				pTempTex[(y+1)*te->Width+x+1]	= ARGB8888(*(u32*)&pPAL[((iColor >> 12) & 0x0f)*2]);
			}
		}
		break;
	}


	te->End = te->Start + te->Width * te->Height /2;//4bits per pixel
	TexGen(pp,te);
}

void TexDecPAL8(PolyParam *pp, TexEntry *te)
{
	u32 palselect = *((u32*)&pp->param0.tcw) >> 21 & 0x3F;

	u16 * pPAL = &((u16*)TA_PalT)[palselect<<5];

	u32 texU = pp->param0.tsp.TexU+3;

	switch(*pPAL_RAM_CTRL &3)
	{
	case 0:	// argb1555
		for(u32 p=0; p<(te->Width * te->Height); p++)
		{
			u8 pval = *(u8*)(emuIf.vram + te->Start + (twop(p,texU)));
			pTempTex[p] = ARGB1555(pPAL[pval*2]) ;
		}
		break;

	case 1:	// rgb565
		for(u32 p=0; p<(te->Width * te->Height); p++)
		{
			u8 pval = *(u8*)(emuIf.vram + te->Start + (twop(p,texU)));
			pTempTex[p] = ARGB565(pPAL[pval*2]) ;
		}
		break;

	case 2:	// argb4444
		for(u32 p=0; p<(te->Width * te->Height); p++)
		{
			u8 pval = *(u8*)(emuIf.vram + te->Start + (twop(p,texU)));
			pTempTex[p] = ARGB4444(pPAL[pval*2]) ;
		}

		break;

	case 3:	// argb8888
		for(u32 p=0; p<(te->Width * te->Height); p++)
		{
			u8 pval = *(u8*)(emuIf.vram + te->Start + (twop(p,texU)));
			pTempTex[p] = ARGB8888(*(u32*)&pPAL[pval*2]) ;
		}
		break;
	}

	te->End = te->Start + te->Width * te->Height *1;//8bits per pixel
	TexGen(pp,te);
	memset(pTempTex, 0xFF, 1024*1024*4);
}



void TexDecYUV_TW_MM(PolyParam *pp, TexEntry *te)
{
	s32 R=0, G=0, B=0;
	s32 Y0, Yu, Y1, Yv;

	u32 texU = pp->param0.tsp.TexU+3;
	u8* texAddr = (emuIf.vram + te->Start) + MipPoint(texU-3);

	for(u32 p=0; p<(te->Width * te->Height); p+=2)
	{
		u16 YUV0 = *(u16 *)(texAddr + (twop(p, texU)<<1) );
		u16 YUV1 = *(u16 *)(texAddr + (twop(p+2, texU)<<1) );

		Y0 = YUV0>>8 &255;//(s32)*(u8*)(emuIf.vram + te->Start + (twop(p + 0, texU)<<1));
		Yu = YUV0>>0 &255;//(s32)*(u8*)(emuIf.vram + te->Start + (twop(p + 1, texU)<<1));
		Y1 = YUV1>>8 &255;//(s32)*(u8*)(emuIf.vram + te->Start + (twop(p + 2, texU)<<1));
		Yv = YUV1>>0 &255;//(s32)*(u8*)(emuIf.vram + te->Start + (twop(p + 3, texU)<<1));

		B = (76283*(Y0 - 16) + 132252*(Yu - 128))>>16;
		G = (76283*(Y0 - 16) - 53281 *(Yv - 128) - 25624*(Yu - 128))>>16;
		R = (76283*(Y0 - 16) + 104595*(Yv - 128))>>16;

		pTempTex[p] = 0xFF000000;
		pTempTex[p] |= ((R>0xFF)?0xFF:(R<0)?0:R);
		pTempTex[p] |= ((G>0xFF)?0xFF:(G<0)?0:G) << 8;
		pTempTex[p] |= ((B>0xFF)?0xFF:(B<0)?0:B) << 16;

		B = (76283*(Y0 - 16) + 132252*(Yu - 128))>>16;
		G = (76283*(Y0 - 16) - 53281 *(Yv - 128) - 25624*(Yu - 128))>>16;
		R = (76283*(Y0 - 16) + 104595*(Yv - 128))>>16;

		pTempTex[p+1] = 0xFF000000;
		pTempTex[p+1] |= ((R>0xFF)?0xFF:(R<0)?0:R);
		pTempTex[p+1] |= ((G>0xFF)?0xFF:(G<0)?0:G) << 8;
		pTempTex[p+1] |= ((B>0xFF)?0xFF:(B<0)?0:B) << 16;
	}

	te->End = te->Start + te->Width * te->Height * 2;
	TexGen(pp,te);
}

// VQ


static u32 lcodebook[256][4];


void TexDec1555_TW_VQ(PolyParam *pp, TexEntry *te)
{
	u32 texU = pp->param0.tsp.TexU+3;
	u16 tcol;
	u32 texAddr = te->Start;

	for( u32 j=0; j<256;j++) {
		for( u32 h=0; h<4; h++ )	// 2byte texture indices
		{
			*(u32*)&lcodebook[j][3-h] = ARGB1555(*(u16*)(emuIf.vram + texAddr));
			texAddr+=2;
		}
	}

	u32 texoffset=0;

	for(u32 i=0; i<te->Height; i+=2) {
		for(u32 j=0; j<te->Width; j+=2)
		{
			int texoffset =  (twop(((i>>1)*te->Width+(j>>1)),texU));
			tcol = *(u8*)(emuIf.vram + (texAddr+texoffset));

			pTempTex[(i+1)*te->Width+(j+1)]	= lcodebook[tcol][0];
			pTempTex[i*te->Width+(j+1)]		= lcodebook[tcol][1];
			pTempTex[(i+1)*te->Width+j]		= lcodebook[tcol][2];
			pTempTex[i*te->Width+j]			= lcodebook[tcol][3];
		}
	}

	te->End = te->Start + te->Width * te->Height +0x800;	// Not Right?
	TexGen(pp,te);
}

void TexDec565_TW_VQ(PolyParam *pp, TexEntry *te)
{
	u32 texU = pp->param0.tsp.TexU+3;

	u16 tcol;
	u32 texAddr = te->Start;

	for( u32 j=0; j<256;j++) {
		for( u32 h=0; h<4; h++ )	// 2byte texture indices
		{
			*(u32*)&lcodebook[j][3-h] = ARGB565(*(u16*)(emuIf.vram + texAddr));
			texAddr+=2;
		}
	}

	u32 texoffset=0;

	for(u32 i=0; i<te->Height; i+=2) {
		for(u32 j=0; j<te->Width; j+=2)
		{
			int texoffset =  (twop(((i>>1)*te->Width+(j>>1)),texU));
			tcol = *(u8*)(emuIf.vram + (texAddr+texoffset));

			pTempTex[(i+1)*te->Width+(j+1)]	= lcodebook[tcol][0];
			pTempTex[i*te->Width+(j+1)]		= lcodebook[tcol][1];
			pTempTex[(i+1)*te->Width+j]		= lcodebook[tcol][2];
			pTempTex[i*te->Width+j]			= lcodebook[tcol][3];
		}
	}

	te->End = te->Start + te->Width * te->Height +0x800;	// Not Right?
	TexGen(pp,te);
}

void TexDec4444_TW_VQ(PolyParam *pp, TexEntry *te)
{
	u32 texU = pp->param0.tsp.TexU+3;

	u16 tcol;
	u32 texAddr = te->Start;

	for( u32 j=0; j<256;j++) {
		for( u32 h=0; h<4; h++ )	// 2byte texture indices
		{
			*(u32*)&lcodebook[j][3-h] = ARGB4444(*(u16*)(emuIf.vram + texAddr));
			texAddr+=2;
		}
	}

	u32 texoffset=0;

	for(u32 i=0; i<te->Height; i+=2) {
		for(u32 j=0; j<te->Width; j+=2)
		{
			int texoffset =  (twop(((i>>1)*te->Width+(j>>1)),texU));
			tcol = *(u8*)(emuIf.vram + (texAddr+texoffset));

			pTempTex[(i+1)*te->Width+(j+1)]	= lcodebook[tcol][0];
			pTempTex[i*te->Width+(j+1)]		= lcodebook[tcol][1];
			pTempTex[(i+1)*te->Width+j]		= lcodebook[tcol][2];
			pTempTex[i*te->Width+j]			= lcodebook[tcol][3];
		}
	}

	te->End = te->Start + te->Width * te->Height +0x800;	// Not Right?
	TexGen(pp,te);
}



	// MipMap Decoding

void TexDec1555_TW_MM(PolyParam *pp, TexEntry *te)
{
	u32 texU = pp->param0.tsp.TexU+3;
	u32 MipOffs = MipPoint(texU-3);

	for(u32 p=0; p<(te->Width * te->Height); p++)
	{
		u16 pval = *(u16*)(emuIf.vram + te->Start + MipOffs + (twop(p,texU)<<1));
		pTempTex[p] = ARGB1555(pval) ;
	}

	te->End = te->Start + te->Width * te->Height * 2;
	TexGen(pp,te);
}

void TexDec565_TW_MM(PolyParam *pp, TexEntry *te)
{
	u32 texU = pp->param0.tsp.TexU+3;
	u32 MipOffs = MipPoint(texU-3);

	for(u32 p=0; p<(te->Width * te->Height); p++)
	{
		u16 pval = *(u16*)(emuIf.vram + te->Start + MipOffs + (twop(p,texU)<<1));
		pTempTex[p] = ARGB565(pval) ;
	}

	te->End = te->Start + te->Width * te->Height * 2;
	TexGen(pp,te);
}

void TexDec4444_TW_MM(PolyParam *pp, TexEntry *te)
{
	u32 texU = pp->param0.tsp.TexU+3;
	u32 MipOffs = MipPoint(texU-3);

	for(u32 p=0; p<(te->Width * te->Height); p++)
	{
		u16 pval = *(u16*)(emuIf.vram + te->Start + MipOffs + (twop(p,texU)<<1));
		pTempTex[p] = ARGB4444(pval) ;
	}

	te->End = te->Start + te->Width * te->Height * 2;
	TexGen(pp,te);
}




	// MM + VQ

void TexDec1555_TW_MM_VQ(PolyParam *pp, TexEntry *te)
{
	u32 texU = pp->param0.tsp.TexU+3;
	u16 tcol;
	u32 texAddr = te->Start;

	for( u32 j=0; j<256;j++) {
		for( u32 h=0; h<4; h++ )	// 2byte texture indices
		{
			*(u32*)&lcodebook[j][3-h] = ARGB1555(*(u16*)(emuIf.vram + texAddr));
			texAddr+=2;
		}
	}

	texAddr += MipPointVQ(texU-3);

	u32 texoffset=0;

	for(u32 i=0; i<te->Height; i+=2) {
		for(u32 j=0; j<te->Width; j+=2)
		{
			int texoffset =  (twop(((i>>1)*te->Width+(j>>1)),texU));
			tcol = *(u8*)(emuIf.vram + (texAddr+texoffset));

			pTempTex[(i+1)*te->Width+(j+1)]	= lcodebook[tcol][0];
			pTempTex[i*te->Width+(j+1)]		= lcodebook[tcol][1];
			pTempTex[(i+1)*te->Width+j]		= lcodebook[tcol][2];
			pTempTex[i*te->Width+j]			= lcodebook[tcol][3];
		}
	}

	te->End = te->Start + te->Width * te->Height +0x800;	// Not Right?
	TexGen(pp,te);
}

void TexDec565_TW_MM_VQ(PolyParam *pp, TexEntry *te)
{
	u32 texU = pp->param0.tsp.TexU+3;

	u16 tcol;
	u32 texAddr = te->Start;

	for( u32 j=0; j<256;j++) {
		for( u32 h=0; h<4; h++ )	// 2byte texture indices
		{
			*(u32*)&lcodebook[j][3-h] = ARGB565(*(u16*)(emuIf.vram + texAddr));
			texAddr+=2;
		}
	}

	texAddr += MipPointVQ(texU-3);

	u32 texoffset=0;

	for(u32 i=0; i<te->Height; i+=2) {
		for(u32 j=0; j<te->Width; j+=2)
		{
			int texoffset = (twop(((i>>1)*te->Width+(j>>1)),texU));
			tcol = *(u8*)(emuIf.vram + (texAddr+texoffset));

			pTempTex[(i+1)*te->Width+(j+1)]	= lcodebook[tcol][0];
			pTempTex[i*te->Width+(j+1)]		= lcodebook[tcol][1];
			pTempTex[(i+1)*te->Width+j]		= lcodebook[tcol][2];
			pTempTex[i*te->Width+j]			= lcodebook[tcol][3];
		}
	}

	te->End = te->Start + te->Width * te->Height +0x800;	// Not Right?
	TexGen(pp,te);
}

void TexDec4444_TW_MM_VQ(PolyParam *pp, TexEntry *te)
{
	u32 texU = pp->param0.tsp.TexU+3;

	u16 tcol;
	u32 texAddr = te->Start;

	for( u32 j=0; j<256;j++) {
		for( u32 h=0; h<4; h++ )	// 2byte texture indices
		{
			*(u32*)&lcodebook[j][3-h] = ARGB4444(*(u16*)(emuIf.vram + texAddr));
			texAddr+=2;
		}
	}

	texAddr += MipPointVQ(texU-3);

	u32 texoffset=0;

	for(u32 i=0; i<te->Height; i+=2) {
		for(u32 j=0; j<te->Width; j+=2)
		{
			int texoffset =  (twop(((i>>1)*te->Width+(j>>1)),texU));
			tcol = *(u8*)(emuIf.vram + (texAddr+texoffset));

			pTempTex[(i+1)*te->Width+(j+1)]	= lcodebook[tcol][0];
			pTempTex[i*te->Width+(j+1)]		= lcodebook[tcol][1];
			pTempTex[(i+1)*te->Width+j]		= lcodebook[tcol][2];
			pTempTex[i*te->Width+j]			= lcodebook[tcol][3];
		}
	}

	te->End = te->Start + te->Width * te->Height +0x800;	// Not Right?
	TexGen(pp,te);
}



/// [K1,K2,K3,Q] are Poly Offs Color Bytes: [3,2,1,0] - [S,R] are Texel Bytes: [1,0]
/*
*
*
*
*/

inline static float someshit(u8 S, u8 R, u8 T, u8 Q)
{
	float s = 3.14/2.f * (float)S/256.f;
	float r = 3.14*2.f * (float)R/256.f;
/*
	float Xs = cos(s)*cos(r);
	float Ys = sin(s);
	float Zs = cos(s)*sin(r);
*/
	float t = 3.14/2.f * (float)T/256.f;	// Where the fuck does T come from ?
	float q = 3.14*2.f * (float)Q/256.f;
/*
	float Xl = cos(t)*cos(q);
	float Yl = sin(t);
	float Zl = cos(t)*sin(q);

	float I = Xs*Xl + Ys*Yl + Zs*Zl;
*/

	// Ok, two simplifications :
	//float I = cos(s)*cos(r)*cos(t)*cos(q) + sin(s)*sin(t) + cos(s)*sin(r)*cos(t)*sin(q) ;

	// Even Better

	float I = sin(s)*sin(t) + cos(s)*cos(t)*cos(r-q) ;
}

void TexDecBump(PolyParam *pp, TexEntry *te)
{
	printf(" -------- BUMP MAP -------- ");
}

void TexDecBump_TW(PolyParam *pp, TexEntry *te)
{
	printf(" -------- BUMP MAP TW -------- ");
}



#endif


#endif // GLES
