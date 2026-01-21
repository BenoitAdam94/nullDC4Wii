// cdi_cvt.cpp : Defines the entry point for the console application.
//

#include <windows.h>
#include <tchar.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>


//#include "cdi.h"
#define u8 BYTE
#define u16 WORD
#define  u32 DWORD
#include "pfctoc.h"

PfcFreeTocFP* PfcFreeToc;
PfcGetTocFP*  PfcGetToc;

SPfcToc* pstToc;
//SessionInfo cdi_ses;
//TocInfo cdi_toc;
enum DiscType
{
	CdDA=0x00,
	CdRom=0x10,
	CdRom_XA=0x20,
	CdRom_Extra=0x30,
	CdRom_CDI=0x40,
	GdRom=0x80,		
	NoDisk=0x1,
	Open=0x2,			//tray is open :)
	Busy=0x3			//busy -> needs to be autmaticaly done by gdhost
};
DiscType cdi_Disctype;
struct file_TrackInfo
{
	u32 FAD;
	u32 Offset;
	u32 SectorSize;
};

file_TrackInfo Track[101];

u32 TrackCount;

u8 SecTemp[2352];
FILE* fp_cdi;
void cdi_CreateToc(TCHAR* file)
{
	wcscat(file,L".cditoc");
	FILE* ftoc=_tfopen(file,L"wb");
	//clear structs to 0xFF :)
	memset(Track,0xFF,sizeof(Track));
//	memset(&cdi_ses,0xFF,sizeof(cdi_ses));
//	memset(&cdi_toc,0xFF,sizeof(cdi_toc));

	printf("\n--GD toc info start--\n");
	int track=0;
	bool CD_DA=false;
	bool CD_M1=false;
	bool CD_M2=false;
#define pv(n,v) fprintf(ftoc,"%s = 0 , %d\n",n,v)
#define pv2(n,v,v2) fprintf(ftoc,"%s = %d , %d\n",n,v,v2)


	printf("Last Sector : %d\n",pstToc->dwOuterLeadOut);
	printf("Session count : %d\n",pstToc->dwSessionCount);

	
//	cdi_toc.FistTrack=1;
	pv("toc.firsttrack",1);
	u32 last_FAD=0;
	u32 TrackOffset=0;

	u32 ses_count=0;
	for (u32 s=0;s<pstToc->dwSessionCount;s++)
	{
		printf("Session %d:\n",s);
		SPfcSession* ses=&pstToc->pstSession[s];
		if (ses->bType==4 && ses->dwTrackCount==0)
		{
			printf("Detected open disc\n");
			continue;
		}
		ses_count++;

		printf("  Track Count: %d\n",ses->dwTrackCount);
		for (u32 t=0;t< ses->dwTrackCount ;t++)
		{
			SPfcTrack* cdi_track=&ses->pstTrack[t];

			//pre gap
			last_FAD	+=cdi_track->pdwIndex[0];
			TrackOffset	+=cdi_track->pdwIndex[0]*cdi_track->dwBlockSize;

			if (t==0)
			{
				pv2("ses.fad",s,last_FAD);
				//cdi_ses.SessionFAD[s]=last_FAD;
				pv2("ses.starttrack",s,track+1);
				//cdi_ses.SessionStart[s]=track+1;
				printf("  Session start FAD: %d\n",last_FAD);
			}

//			verify(cdi_track->dwIndexCount==2);
			printf("  track %d:\n",t);
			printf("    Type : %d\n",cdi_track->bMode);

			if (cdi_track->bMode==2)
				CD_M2=true;
			if (cdi_track->bMode==1)
				CD_M1=true;
			if (cdi_track->bMode==0)
				CD_DA=true;
			
			
			pv2("track.addr",track,1);
			//cdi_toc.tracks[track].Addr=1;//hmm is that ok ?
			pv2("track.session",track,s);
			//cdi_toc.tracks[track].Session=s;
			pv2("track.control",track,cdi_track->bCtrl);
			//cdi_toc.tracks[track].Control=cdi_track->bCtrl;
			pv2("track.fad",track,last_FAD);
			//cdi_toc.tracks[track].FAD=last_FAD;


//			Track[track].FAD=cdi_toc.tracks[track].FAD;
			pv2("track.sectorsize",track,cdi_track->dwBlockSize);
//			Track[track].SectorSize=cdi_track->dwBlockSize;
			pv2("track.fileoffset",track,TrackOffset);
//			Track[track].Offset=TrackOffset;
			
			printf("    Start FAD : %d\n",Track[track].FAD);
			printf("    SectorSize : %d\n",Track[track].SectorSize);
			printf("    File Offset : %d\n",Track[track].Offset);

			printf("    %d indexes \n",cdi_track->dwIndexCount);
			for (u32 i=0;i<cdi_track->dwIndexCount;i++)
			{
				printf("     index %d : %d\n",i,cdi_track->pdwIndex[i]);
			}
			//main track data
			TrackOffset+=(cdi_track->pdwIndex[1])*cdi_track->dwBlockSize;
			last_FAD+=cdi_track->pdwIndex[1];
			track++;
		}
		last_FAD+=11400-150;///next session
	}

	if ((CD_M1==true) && (CD_DA==false) && (CD_M2==false))
		cdi_Disctype = CdRom;
	else if (CD_M2)
		cdi_Disctype = CdRom_XA;
	else if (CD_DA && CD_M1) 
		cdi_Disctype = CdRom_Extra;
	else
		cdi_Disctype=CdRom;//hmm?

	pv("ses.count",ses_count);
	//cdi_ses.SessionCount=ses_count;

	pv("ses.endfad",pstToc->dwOuterLeadOut);
	//cdi_ses.SessionsEndFAD=pstToc->dwOuterLeadOut;

	pv("toc.leadout.fad",pstToc->dwOuterLeadOut);
	//cdi_toc.LeadOut.FAD=pstToc->dwOuterLeadOut;

	pv("toc.leadout.addr",0);
	//cdi_toc.LeadOut.Addr=0;

	pv("toc.leadout.ctrl",0);
	//cdi_toc.LeadOut.Control=0;

	pv("toc.leadout.sesion",0);
	//cdi_toc.LeadOut.Session=0;

	pv("toc.leadout.fad",pstToc->dwOuterLeadOut);

	printf("Disc Type = %d\n",cdi_Disctype);
	TrackCount=track;
	pv("track.count",track);
	pv("disctype",cdi_Disctype);
//	cdi_toc.LastTrack=track;
	printf("--GD toc info end--\n\n");
	fclose(ftoc);
}

