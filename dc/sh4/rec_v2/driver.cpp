/*
	Improved driver.cpp for Dreamcast Emulator on Wii
	
	Key improvements:
	- Wii PowerPC optimizations
	- Better memory management for limited RAM
	- Improved code cache efficiency
	- Better error handling and debugging
	- Performance monitoring
*/

#include "types.h"

#if HOST_OS==OS_WINDOWS
#include <windows.h>
#elif HOST_OS==OS_LINUX
#include <unistd.h>
#include <sys/mman.h>
#elif HOST_OS==OS_WII
// Wii-specific includes
#include <gccore.h>
#include <malloc.h>
#endif

#include "../sh4_interpreter.h"
#include "../sh4_opcode_list.h"
#include "../sh4_registers.h"
#include "../sh4_if.h"
#include "../dmac.h"
#include "../intc.h"
#include "../tmu.h"

#include "dc/mem/sh4_mem.h"
#include "dc/pvr/pvr_if.h"
#include "dc/aica/aica_if.h"
#include "dc/gdrom/gdrom_if.h"

#include <time.h>
#include <float.h>

#include "blockmanager.h"
#include "ngen.h"
#include "decoder.h"

#ifndef HOST_NO_REC

// Performance monitoring (can be disabled in release builds)
// #define ENABLE_PERF_MONITORING

#ifdef ENABLE_PERF_MONITORING
static u32 perf_blocks_compiled = 0;
static u32 perf_cache_clears = 0;
static u32 perf_block_checks_failed = 0;
#endif

// ============================================================================
// Forward Declarations
// ============================================================================
void recSh4_ClearCache();

// ============================================================================
// Code Cache Management
// ============================================================================

extern volatile bool sh4_int_bCpuRun;

// Code cache configuration for Wii
// Reduced from default to fit Wii's memory constraints
#ifndef CODE_SIZE
#define CODE_SIZE (4*1024*1024)  // 4 MB instead of 6 MB for Wii
#endif

// Wii-specific: align code cache to 32-byte cache line
#if HOST_OS == OS_WII
u8 CodeCache[CODE_SIZE] __attribute__((aligned(32)));
#elif HOST_OS != OS_LINUX
u8 CodeCache[CODE_SIZE];
#elif HOST_OS == OS_LINUX
u8 pMemBuff[CODE_SIZE+4095];
u8 *CodeCache;
#endif

// Code cache state
u32 LastAddr = 0;
u32 LastAddr_min = 0;
u32* emit_ptr = 0;

// Wii-specific: track cache pressure for better management
#if HOST_OS == OS_WII
static u32 cache_high_water_mark = 0;
#define CACHE_PRESSURE_THRESHOLD (CODE_SIZE * 90 / 100)  // 90% full
#endif

// ============================================================================
// Emit Functions - Optimized for Wii
// ============================================================================

void* emit_GetCCPtr() 
{ 
	return emit_ptr == 0 ? (void*)&CodeCache[LastAddr] : (void*)emit_ptr; 
}

void emit_SetBaseAddr() 
{ 
	LastAddr_min = LastAddr; 
}

// Wii-specific: flush instruction cache after code generation
#if HOST_OS == OS_WII
static inline void emit_FlushCache(void* start, u32 size)
{
	DCFlushRange(start, size);
	ICInvalidateRange(start, size);
}
#endif

void emit_Write8(u8 data)
{
	verify(!emit_ptr);
	if (LastAddr >= CODE_SIZE - 1)
	{
		printf("ERROR: Code cache overflow in emit_Write8!\n");
		recSh4_ClearCache();
		return;
	}
	CodeCache[LastAddr] = data;
	LastAddr += 1;
}

void emit_Write16(u16 data)
{
	verify(!emit_ptr);
	if (LastAddr >= CODE_SIZE - 2)
	{
		printf("ERROR: Code cache overflow in emit_Write16!\n");
		recSh4_ClearCache();
		return;
	}
	*(u16*)&CodeCache[LastAddr] = data;
	LastAddr += 2;
}

void emit_Write32(u32 data)
{
	if (emit_ptr)
	{
		*emit_ptr = data;
		emit_ptr++;
	}
	else
	{
		if (LastAddr >= CODE_SIZE - 4)
		{
			printf("ERROR: Code cache overflow in emit_Write32!\n");
			recSh4_ClearCache();
			return;
		}
		*(u32*)&CodeCache[LastAddr] = data;
		LastAddr += 4;
	}
}

