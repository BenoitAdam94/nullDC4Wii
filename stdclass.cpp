#include "types.h"
#include <string.h>
#include <stdarg.h>

#include "dc/mem/_vmem.h"
#include "plugins/plugin_manager.h"

//comonly used classes across the project

u32 Array_T_id_count=0;

u32 fastrand_seed=0xDEADCAFE;

u32 fastrand()
{
	fastrand_seed=(fastrand_seed>>9)^(fastrand_seed<<11)^(fastrand_seed>>24);//^1 is there
	return fastrand_seed++;//if it got 0 , take good care of it :)
}

//Misc function to get relative source directory for printf's
wchar temp[1000];
wchar* GetNullDCSoruceFileName(const char* full)
{
	size_t len = strlen(full);
	while(len>18)
	{
		if (full[len]=='/')
		{
			memcpy(&temp[0],&full[len-14],15*sizeof(char));
			temp[15*sizeof(char)]=0;
			if (strcmp(&temp[0],"/nulldc/nulldc/")==0)
			{

				strcpy(temp,&full[len+1]);
				return temp;
			}
		}
		len--;
	}
	strcpy(temp,full);
	return &temp[0];
}

wchar* GetPathFromFileNameTemp(wchar* full)
{
	size_t len = strlen(full);
	while(len>2)
	{
		if (full[len]=='/')
		{
			memcpy(&temp[0],&full[0],(len+1)*sizeof(wchar));
			temp[len+1]=0;
			return temp;
		}
		len--;
	}
	strcpy(temp,full);
	return &temp[0];
}

void GetPathFromFileName(wchar* path)
{
	strcpy(path,GetPathFromFileNameTemp(path));
}

void GetFileNameFromPath(wchar* path,wchar* outp)
{

	size_t i=strlen(path);

	while (i>0)
	{
		i--;
		if (path[i]=='/')
		{
			strcpy(outp,&path[i+1]);
			return;
		}
	}

	strcpy(outp,path);
}

wchar AppPath[1024];
void GetApplicationPath(wchar* path,u32 size)
{
	if (AppPath[0]==0)
	{
		strcpy(path,"./");
	}

	strcpy(path,AppPath);
}

void SetApplicationPath(wchar* path)
{
	strcpy(AppPath,path);
}

wchar* GetEmuPath(const wchar* subpath)
{
	wchar* temp=(wchar*)malloc(1024);
	GetApplicationPath(temp,1024);
	strcat(temp,subpath);
	return temp;
}

void VArray2::LockRegion(u32 offset,u32 size)
{
	printf("LOCK REGION\n");
}
void VArray2::UnLockRegion(u32 offset,u32 size)
{
	printf("UNLOCK REGION\n");
}

int msgboxf(const wchar* text,unsigned int type,...)
{

	va_list args;

	wchar temp[2048];
	va_start(args, type);
	vsprintf(temp, text, args);
	va_end(args);

	os_msgbox(temp,type);
	return MBX_RV_OK;
}