HMODULE pfctoc_mod=NULL;
bool cdi_init(_TCHAR* file_)
{
	char file[512];
	wcstombs(file,file_,512);
	pfctoc_mod=LoadLibrary(L"plugins\\pfctoc.dll");
	if (pfctoc_mod==NULL)
		pfctoc_mod=LoadLibrary(L"pfctoc.dll");
	if(pfctoc_mod==NULL)
		return false;

	PfcFreeToc=(PfcFreeTocFP*)GetProcAddress(pfctoc_mod,"PfcFreeToc");
	PfcGetToc=(PfcGetTocFP*)GetProcAddress(pfctoc_mod,"PfcGetToc");
//	verify(PfcFreeToc!=NULL && PfcFreeToc!=NULL);

	//char fn[512]="";
	//GetFile(fn,"cdi images (*.cdi) \0*.cdi\0\0");
	DWORD dwSize;//
	DWORD dwErr = PfcGetToc(file, pstToc, dwSize);
    if (dwErr == PFCTOC_OK) 
	{
		cdi_CreateToc(file_);
    }
	else
	{
		return false;
		//printf("Failed to open file , %d",dwErr);
	}
	//fp_cdi=fopen(file,"rb");

	return true;
}

void cdi_term()
{
	if (pstToc)
		PfcFreeToc(pstToc);
	if (pfctoc_mod)
		FreeLibrary(pfctoc_mod);
	pstToc=0;
	pfctoc_mod=0;
}


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc!=2)
		printf("Usage : cdi_cvt <CDI FILE>\nIE\n\tcdi_cvt \"c:\\my images\\game.cdi\"\n\tcdi_cvt game.cdi\n");
	else
	{
		wprintf(L"Opening \"%s\"\n",argv[1]);
		if (cdi_init(argv[1]))
		{
			wprintf(L"Toc writen @ \"%s\"\n",argv[1]);
		}
		else
			printf("Failed to open image or pfctoc.dll is missing\n");
	}
	return 0;
}

