#pragma once
#include "types.h"

/**
 * @file pvr_sb_regs.h
 * @brief PVR (PowerVR) System Bus Register Interface
 * 
 * Handles PVR DMA operations and system bus register interactions
 * for the Dreamcast's PowerVR graphics chip.
 */

//==============================================================================
// Register Read/Write Interface
//==============================================================================

/**
 * @brief Read from PVR system bus register
 * @param addr Register address
 * @param sz Size of read operation (in bytes)
 * @return Register value
 */
u32 pvr_sb_readreg_Pvr(u32 addr, u32 sz);

/**
 * @brief Write to PVR system bus register
 * @param addr Register address
 * @param data Data to write
 * @param sz Size of write operation (in bytes)
 */
void pvr_sb_writereg_Pvr(u32 addr, u32 data, u32 sz);

//==============================================================================
// Lifecycle Management
//==============================================================================

/**
 * @brief Initialize PVR system bus register handlers
 * 
 * Sets up register callbacks and initializes DMA subsystem.
 * Must be called before any register access.
 */
void pvr_sb_Init();

/**
 * @brief Cleanup PVR system bus resources
 * 
 * Releases any resources allocated during initialization.
 */
void pvr_sb_Term();

/**
 * @brief Reset PVR system bus to initial state
 * @param Manual True if manual reset, false if automatic
 * 
 * Resets all DMA state and register values.
 */
void pvr_sb_Reset(bool Manual);
