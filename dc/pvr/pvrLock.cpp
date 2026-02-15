/*
 * PowerVR VRAM Address Conversion - Optimized Implementation
 * 
 * Provides optimized VRAM address conversion functions.
 * These are compiled once and linked, which works well with GCC's
 * link-time optimization (-flto) for cross-module inlining.
 */

#include "plugins/plugin_manager.h"
#include "pvrLock.h"

using namespace std;

// VRAM array (supports both 32-bit and 64-bit addressing modes)
VArray2 vram;

//==============================================================================
// Optimized Conversion Functions
// 
// Note: With GCC's -O3 and link-time optimization, these will be inlined
// across compilation units automatically where beneficial.
//==============================================================================

/**
 * Convert SH4 address to 32-bit VRAM offset
 * 
 * In 64-bit mode: Deinterleaves banks into sequential offset
 * In 32-bit mode: Direct masking
 */
u32 vramlock_ConvAddrtoOffset32(u32 Address)
{
	if (Is_64_Bit(Address))
	{
		// 64-bit mode: Deinterleave banks
		// Extract: bank bit (bit 2), upper offset bits, lower 2 bits
		u32 upper = (Address >> 1) & ((VRAM_MASK >> 1) - 3);  // Upper offset
		u32 lower = Address & 0x3;                             // Preserve bits 0-1
		u32 bank_offset = (Address & (1 << 2)) << 20;         // Bank bit → bit 22
		
		return bank_offset | upper | lower;
	}
	else
	{
		// 32-bit mode: Direct sequential addressing
		return Address & VRAM_MASK;
	}
}

/**
 * Convert 64-bit VRAM offset to 32-bit VRAM offset
 * 
 * Deinterleaves the 64-bit banked layout into sequential 32-bit offset.
 */
u32 vramlock_ConvOffset64toOffset32(u32 offset64)
{
	// Deinterleave: extract bank and repack
	u32 upper = (offset64 >> 1) & (VRAM_MASK - 3);  // Upper offset bits
	u32 lower = offset64 & 0x3;                      // Preserve bits 0-1
	u32 bank_offset = (offset64 & (1 << 2)) << 20;  // Bank bit → bit 22
	
	return bank_offset | upper | lower;
}

/**
 * Convert SH4 address to 64-bit VRAM offset
 * 
 * In 64-bit mode: Direct masking
 * In 32-bit mode: Interleave banks
 */
u32 vramlock_ConvAddrtoOffset64(u32 Address)
{
	if (Is_64_Bit(Address))
	{
		// 64-bit mode: Direct offset (already interleaved)
		return Address & VRAM_MASK;
	}
	else
	{
		// 32-bit mode: Need to interleave banks
		u32 bank = ((Address >> 22) & 0x1) << 2;       // Bank selection → bit 2
		u32 lower = Address & 0x3;                      // Preserve bits 0-1
		u32 addr_shifted = (Address & 0x3FFFFC) << 1;  // Shift middle bits
		
		return addr_shifted | bank | lower;
	}
}

/**
 * Convert 32-bit VRAM offset to 64-bit VRAM offset
 * 
 * Interleaves sequential 32-bit offset into 64-bit banked layout.
 */
u32 vramlock_ConvOffset32toOffset64(u32 offset32)
{
	// Mask to valid VRAM range
	offset32 &= VRAM_MASK;
	
	// Extract bank from bit 22
	u32 bank = ((offset32 >> 22) & 0x1) << 2;       // Bank selection → bit 2
	u32 lower = offset32 & 0x3;                      // Preserve bits 0-1
	u32 addr_shifted = (offset32 & 0x3FFFFC) << 1;  // Shift middle bits
	
	return addr_shifted | bank | lower;
}