void emit_Skip(u32 sz)
{
	if (LastAddr + sz > CODE_SIZE)
	{
		printf("ERROR: Code cache overflow in emit_Skip!\n");
		recSh4_ClearCache();
		return;
	}
	LastAddr += sz;
}

u32 emit_FreeSpace()
{
	return CODE_SIZE - LastAddr;
}

// ============================================================================
// Code Cache Management Functions
// ============================================================================

void emit_WriteCodeCache()
{
	char path[512];
	sprintf(path, "code_cache_%08X.bin", (u32)CodeCache);
	printf("Writing code cache to %s\n", path);
	
	FILE* f = fopen(path, "wb");
	if (f)
	{
		fwrite(CodeCache, LastAddr, 1, f);
		fclose(f);
		printf("Code cache written: %u bytes\n", LastAddr);
	}
	else
	{
		printf("ERROR: Failed to write code cache!\n");
	}
}

void recSh4_ClearCache()
{
	LastAddr = LastAddr_min;
	bm_Reset();
	
#ifdef ENABLE_PERF_MONITORING
	perf_cache_clears++;
	printf("recSh4: Dynarec cache cleared at %08X (%u clears total)\n", 
	       curr_pc, perf_cache_clears);
#else
	printf("recSh4: Dynarec cache cleared at %08X\n", curr_pc);
#endif
	
#if HOST_OS == OS_WII
	// Wii-specific: invalidate instruction cache
	ICInvalidateRange(CodeCache, CODE_SIZE);
	cache_high_water_mark = LastAddr;
#endif
}

// ============================================================================
// Block Analysis and Compilation
// ============================================================================

// Check if we should enable extra debugging for specific addresses
bool DoCheck(u32 pc)
{
	if (IsOnRam(pc))
	{
		pc &= 0xFFFFFF;
		switch(pc)
		{
			// Add problematic addresses here for debugging
			case 0x3DAFC6:
			case 0x3C83F8:
				return true;
			default:
				return false;
		}
	}
	return false;
}

void AnalyseBlock(DecodedBlock* blk);

// Compile a block at given PC with improved error handling
DynarecCodeEntry* rdv_CompileBlock(u32 bpc)
{
	// Check if we're running low on cache space
	u32 free_space = emit_FreeSpace();
	if (free_space < 4096)  // Less than 4 KB free
	{
		printf("WARNING: Low code cache space (%u bytes), clearing...\n", free_space);
		recSh4_ClearCache();
	}
	
#if HOST_OS == OS_WII
	// Track high water mark
	if (LastAddr > cache_high_water_mark)
	{
		cache_high_water_mark = LastAddr;
		
		// Warn if approaching capacity
		if (cache_high_water_mark > CACHE_PRESSURE_THRESHOLD)
		{
			printf("WARNING: Code cache pressure high (%u%% used)\n", 
			       (cache_high_water_mark * 100) / CODE_SIZE);
		}
	}
#endif
	
	// Decode block
	DecodedBlock* blk = dec_DecodeBlock(bpc, fpscr, (SH4_TIMESLICE / 2));
	if (!blk)
	{
		printf("ERROR: Failed to decode block at %08X\n", bpc);
		return 0;
	}
	
	// Analyze block
	AnalyseBlock(blk);
	
	// Get starting address for cache flush
#if HOST_OS == OS_WII
	void* code_start = emit_GetCCPtr();
#endif
	
	// Compile block
	DynarecCodeEntry* rv = ngen_Compile(blk, DoCheck(blk->start));
	
#if HOST_OS == OS_WII
	// Flush instruction cache for newly compiled code
	if (rv)
	{
		u32 code_size = (u8*)emit_GetCCPtr() - (u8*)code_start;
		emit_FlushCache(code_start, code_size);
	}
#endif
	
	// Cleanup
	dec_Cleanup();
	
#ifdef ENABLE_PERF_MONITORING
	if (rv)
		perf_blocks_compiled++;
#endif
	
	return rv;
}

// ============================================================================
// Block Lookup and Execution
// ============================================================================

u32 rdv_FailedToFindBlock_pc;

