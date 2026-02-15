#pragma once
#include "types.h"
#include "sh4_interpreter.h"

// ============================================================================
// SH4 FPU INSTRUCTION DECLARATIONS
// ============================================================================
// This header declares all SH4 FPU instruction handlers
// Each instruction is implemented in sh4_fpu.cpp
// ============================================================================

// ----------------------------------------------------------------------------
// ARITHMETIC OPERATIONS
// ----------------------------------------------------------------------------

//fadd <FREG_M>,<FREG_N>
// Single: FR[n] = FR[n] + FR[m]
// Double: DR[n] = DR[n] + DR[m]
sh4op(i1111_nnnn_mmmm_0000);

//fsub <FREG_M>,<FREG_N>
// Single: FR[n] = FR[n] - FR[m]
// Double: DR[n] = DR[n] - DR[m]
sh4op(i1111_nnnn_mmmm_0001);

//fmul <FREG_M>,<FREG_N>
// Single: FR[n] = FR[n] * FR[m]
// Double: DR[n] = DR[n] * DR[m]
sh4op(i1111_nnnn_mmmm_0010);

//fdiv <FREG_M>,<FREG_N>
// Single: FR[n] = FR[n] / FR[m]
// Double: DR[n] = DR[n] / DR[m]
sh4op(i1111_nnnn_mmmm_0011);

//fmac <FREG_0>,<FREG_M>,<FREG_N>
// Single: FR[n] = FR[0] * FR[m] + FR[n]
// Fused multiply-add for better accuracy
sh4op(i1111_nnnn_mmmm_1110);

// ----------------------------------------------------------------------------
// COMPARISON OPERATIONS
// ----------------------------------------------------------------------------

//fcmp/eq <FREG_M>,<FREG_N>
// T = (FR[n] == FR[m])
sh4op(i1111_nnnn_mmmm_0100);

//fcmp/gt <FREG_M>,<FREG_N>
// T = (FR[n] > FR[m])
sh4op(i1111_nnnn_mmmm_0101);

// ----------------------------------------------------------------------------
// MEMORY OPERATIONS
// ----------------------------------------------------------------------------

//fmov.s @(R0,<REG_M>),<FREG_N>
// Load: FR[n] = [R[m] + R0]
sh4op(i1111_nnnn_mmmm_0110);

//fmov.s <FREG_M>,@(R0,<REG_N>)
// Store: [R[n] + R0] = FR[m]
sh4op(i1111_nnnn_mmmm_0111);

//fmov.s @<REG_M>,<FREG_N>
// Load: FR[n] = [R[m]]
sh4op(i1111_nnnn_mmmm_1000);

//fmov.s @<REG_M>+,<FREG_N>
// Load with post-increment: FR[n] = [R[m]], R[m] += 4/8
sh4op(i1111_nnnn_mmmm_1001);

//fmov.s <FREG_M>,@<REG_N>
// Store: [R[n]] = FR[m]
sh4op(i1111_nnnn_mmmm_1010);

//fmov.s <FREG_M>,@-<REG_N>
// Store with pre-decrement: R[n] -= 4/8, [R[n]] = FR[m]
sh4op(i1111_nnnn_mmmm_1011);

//fmov <FREG_M>,<FREG_N>
// Register-to-register move: FR[n] = FR[m]
sh4op(i1111_nnnn_mmmm_1100);

// ----------------------------------------------------------------------------
// SINGLE OPERAND OPERATIONS
// ----------------------------------------------------------------------------

//fabs <FREG_N>
// FR[n] = |FR[n]| (clear sign bit)
sh4op(i1111_nnnn_0101_1101);

//fneg <FREG_N>
// FR[n] = -FR[n] (toggle sign bit)
sh4op(i1111_nnnn_0100_1101);

//fsqrt <FREG_N>
// Single: FR[n] = sqrt(FR[n])
// Double: DR[n] = sqrt(DR[n])
sh4op(i1111_nnnn_0110_1101);

// ----------------------------------------------------------------------------
// TRANSCENDENTAL OPERATIONS
// ----------------------------------------------------------------------------

