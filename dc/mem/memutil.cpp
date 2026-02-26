#include "types.h"
#include "memutil.h"
#include "sh4_mem.h"

// *FIXME* ENDIAN

// Helper: open a file and get its size. Returns NULL on failure.
// Caller is responsible for fclose().
static FILE* OpenFileAndGetSize(const char* file, int* out_size)
{
	FILE* fd = fopen(file, "rb");
	if (!fd) return NULL;

	fseek(fd, 0, SEEK_END);
	*out_size = (int)ftell(fd);
	fseek(fd, 0, SEEK_SET);
	return fd;
}

u32 LoadFileToSh4Mem(u32 offset, char* file)
{
	int size = 0;
	FILE* fd = OpenFileAndGetSize(file, &size);
	if (!fd) {
		printf("LoadFileToSh4Mem: can't load file \"%s\", file not found\n", file);
		return 0;
	}

	// BUG FIX: original code had fseek(fd, SEEK_SET, 0) — args were swapped.
	// We now get the size above before reading the ELF magic.
	u32 e_ident = 0;
	fread(&e_ident, 1, 4, fd);
	fseek(fd, 0, SEEK_SET); // fixed: was fseek(fd, SEEK_SET, 0)

	if (0x464C457F == e_ident)
	{
		fclose(fd);
		printf("!\tERROR: Loading ELF is not supported (%s)\n", file);
		return 0;
	}

	if (size <= 0) {
		fclose(fd);
		printf("LoadFileToSh4Mem: file \"%s\" is empty or unreadable\n", file);
		return 0;
	}

	// IMPROVEMENT: guard against writing past the end of mem_b.
	// MEM_SIZE should be defined in sh4_mem.h; if not, remove this check.
#ifdef MEM_SIZE
	if ((u32)size > MEM_SIZE || offset > MEM_SIZE - (u32)size) {
		fclose(fd);
		printf("LoadFileToSh4Mem: file \"%s\" would overflow SH4 memory at offset 0x%x\n", file, offset);
		return 0;
	}
#endif

	size_t rd = fread(&mem_b[offset], 1, size, fd);
	fclose(fd);

	if ((int)rd != size) {
		printf("LoadFileToSh4Mem: WARNING: expected %d bytes, read %zu from \"%s\"\n", size, rd, file);
	}

	u32 end = offset + (u32)rd;
	printf("LoadFileToSh4Mem: loaded \"%s\" to SysMem[0x%x..0x%x], %zu bytes\n",
	       file, offset, end - 1, rd);
	return 1;
}

u32 LoadBinfileToSh4Mem(u32 offset, char* file)
{
	// Magic bytes that indicate an embedded IP.BIN header inside the binary
	static const u8 CheckStr[8] = {0x7, 0xd0, 0x8, 0xd1, 0x17, 0x10, 0x5, 0xdf};

	u32 rv = LoadFileToSh4Mem(0x10000, file);
	if (!rv) return rv; // IMPROVEMENT: skip check if initial load failed

	for (int i = 0; i < 8; i++)
	{
		if (ReadMem8(0x8C010000 + i + 0x300) != CheckStr[i])
			return rv;
	}

	// File has embedded IP.BIN — reload at lower offset
	return LoadFileToSh4Mem(0x8000, file);
}

bool LoadFileToSh4Bootrom(wchar* szFile)
{
	FILE* fd = fopen(szFile, "rb");
	if (!fd) {
		printf("LoadFileToSh4Bootrom: can't load file \"%s\", file not found\n", szFile);
		return false;
	}

	fseek(fd, 0, SEEK_END);
	int flen = (int)ftell(fd);
	fseek(fd, 0, SEEK_SET);

#ifndef BUILD_DEV_UNIT
	if (flen > BIOS_SIZE) {
		printf("LoadFileToSh4Bootrom: can't load \"%s\", too large (%d bytes, max %d)\n",
		       szFile, flen, BIOS_SIZE);
		fclose(fd); // BUG FIX: original forgot to fclose on this error path
		return false;
	}
#else
	// Dev build: skip to internal offset 0x15014
	fseek(fd, 0x15014, SEEK_SET);
	flen -= 0x15014; // IMPROVEMENT: adjust length so we don't read past EOF
	if (flen <= 0) {
		printf("LoadFileToSh4Bootrom: dev file \"%s\" too small for offset 0x15014\n", szFile);
		fclose(fd);
		return false;
	}
#endif

	size_t rd = fread(&bios_b[0], 1, flen, fd);
	fclose(fd);

	if ((int)rd != flen) {
		printf("LoadFileToSh4Bootrom: WARNING: expected %d bytes, read %zu from \"%s\"\n",
		       flen, rd, szFile);
	}

	printf("LoadFileToSh4Bootrom: loaded \"%s\", %zu bytes\n", szFile, rd);
	return true;
}

bool LoadFileToSh4Flashrom(wchar* szFile)
{
	FILE* fd = fopen(szFile, "rb");
	if (!fd) {
		printf("LoadFileToSh4Flashrom: can't load file \"%s\", file not found\n", szFile);
		return false;
	}

	fseek(fd, 0, SEEK_END);
	int flen = (int)ftell(fd);
	fseek(fd, 0, SEEK_SET);

	if (flen > FLASH_SIZE) {
		printf("LoadFileToSh4Flashrom: can't load \"%s\", too large (%d bytes, max %d)\n",
		       szFile, flen, FLASH_SIZE);
		fclose(fd); // BUG FIX: original forgot to fclose on this error path
		return false;
	}

	size_t rb = fread(&flash_b[0], 1, flen, fd);
	fclose(fd);

	if ((int)rb != flen) {
		printf("LoadFileToSh4Flashrom: WARNING: expected %d bytes, read %zu from \"%s\"\n",
		       flen, rb, szFile);
	}

	printf("LoadFileToSh4Flashrom: loaded \"%s\", %zu bytes\n", szFile, rb);
	return true;
}

bool SaveSh4FlashromToFile(wchar* szFile)
{
	FILE* fd = fopen(szFile, "wb");
	if (!fd) {
		printf("SaveSh4FlashromToFile: can't open file \"%s\"\n", szFile);
		return false;
	}

	// No need to fseek(fd, 0, SEEK_SET) on a freshly opened "wb" file — removed.
	size_t written = fwrite(&flash_b[0], 1, FLASH_SIZE, fd);
	fclose(fd);

	if (written != FLASH_SIZE) {
		printf("SaveSh4FlashromToFile: WARNING: only wrote %zu of %d bytes to \"%s\"\n",
		       written, FLASH_SIZE, szFile);
		return false; // IMPROVEMENT: report failure if write was incomplete
	}

	printf("SaveSh4FlashromToFile: saved flash to \"%s\"\n", szFile);
	return true;
}
