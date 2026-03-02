#pragma once

#define 	ReadMemArrRet(arr,addr,sz)				\
			{if (sz==1)								\
				return arr[addr];					\
			else if (sz==2)							\
				return *(u16*)&arr[addr];			\
			else if (sz==4)							\
				return *(u32*)&arr[addr];}	

#define WriteMemArrRet(arr,addr,data,sz)				\
			{if(sz==1)								\
				{arr[addr]=(u8)data;return;}				\
			else if (sz==2)							\
				{*(u16*)&arr[addr]=(u16)data;return;}		\
			else if (sz==4)							\
			{*(u32*)&arr[addr]=data;return;}}	
#define WriteMemArr(arr,addr,data,sz)				\
			{if(sz==1)								\
				{arr[addr]=(u8)data;}				\
			else if (sz==2)							\
				{*(u16*)&arr[addr]=(u16)data;}		\
			else if (sz==4)							\
			{*(u32*)&arr[addr]=data;}}	