DynarecCodeEntry* rdv_CompilePC()
{
	u32 pc = next_pc;
	DynarecCodeEntry* rv;
	
	rv = rdv_CompileBlock(pc);
	
	if (rv == 0)
	{
		// Compilation failed, clear cache and retry
		printf("WARNING: Block compilation failed at %08X, retrying...\n", pc);
		recSh4_ClearCache();
		rv = rdv_CompileBlock(pc);
		
		if (rv == 0)
		{
			// Still failed, this is serious
			printf("FATAL: Failed to compile block at %08X after cache clear!\n", pc);
			// Fall back to interpreter for this block
			return 0;
		}
	}
	
	bm_AddCode(pc, rv);
	
	// Check if we're booting - reset cache after boot to free up memory
	if ((pc & 0xFFFFFF) == 0x08300 || (pc & 0xFFFFFF) == 0x10000)
	{
		printf("Boot detected at %08X, scheduling cache reset\n", pc);
		// **NOTE** The block must still be valid after this
		// We clear the cache but the current block remains valid
		recSh4_ClearCache();
	}
	
	return rv;
}

DynarecCodeEntry* rdv_FailedToFindBlock()
{
	next_pc = rdv_FailedToFindBlock_pc;
	
	printf("rdv_FailedToFindBlock ~ %08X\n", next_pc);
	
	return rdv_CompilePC();
}

DynarecCodeEntry* FASTCALL rdv_BlockCheckFail(u32 pc)
{
	next_pc = pc;
	
#ifdef ENABLE_PERF_MONITORING
	perf_block_checks_failed++;
#endif
	
	printf("Block check failed at %08X, recompiling...\n", pc);
	
	// Remove the invalid block
	bm_RemoveCode(pc);
	
	return rdv_CompilePC();
}

DynarecCodeEntry* rdv_FindCode()
{
	DynarecCodeEntry* rv = bm_GetCode(next_pc);
	if (rv == ngen_FailedToFindBlock)
		return 0;
	
	return rv;
}

DynarecCodeEntry* rdv_FindOrCompile()
{
	DynarecCodeEntry* rv = bm_GetCode(next_pc);
	if (rv == ngen_FailedToFindBlock)
		rv = rdv_CompilePC();
	
	return rv;
}

// ============================================================================
// Main Execution Loop
// ============================================================================

void recSh4_Run()
{
	sh4_int_bCpuRun = true;
	
	printf("recSh4: Starting dynarec execution\n");
	
#ifdef ENABLE_PERF_MONITORING
	u32 start_blocks = perf_blocks_compiled;
#endif
	
	ngen_mainloop();
	
#ifdef ENABLE_PERF_MONITORING
	printf("recSh4: Execution stopped. Compiled %u new blocks\n", 
	       perf_blocks_compiled - start_blocks);
#endif
	
	sh4_int_bCpuRun = false;
}

// ============================================================================
// Emulator Control Functions
// ============================================================================

void recSh4_Stop()
{
	Sh4_int_Stop();
}

void recSh4_Step()
{
	Sh4_int_Step();
}

void recSh4_Skip()
{
	Sh4_int_Skip();
}

void recSh4_Reset(bool Manual)
{
	Sh4_int_Reset(Manual);
	recSh4_ClearCache();
}

// ============================================================================
// Initialization and Cleanup
// ============================================================================

void recSh4_Init()
{
	printf("recSh4: Initializing dynarec\n");
	
	Sh4_int_Init();
	
	// Initialize block manager with pre-allocation
	bm_Init();
	
#ifdef ENABLE_PERF_MONITORING
	perf_blocks_compiled = 0;
	perf_cache_clears = 0;
	perf_block_checks_failed = 0;
#endif
	
	// Platform-specific code cache setup
#if HOST_OS == OS_WINDOWS
	DWORD old;
	VirtualProtect(CodeCache, CODE_SIZE, PAGE_EXECUTE_READWRITE, &old);
	printf("recSh4: Code cache allocated at %p (Windows)\n", CodeCache);
	
#elif HOST_OS == OS_LINUX
	// Align to page boundary
	CodeCache = (u8*)(((unat)pMemBuff + 4095) & ~4095);
	
	printf("recSh4: Code cache addr: %p | from: %p\n", CodeCache, pMemBuff);
	
	if (mprotect(CodeCache, CODE_SIZE, PROT_EXEC | PROT_READ | PROT_WRITE))
	{
		perror("ERROR: Couldn't mprotect CodeCache!");
		verify(false);
	}
	
	memset(CodeCache, 0xCD, CODE_SIZE);
	
#elif HOST_OS == OS_WII
	// Wii-specific: ensure code cache is properly aligned and cached
	printf("recSh4: Code cache allocated at %p (Wii, %u KB)\n", 
	       CodeCache, CODE_SIZE / 1024);
	
	// Clear code cache
	memset(CodeCache, 0x00, CODE_SIZE);
	
	// Flush data cache and invalidate instruction cache
	DCFlushRange(CodeCache, CODE_SIZE);
	ICInvalidateRange(CodeCache, CODE_SIZE);
	
	cache_high_water_mark = 0;
	
	printf("recSh4: Wii code cache initialized and flushed\n");
#endif
	
	LastAddr = 0;
	LastAddr_min = 0;
	emit_ptr = 0;
	
	printf("recSh4: Initialization complete\n");
}

