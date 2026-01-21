
#if 0
#if HOST_OS==OS_LINUX
#include <zlib.h>
#else
#include "zlib/zlib.h"
#endif

#include "remote_tcp.h"
#include "local_cache.h"


// this hurts my brain .. tmp disable

#if HOST_OS==OS_WINDOWS

Cache_Data* rtcp_local_cache;
void rtcp_DriveReadSector(u8 * buff,u32 StartSector,u32 SectorCount,u32 secsz)
{
	printf("SectorRead : Sector %d , size %d , mode %d \n",StartSector,SectorCount,secsz);


	for (u32 i=0;i<SectorCount;i++)
	{
		if (rtcp_local_cache->ReadSector(buff,StartSector+i,secsz))
		{
			printf("Local Cache Read %d\n",StartSector+i);
		}
		else
		{
			printf("Remote Read %d\n",StartSector+i);
			//do some magic
			rtcp_local_cache->SendSectorBlock(0,0,0);
			//and try to reread from cache
			if (!rtcp_local_cache->ReadSector(buff,StartSector+i,secsz))
			{
				printf("Remote Read %d failed\n",StartSector+i);
			}
		}

		buff+=secsz;//advance pointer
	}
}

void rtcp_DriveGetTocInfo(TocInfo* toc,DiskArea area)
{
	//Send a fake a$$ toc
	//toc->last.full		= toc->first.full	= CTOC_TRACK(1);
	toc->first.number=1;
	toc->last.number=1;
	toc->first.ControlInfo=toc->last.ControlInfo=4;
	toc->first.Addr=toc->last.Addr=0;
	toc->lba_leadout.FAD=400000;

 	//toc->entry[0].full	= CTOC_LBA(150) | CTOC_ADR(0) | CTOC_CTRL(4);
	toc->entry[0].Addr=0;
	toc->entry[0].ControlInfo=4;
	//toc->entry[1].Addr=0;
	//toc->entry[1].ControlInfo=4;
	if (area==DoubleDensity)
	{
		toc->entry[0].FAD=150;
		//toc->entry[1].FAD=45150;
	}
	else
	{
		toc->entry[0].FAD=150;
		//toc->entry[1].FAD=45150;
	}

	for (int i=1;i<99;i++)
	{
		toc->entry[i].full=0xFFFFFFFF;
	}
}
//TODO : fix up
DiskType rtcp_DriveGetDiskType()
{
	return DiskType::GdRom;
}


void rtcp_init()
{
	rtcp_local_cache=new Cache_Data("192.168.1.1");
}

void rtcp_term()
{
	delete rtcp_local_cache;
}

#endif // !PPC

#endif