#pragma once
#include "common.h"

struct Cache_Data_Entry
{
	u64 offset;
	u16 type;

	//used for save/load from file
	u32 start_sector;
};

class Cache_Data
{
private:
	FILE* SaveFile;
	u32 num_blocks;  // total number of sector blocks

public:
	//remote file
	char Remote[4096];

	//local file
	char CacheFile[512];

	u32 block_size;
	Cache_Data_Entry* Sectors;

	bool Save();
	Cache_Data(const char* remote, u32 total_sectors);
	~Cache_Data();
	bool ReadSector(u8* buffer, u32 Sector, u16 type);
	bool SendSectorBlock(u32 start_sector, u16 type, u8* data);
};

//path of the cache files, including "\"
extern char CachePath[512];