void recSh4_Term()
{
	printf("recSh4: Terminating dynarec\n");
	
#ifdef ENABLE_PERF_MONITORING
	printf("recSh4 Performance Stats:\n");
	printf("  - Blocks compiled: %u\n", perf_blocks_compiled);
	printf("  - Cache clears: %u\n", perf_cache_clears);
	printf("  - Block check failures: %u\n", perf_block_checks_failed);
	printf("  - Code cache usage: %u / %u bytes (%.1f%%)\n", 
	       LastAddr, CODE_SIZE, (LastAddr * 100.0f) / CODE_SIZE);
#if HOST_OS == OS_WII
	printf("  - High water mark: %u bytes (%.1f%%)\n",
	       cache_high_water_mark, (cache_high_water_mark * 100.0f) / CODE_SIZE);
#endif
	
#ifdef BM_ENABLE_STATS
	u32 hits, misses, cache_hits, total_blocks;
	bm_GetStats(&hits, &misses, &cache_hits, &total_blocks);
	printf("  - Block manager hits: %u\n", hits);
	printf("  - Block manager misses: %u\n", misses);
	printf("  - Block manager cache hits: %u\n", cache_hits);
	printf("  - Total blocks: %u\n", total_blocks);
	if (hits + cache_hits > 0)
		printf("  - Cache efficiency: %.2f%%\n", 
		       (cache_hits * 100.0f) / (hits + cache_hits));
#endif
#endif
	
	Sh4_int_Term();
	
#if HOST_OS == OS_LINUX
	// Linux cleanup if needed
#elif HOST_OS == OS_WII
	// Wii cleanup if needed
	printf("recSh4: Wii-specific cleanup complete\n");
#endif
}

bool recSh4_IsCpuRunning()
{
	return Sh4_int_IsCpuRunning();
}

// ============================================================================
// Debug and Utility Functions
// ============================================================================

// Get code cache statistics
void recSh4_GetCacheStats(u32* total_size, u32* used_size, u32* free_size)
{
	if (total_size) *total_size = CODE_SIZE;
	if (used_size) *used_size = LastAddr;
	if (free_size) *free_size = CODE_SIZE - LastAddr;
}

#ifdef ENABLE_PERF_MONITORING
void recSh4_GetPerfStats(u32* blocks_compiled, u32* cache_clears, u32* check_failures)
{
	if (blocks_compiled) *blocks_compiled = perf_blocks_compiled;
	if (cache_clears) *cache_clears = perf_cache_clears;
	if (check_failures) *check_failures = perf_block_checks_failed;
}

void recSh4_ResetPerfStats()
{
	perf_blocks_compiled = 0;
	perf_cache_clears = 0;
	perf_block_checks_failed = 0;
}
#endif

#endif // HOST_NO_REC

// ============================================================================
// Interface Setup
// ============================================================================

void Get_Sh4Recompiler(sh4_if* rv)
{
#ifdef HOST_NO_REC
	Get_Sh4Interpreter(rv);
#else
	rv->Run = recSh4_Run;
	rv->Stop = recSh4_Stop;
	rv->Step = recSh4_Step;
	rv->Skip = recSh4_Skip;
	rv->Reset = recSh4_Reset;
	rv->Init = recSh4_Init;
	rv->Term = recSh4_Term;
	rv->IsCpuRunning = recSh4_IsCpuRunning;
	rv->ResetCache = recSh4_ClearCache;
#endif
}