//FSCA FPUL, DRn
// Sine/Cosine approximation
// FR[n] = sin(2*pi*FPUL/65536), FR[n+1] = cos(2*pi*FPUL/65536)
sh4op(i1111_nnn0_1111_1101);

//FSRRA <FREG_N>
// Reciprocal square root approximation: FR[n] = 1/sqrt(FR[n])
sh4op(i1111_nnnn_0111_1101);

// ----------------------------------------------------------------------------
// CONVERSION OPERATIONS
// ----------------------------------------------------------------------------

//fcnvds <DR_N>,FPUL
// Convert double to single: FPUL = (float)DR[n]
sh4op(i1111_nnnn_1011_1101);

//fcnvsd FPUL,<DR_N>
// Convert single to double: DR[n] = (double)FPUL
sh4op(i1111_nnnn_1010_1101);

//float FPUL,<FREG_N>
// Convert integer to float
// Single: FR[n] = (float)(s32)FPUL
// Double: DR[n] = (double)(s32)FPUL
sh4op(i1111_nnnn_0010_1101);

//ftrc <FREG_N>, FPUL
// Truncate float to integer
// Single: FPUL = (s32)FR[n]
// Double: FPUL = (s32)DR[n]
sh4op(i1111_nnnn_0011_1101);

// ----------------------------------------------------------------------------
// VECTOR OPERATIONS
// ----------------------------------------------------------------------------

//fipr <FV_M>,<FV_N>
// Inner product: FR[n+3] = FR[n+0]*FR[m+0] + FR[n+1]*FR[m+1] + 
//                          FR[n+2]*FR[m+2] + FR[n+3]*FR[m+3]
sh4op(i1111_nnmm_1110_1101);

//ftrv xmtrx,<FV_N>
// Transform vector by matrix: FV[n] = XMTRX * FV[n]
// Matrix-vector multiplication using XF bank
sh4op(i1111_nn01_1111_1101);

// ----------------------------------------------------------------------------
// CONSTANT LOAD OPERATIONS
// ----------------------------------------------------------------------------

//fldi0 <FREG_N>
// Load immediate 0.0: FR[n] = 0.0f
sh4op(i1111_nnnn_1000_1101);

//fldi1 <FREG_N>
// Load immediate 1.0: FR[n] = 1.0f
sh4op(i1111_nnnn_1001_1101);

// ----------------------------------------------------------------------------
// FPUL TRANSFER OPERATIONS
// ----------------------------------------------------------------------------

//flds <FREG_N>,FPUL
// Load from FR to FPUL: FPUL = FR[n]
sh4op(i1111_nnnn_0001_1101);

//fsts FPUL,<FREG_N>
// Store from FPUL to FR: FR[n] = FPUL
sh4op(i1111_nnnn_0000_1101);

// ----------------------------------------------------------------------------
// CONTROL OPERATIONS
// ----------------------------------------------------------------------------

//frchg
// Toggle FR bit: Toggle between FR0-15 and XF0-15 banks
sh4op(i1111_1011_1111_1101);

//fschg
// Toggle SZ bit: Toggle between single (32-bit) and paired (64-bit) mode
sh4op(i1111_0011_1111_1101);

// ============================================================================
// NOTES ON SH4 FPU
// ============================================================================
//
// PRECISION MODES:
// - fpscr.PR = 0: Single precision (32-bit float operations)
// - fpscr.PR = 1: Double precision (64-bit double operations)
//
// SIZE MODES:
// - fpscr.SZ = 0: Single transfer (fmov transfers 32 bits)
// - fpscr.SZ = 1: Paired transfer (fmov transfers 64 bits)
//
// REGISTER BANKS:
// - fpscr.FR = 0: Use FR0-FR15
// - fpscr.FR = 1: Use XF0-XF15 (toggled by frchg)
//
// SPECIAL REGISTERS:
// - FPUL: FP communication register (32-bit)
// - FPSCR: FP status/control register
//
// NaN HANDLING:
// - NaN values are normalized to a canonical form (0x7FFFFFFF or 0xFFFFFFFF)
// - This improves accuracy and matches hardware behavior
//
// DENORMAL HANDLING:
// - When fpscr.DN=1, denormal numbers are flushed to zero
// - Preserves sign bit when flushing
//
// ============================================================================
