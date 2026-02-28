#include "local_cache.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char CachePath[512];

// MAX_SECTOR_SIZE covers all GD-ROM/CD-ROM sector formats (up to 2352 bytes raw)
#define MAX_SECTOR_SIZE 2352

Cache_Data::Cache_Data(const char* remote, u32 total_sectors)
	: SaveFile(NULL), num_blocks(0), block_size(16), Sectors(NULL)
{
	strncpy(Remote, remote, sizeof(Remote) - 1);
	Remote[sizeof(Remote) - 1] = '\0';

	// Write directly into CacheFile, no intermediate buffer needed
	snprintf(CacheFile, sizeof(CacheFile), "%s%s", CachePath, remote);

	SaveFile = fopen(CacheFile, "w+b");
	if (!SaveFile)
		SaveFile = fopen(CacheFile, "r+b");

	if (block_size > 0 && total_sectors > 0)
	{
		num_blocks = (total_sectors + block_size - 1) / block_size;
		Sectors = (Cache_Data_Entry*)calloc(num_blocks, sizeof(Cache_Data_Entry));
		// type == 0 means uncached (calloc zeroes everything, so this is safe)
	}
}

Cache_Data::~Cache_Data()
{
	if (SaveFile)
		fclose(SaveFile);
	if (Sectors)
		free(Sectors);
}

bool Cache_Data::ReadSector(u8* buffer, u32 Sector, u16 type)
{
	if (!SaveFile || !Sectors || block_size == 0)
		return false;

	u32 sec_block = Sector / block_size;
	if (sec_block >= num_blocks)
		return false;

	Cache_Data_Entry* secblk = &Sectors[sec_block];

	if (secblk->type == 0)
		return false; // block not cached yet

	// Bounds-check sector size
	if (secblk->type > MAX_SECTOR_SIZE)
		return false;

	u64 sector_offset = secblk->offset + (u64)secblk->type * (Sector - secblk->start_sector);
	fseek(SaveFile, (long)sector_offset, SEEK_SET);

	u8 temp_buff[MAX_SECTOR_SIZE];
	if (fread(temp_buff, 1, secblk->type, SaveFile) != secblk->type)
		return false;

	ConvertSector(temp_buff, buffer, secblk->type, type, Sector);
	return true;
}

bool Cache_Data::SendSectorBlock(u32 start_sector, u16 type, u8* data)
{
	if (!SaveFile || !Sectors || block_size == 0)
		return false;

	u32 sec_block = start_sector / block_size;
	if (sec_block >= num_blocks)
		return false;

	fseek(SaveFile, 0, SEEK_END);

	// Write block header
	fwrite(&start_sector, 1, sizeof(start_sector), SaveFile);
	fwrite(&type,         1, sizeof(type),         SaveFile);
	fwrite(&block_size,   1, sizeof(block_size),   SaveFile);

	long pos = ftell(SaveFile);
	if (pos < 0)
		return false;

	// Write sector data
	size_t data_size = (size_t)type * block_size;
	if (fwrite(data, 1, data_size, SaveFile) != data_size)
		return false;

	// Update in-memory descriptor
	Cache_Data_Entry* secblk = &Sectors[sec_block];
	secblk->offset       = (u64)pos;
	secblk->start_sector = start_sector;
	secblk->type         = type;

	return true;
}

bool Cache_Data::Save()
{
	if (!SaveFile)
		return false;
	fflush(SaveFile);
	return true;
}
