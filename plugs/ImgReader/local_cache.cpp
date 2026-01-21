#include "local_cache.h"

char CachePath[512];

Cache_Data::Cache_Data(char Remote[])
{
	char temp[512];
	strcpy(temp,CachePath);
	strcat(temp,Remote);
	SaveFile =fopen(temp,"rwb");
}

Cache_Data::~Cache_Data()
{
	if (SaveFile)
		fclose(SaveFile);
}

bool Cache_Data::ReadSector(u8* buffer,u32 Sector,u16 type)
{
	//get sector's block
	u32 sec_block=Sector/block_size;
	Cache_Data_Entry* secblk=&Sectors[sec_block];
	
	if (secblk->type!=0)
	{
		fseek(SaveFile,secblk->offset + secblk->type*(Sector-secblk->start_sector),SEEK_SET);
		u8 temp_buff[3000];
		fread(temp_buff,1,secblk->type,SaveFile);
		ConvertSector(&temp_buff[0],buffer,secblk->type,type,Sector);

		return true;
	}

	return false;
}

bool Cache_Data::SendSectorBlock(u32 start_sector,u16 type,u8* data)
{
	//seek to the end of the file
	fseek(SaveFile,0,SEEK_END);
	
	//write header
	fwrite(&start_sector,1,4,SaveFile);
	fwrite(&type,1,2,SaveFile);
	fwrite(&block_size,1,4,SaveFile);

	u64 pos=ftell(SaveFile);
	//write data
	fwrite(data,1,type*block_size,SaveFile);

	//get sector's block
	u32 sec_block=start_sector/block_size;
	Cache_Data_Entry* secblk=&Sectors[sec_block];

	//save the new block info on mem descriptor too
	secblk->offset=pos;
	secblk->start_sector=start_sector;
	secblk->type=type;
	return true;
}

