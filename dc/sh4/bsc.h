#pragma once

// Bus State Controller (BSC) register definitions
// SH4 CPU memory bus interface for Dreamcast emulation
//
// Supports both little-endian (x86) and big-endian (Wii/PowerPC) hosts
// via HOST_ENDIAN bitfield ordering.

#include "types.h"

// Cable type constants used by read_BSC_PDTRA() for BIOS detection
// The Dreamcast BIOS probes PCTRA/PDTRA pin directions to identify the
// connected video cable.  These values are shifted into bits [9:8] of
// the returned PDTRA value.
#define DC_CABLE_VGA       0  // VGA / RGB (480p)
#define DC_CABLE_RGB       2  // RGB SCART
#define DC_CABLE_COMPOSITE 3  // Composite / S-Video

// Init / Reset / Term
void bsc_Init();
void bsc_Reset(bool Manual);
void bsc_Term();

// ============================================================
// BCR1 – Bus Control Register 1 (32-bit)
// All bits except A0MPX, MASTER, ENDIAN are writable and reset to 0.
// ENDIAN is always 1 on Dreamcast (big-endian bus).
// MASTER reflects whether the SH4 is the bus master.
// ============================================================
union BCR1_type
{
    struct
    {
#if HOST_ENDIAN == ENDIAN_LITTLE
        u32 A56PCM  : 1;
        u32 res_0   : 1;
        u32 DRAMTP0 : 1;
        u32 DRAMTP1 : 1;
        u32 DRAMTP2 : 1;
        u32 A6BST0  : 1;
        u32 A6BST1  : 1;
        u32 A6BST2  : 1;
        // byte 1
        u32 A5BST0  : 1;
        u32 A5BST1  : 1;
        u32 A5BST2  : 1;
        u32 A0BST0  : 1;
        u32 A0BST1  : 1;
        u32 A0BST2  : 1;
        u32 HIZCNT  : 1;
        u32 HIZMEM  : 1;
        // byte 2
        u32 res_1   : 1;
        u32 MEMMPX  : 1;
        u32 PSHR    : 1;
        u32 BREQEN  : 1;
        u32 A4MBC   : 1;
        u32 A1MBC   : 1;
        u32 res_2   : 1;
        u32 res_3   : 1;
        // byte 3
        u32 OPUP    : 1;
        u32 IPUP    : 1;
        u32 res_4   : 1;
        u32 res_5   : 1;
        u32 res_6   : 1;
        u32 A0MPX   : 1;  // 1 = area 0 is MPX
        u32 MASTER  : 1;  // Bus master flag
        u32 ENDIAN  : 1;  // Always 1 on Dreamcast (big-endian)
#else
        u32 ENDIAN  : 1;
        u32 MASTER  : 1;
        u32 A0MPX   : 1;
        u32 res_6   : 1;
        u32 res_5   : 1;
        u32 res_4   : 1;
        u32 IPUP    : 1;
        u32 OPUP    : 1;

        u32 res_3   : 1;
        u32 res_2   : 1;
        u32 A1MBC   : 1;
        u32 A4MBC   : 1;
        u32 BREQEN  : 1;
        u32 PSHR    : 1;
        u32 MEMMPX  : 1;
        u32 res_1   : 1;

        u32 HIZMEM  : 1;
        u32 HIZCNT  : 1;
        u32 A0BST2  : 1;
        u32 A0BST1  : 1;
        u32 A0BST0  : 1;
        u32 A5BST2  : 1;
        u32 A5BST1  : 1;
        u32 A5BST0  : 1;

        u32 A6BST2  : 1;
        u32 A6BST1  : 1;
        u32 A6BST0  : 1;
        u32 DRAMTP2 : 1;
        u32 DRAMTP1 : 1;
        u32 DRAMTP0 : 1;
        u32 res_0   : 1;
        u32 A56PCM  : 1;
#endif
    };

    u32 full;
};

extern BCR1_type BSC_BCR1;

// ============================================================
// BCR2 – Bus Control Register 2 (16-bit)
// A0SZ0_inp / A0SZ1_inp are read-only hardware pins.
// NOTE: bitfield members use u16 to match the 16-bit register.
// ============================================================
union BCR2_type
{
    struct
    {
#if HOST_ENDIAN == ENDIAN_LITTLE
        u16 PORTEN    : 1;
        u16 res_0     : 1;
        u16 A0SZ0     : 1;   // NOTE: manual typo – listed as A0SZ0 twice; second entry is A0SZ1
        u16 A0SZ1     : 1;   // corrected field name (was A1SZ1 in original – typo)
        u16 A2SZ0     : 1;
        u16 A2SZ1     : 1;
        u16 A3SZ0     : 1;
        u16 A3SZ1     : 1;
        // byte 1
        u16 A4SZ0     : 1;
        u16 A4SZ1     : 1;
        u16 A5SZ0     : 1;
        u16 A5SZ1     : 1;
        u16 A6SZ0     : 1;
        u16 A6SZ1     : 1;
        u16 A0SZ0_inp : 1;   // read-only input pin
        u16 A0SZ1_inp : 1;   // read-only input pin
#else
        u16 A0SZ1_inp : 1;
        u16 A0SZ0_inp : 1;
        u16 A6SZ1     : 1;
        u16 A6SZ0     : 1;
        u16 A5SZ1     : 1;
        u16 A5SZ0     : 1;
        u16 A4SZ1     : 1;
        u16 A4SZ0     : 1;

        u16 A3SZ1     : 1;
        u16 A3SZ0     : 1;
        u16 A2SZ1     : 1;
        u16 A2SZ0     : 1;
        u16 A0SZ1     : 1;
        u16 A0SZ0     : 1;
        u16 res_0     : 1;
        u16 PORTEN    : 1;
#endif
    };

    u16 full;
};

extern BCR2_type BSC_BCR2;

// ============================================================
// WCR1 – Wait Control Register 1 (32-bit)
// Inter-memory-cycle wait states for areas 0-6 and DMA.
// ============================================================
union WCR1_type
{
    struct
    {
#if HOST_ENDIAN == ENDIAN_LITTLE
        u32 A0IW0   : 1;
        u32 A0IW1   : 1;
        u32 A0IW2   : 1;
        u32 res_0   : 1;
        u32 A1IW0   : 1;
        u32 A1IW1   : 1;
        u32 A1IW2   : 1;
        u32 res_1   : 1;

        u32 A2IW0   : 1;
        u32 A2IW1   : 1;
        u32 A2IW2   : 1;
        u32 res_2   : 1;
        u32 A3IW0   : 1;
        u32 A3IW1   : 1;
        u32 A3IW2   : 1;
        u32 res_3   : 1;

        u32 A4IW0   : 1;
        u32 A4IW1   : 1;
        u32 A4IW2   : 1;
        u32 res_4   : 1;
        u32 A5IW0   : 1;
        u32 A5IW1   : 1;
        u32 A5IW2   : 1;
        u32 res_5   : 1;

        u32 A6IW0   : 1;
        u32 A6IW1   : 1;
        u32 A6IW2   : 1;
        u32 res_6   : 1;
        u32 DMAIW0  : 1;
        u32 DMAIW1  : 1;
        u32 DMAIW2  : 1;
        u32 res_7   : 1;
#else
        u32 res_7   : 1;
        u32 DMAIW2  : 1;
        u32 DMAIW1  : 1;
        u32 DMAIW0  : 1;
        u32 res_6   : 1;
        u32 A6IW2   : 1;
        u32 A6IW1   : 1;
        u32 A6IW0   : 1;

        u32 res_5   : 1;
        u32 A5IW2   : 1;
        u32 A5IW1   : 1;
        u32 A5IW0   : 1;
        u32 res_4   : 1;
        u32 A4IW2   : 1;
        u32 A4IW1   : 1;
        u32 A4IW0   : 1;

        u32 res_3   : 1;
        u32 A3IW2   : 1;
        u32 A3IW1   : 1;
        u32 A3IW0   : 1;
        u32 res_2   : 1;
        u32 A2IW2   : 1;
        u32 A2IW1   : 1;
        u32 A2IW0   : 1;

        u32 res_1   : 1;
        u32 A1IW2   : 1;
        u32 A1IW1   : 1;
        u32 A1IW0   : 1;
        u32 res_0   : 1;
        u32 A0IW2   : 1;
        u32 A0IW1   : 1;
        u32 A0IW0   : 1;
#endif
    };

    u32 full;
};

extern WCR1_type BSC_WCR1;

// ============================================================
// WCR2 – Wait Control Register 2 (32-bit)
// Burst/wait cycles for areas 0-6.
// ============================================================
union WCR2_type
{
    struct
    {
#if HOST_ENDIAN == ENDIAN_LITTLE
        u32 A0B0  : 1;
        u32 A0B1  : 1;
        u32 A0B2  : 1;
        u32 A0W0  : 1;
        u32 A0W1  : 1;
        u32 A0W2  : 1;
        u32 A1W0  : 1;
        u32 A1W1  : 1;

        u32 A1W2  : 1;
        u32 A2W0  : 1;
        u32 A2W1  : 1;
        u32 A2W2  : 1;
        u32 res_0 : 1;
        u32 A3W0  : 1;
        u32 A3W1  : 1;
        u32 A3W2  : 1;

        u32 res_1 : 1;
        u32 A4W0  : 1;
        u32 A4W1  : 1;
        u32 A4W2  : 1;
        u32 A5B0  : 1;
        u32 A5B1  : 1;
        u32 A5B2  : 1;
        u32 A5W0  : 1;

        u32 A5W1  : 1;
        u32 A5W2  : 1;
        u32 A6B0  : 1;
        u32 A6B1  : 1;
        u32 A6B2  : 1;
        u32 A6W0  : 1;
        u32 A6W1  : 1;
        u32 A6W2  : 1;
#else
        u32 A6W2  : 1;
        u32 A6W1  : 1;
        u32 A6W0  : 1;
        u32 A6B2  : 1;
        u32 A6B1  : 1;
        u32 A6B0  : 1;
        u32 A5W2  : 1;
        u32 A5W1  : 1;

        u32 A5W0  : 1;
        u32 A5B2  : 1;
        u32 A5B1  : 1;
        u32 A5B0  : 1;
        u32 A4W2  : 1;
        u32 A4W1  : 1;
        u32 A4W0  : 1;
        u32 res_1 : 1;

        u32 A3W2  : 1;
        u32 A3W1  : 1;
        u32 A3W0  : 1;
        u32 res_0 : 1;
        u32 A2W2  : 1;
        u32 A2W1  : 1;
        u32 A2W0  : 1;
        u32 A1W2  : 1;

        u32 A1W1  : 1;
        u32 A1W0  : 1;
        u32 A0W2  : 1;
        u32 A0W1  : 1;
        u32 A0W0  : 1;
        u32 A0B2  : 1;
        u32 A0B1  : 1;
        u32 A0B0  : 1;
#endif
    };

    u32 full;
};

extern WCR2_type BSC_WCR2;

// ============================================================
// WCR3 – Wait Control Register 3 (32-bit)
// Data hold / setup cycles for areas 0-6.
// ============================================================
union WCR3_type
{
    struct
    {
#if HOST_ENDIAN == ENDIAN_LITTLE
        u32 A0H0   : 1;
        u32 A0H1   : 1;
        u32 A0S0   : 1;
        u32 res_0  : 1;
        u32 A1H0   : 1;  // NOTE: SH4 manual may list this as A1H0 – confirmed correct
        u32 A1H1   : 1;
        u32 A1S0   : 1;
        u32 res_1  : 1;

        u32 A2H0   : 1;
        u32 A2H1   : 1;
        u32 A2S0   : 1;
        u32 res_2  : 1;
        u32 A3H0   : 1;
        u32 A3H1   : 1;
        u32 A3S0   : 1;
        u32 res_3  : 1;

        u32 A4H0   : 1;
        u32 A4H1   : 1;
        u32 A4S0   : 1;
        u32 res_4  : 1;
        u32 A5H0   : 1;
        u32 A5H1   : 1;
        u32 A5S0   : 1;
        u32 res_5  : 1;

        u32 A6H0   : 1;
        u32 A6H1   : 1;
        u32 A6S0   : 1;
        u32 res_6  : 1;
        u32 res_7  : 1;
        u32 res_8  : 1;
        u32 res_9  : 1;
        u32 res_10 : 1;
#else
        u32 res_10 : 1;
        u32 res_9  : 1;
        u32 res_8  : 1;
        u32 res_7  : 1;
        u32 res_6  : 1;
        u32 A6S0   : 1;
        u32 A6H1   : 1;
        u32 A6H0   : 1;

        u32 res_5  : 1;
        u32 A5S0   : 1;
        u32 A5H1   : 1;
        u32 A5H0   : 1;
        u32 res_4  : 1;
        u32 A4S0   : 1;
        u32 A4H1   : 1;
        u32 A4H0   : 1;

        u32 res_3  : 1;
        u32 A3S0   : 1;
        u32 A3H1   : 1;
        u32 A3H0   : 1;
        u32 res_2  : 1;
        u32 A2S0   : 1;
        u32 A2H1   : 1;
        u32 A2H0   : 1;

        u32 res_1  : 1;
        u32 A1S0   : 1;
        u32 A1H1   : 1;
        u32 A1H0   : 1;
        u32 res_0  : 1;
        u32 A0S0   : 1;
        u32 A0H1   : 1;
        u32 A0H0   : 1;
#endif
    };

    u32 full;
};

extern WCR3_type BSC_WCR3;

// ============================================================
// MCR – Memory Control Register (32-bit)
// SDRAM / DRAM timing and configuration.
// ============================================================
union MCR_type
{
    struct
    {
#if HOST_ENDIAN == ENDIAN_LITTLE
        u32 EDO_MODE : 1;
        u32 RMODE    : 1;
        u32 RFSH     : 1;
        u32 AMX0     : 1;
        u32 AMX1     : 1;
        u32 AMX2     : 1;
        u32 AMXEXT   : 1;
        u32 SZ0      : 1;

        u32 SZ1      : 1;
        u32 BE       : 1;
        u32 TRAS0    : 1;
        u32 TRAS1    : 1;
        u32 TRAS2    : 1;
        u32 TRWL0    : 1;
        u32 TRWL1    : 1;
        u32 TRWL2    : 1;

        u32 RCD0     : 1;
        u32 RCD1     : 1;
        u32 res_0    : 1;
        u32 TPC0     : 1;
        u32 TPC1     : 1;
        u32 TPC2     : 1;
        u32 res_1    : 1;
        u32 TCAS     : 1;

        u32 res_2    : 1;
        u32 res_3    : 1;
        u32 res_4    : 1;
        u32 TRC0     : 1;
        u32 TRC1     : 1;
        u32 TRC2     : 1;
        u32 MRSET    : 1;
        u32 RASD     : 1;
#else
        u32 RASD     : 1;
        u32 MRSET    : 1;
        u32 TRC2     : 1;
        u32 TRC1     : 1;
        u32 TRC0     : 1;
        u32 res_4    : 1;
        u32 res_3    : 1;
        u32 res_2    : 1;

        u32 TCAS     : 1;
        u32 res_1    : 1;
        u32 TPC2     : 1;
        u32 TPC1     : 1;
        u32 TPC0     : 1;
        u32 res_0    : 1;
        u32 RCD1     : 1;
        u32 RCD0     : 1;

        u32 TRWL2    : 1;
        u32 TRWL1    : 1;
        u32 TRWL0    : 1;
        u32 TRAS2    : 1;
        u32 TRAS1    : 1;
        u32 TRAS0    : 1;
        u32 BE       : 1;
        u32 SZ1      : 1;

        u32 SZ0      : 1;
        u32 AMXEXT   : 1;
        u32 AMX2     : 1;
        u32 AMX1     : 1;
        u32 AMX0     : 1;
        u32 RFSH     : 1;
        u32 RMODE    : 1;
        u32 EDO_MODE : 1;
#endif
    };

    u32 full;
};

extern MCR_type BSC_MCR;

// ============================================================
// PCR – PCMCIA Control Register (16-bit)
// ============================================================
union PCR_type
{
    struct
    {
#if HOST_ENDIAN == ENDIAN_LITTLE
        u16 A6TEH0 : 1;
        u16 A6TEH1 : 1;
        u16 A6TEH2 : 1;
        u16 A5TEH0 : 1;
        u16 A5TEH1 : 1;
        u16 A5TEH2 : 1;
        u16 A6TED0 : 1;
        u16 A6TED1 : 1;

        u16 A6TED2 : 1;
        u16 A5TED0 : 1;
        u16 A5TED1 : 1;
        u16 A5TED2 : 1;
        u16 A6PCW0 : 1;
        u16 A6PCW1 : 1;
        u16 A5PCW0 : 1;
        u16 A5PCW1 : 1;
#else
        u16 A5PCW1 : 1;
        u16 A5PCW0 : 1;
        u16 A6PCW1 : 1;
        u16 A6PCW0 : 1;
        u16 A5TED2 : 1;
        u16 A5TED1 : 1;
        u16 A5TED0 : 1;
        u16 A6TED2 : 1;

        u16 A6TED1 : 1;
        u16 A6TED0 : 1;
        u16 A5TEH2 : 1;
        u16 A5TEH1 : 1;
        u16 A5TEH0 : 1;
        u16 A6TEH2 : 1;
        u16 A6TEH1 : 1;
        u16 A6TEH0 : 1;
#endif
    };

    u16 full;
};

extern PCR_type BSC_PCR;

// ============================================================
// RTCSR – Refresh Timer Control/Status Register (16-bit)
// (Manual erroneously calls it RTSCR in some revisions.)
// ============================================================
union RTCSR_type
{
    struct
    {
#if HOST_ENDIAN == ENDIAN_LITTLE
        u16 LMTS  : 1;
        u16 OVIE  : 1;
        u16 OVF   : 1;
        u16 CKS0  : 1;
        u16 CKS1  : 1;
        u16 CKS2  : 1;
        u16 CMIE  : 1;
        u16 CMF   : 1;

        u16 res_0 : 1;
        u16 res_1 : 1;
        u16 res_2 : 1;
        u16 res_3 : 1;
        u16 res_4 : 1;
        u16 res_5 : 1;
        u16 res_6 : 1;
        u16 res_7 : 1;
#else
        u16 res_7 : 1;
        u16 res_6 : 1;
        u16 res_5 : 1;
        u16 res_4 : 1;
        u16 res_3 : 1;
        u16 res_2 : 1;
        u16 res_1 : 1;
        u16 res_0 : 1;

        u16 CMF   : 1;
        u16 CMIE  : 1;
        u16 CKS2  : 1;
        u16 CKS1  : 1;
        u16 CKS0  : 1;
        u16 OVF   : 1;
        u16 OVIE  : 1;
        u16 LMTS  : 1;
#endif
    };

    u16 full;
};

extern RTCSR_type BSC_RTCSR;

// ============================================================
// RTCNT – Refresh Timer Counter (16-bit, 8-bit value field)
// ============================================================
union RTCNT_type
{
    struct
    {
#if HOST_ENDIAN == ENDIAN_LITTLE
        u16 VALUE : 8;
        u16 res_0 : 1;
        u16 res_1 : 1;
        u16 res_2 : 1;
        u16 res_3 : 1;
        u16 res_4 : 1;
        u16 res_5 : 1;
        u16 res_6 : 1;
        u16 res_7 : 1;
#else
        u16 res_7 : 1;
        u16 res_6 : 1;
        u16 res_5 : 1;
        u16 res_4 : 1;
        u16 res_3 : 1;
        u16 res_2 : 1;
        u16 res_1 : 1;
        u16 res_0 : 1;
        u16 VALUE : 8;
#endif
    };

    u16 full;
};

extern RTCNT_type BSC_RTCNT;

// ============================================================
// RTCOR – Refresh Timer Constant Register (16-bit, 8-bit value)
// ============================================================
union RTCOR_type
{
    struct
    {
#if HOST_ENDIAN == ENDIAN_LITTLE
        u16 VALUE : 8;
        u16 res_0 : 1;
        u16 res_1 : 1;
        u16 res_2 : 1;
        u16 res_3 : 1;
        u16 res_4 : 1;
        u16 res_5 : 1;
        u16 res_6 : 1;
        u16 res_7 : 1;
#else
        u16 res_7 : 1;
        u16 res_6 : 1;
        u16 res_5 : 1;
        u16 res_4 : 1;
        u16 res_3 : 1;
        u16 res_2 : 1;
        u16 res_1 : 1;
        u16 res_0 : 1;
        u16 VALUE : 8;
#endif
    };

    u16 full;
};

extern RTCOR_type BSC_RTCOR;

// ============================================================
// RFCR – Refresh Count Register (16-bit, 10-bit value)
// ============================================================
union RFCR_type
{
    struct
    {
#if HOST_ENDIAN == ENDIAN_LITTLE
        u16 VALUE : 10;
        u16 res_2 : 1;
        u16 res_3 : 1;
        u16 res_4 : 1;
        u16 res_5 : 1;
        u16 res_6 : 1;
        u16 res_7 : 1;
#else
        u16 res_7 : 1;
        u16 res_6 : 1;
        u16 res_5 : 1;
        u16 res_4 : 1;
        u16 res_3 : 1;
        u16 res_2 : 1;
        u16 VALUE : 10;
#endif
    };

    u16 full;
};

extern RFCR_type BSC_RFCR;

// ============================================================
// PCTRA – Port Control Register A (32-bit)
// Each GPIO pin PB0–PB15 has a 2-bit control field:
//   bit [2i]   = IO  (1 = output, 0 = input)
//   bit [2i+1] = PUP (0 = pull-up enabled when input)
// ============================================================
union PCTRA_type
{
    struct
    {
#if HOST_ENDIAN == ENDIAN_LITTLE
        u32 PB0IO   : 1;  u32 PB0PUP  : 1;
        u32 PB1IO   : 1;  u32 PB1PUP  : 1;
        u32 PB2IO   : 1;  u32 PB2PUP  : 1;
        u32 PB3IO   : 1;  u32 PB3PUP  : 1;
        u32 PB4IO   : 1;  u32 PB4PUP  : 1;
        u32 PB5IO   : 1;  u32 PB5PUP  : 1;
        u32 PB6IO   : 1;  u32 PB6PUP  : 1;
        u32 PB7IO   : 1;  u32 PB7PUP  : 1;
        u32 PB8IO   : 1;  u32 PB8PUP  : 1;
        u32 PB9IO   : 1;  u32 PB9PUP  : 1;
        u32 PB10IO  : 1;  u32 PB10PUP : 1;
        u32 PB11IO  : 1;  u32 PB11PUP : 1;
        u32 PB12IO  : 1;  u32 PB12PUP : 1;
        u32 PB13IO  : 1;  u32 PB13PUP : 1;
        u32 PB14IO  : 1;  u32 PB14PUP : 1;
        u32 PB15IO  : 1;  u32 PB15PUP : 1;
#else
        u32 PB15PUP : 1;  u32 PB15IO  : 1;
        u32 PB14PUP : 1;  u32 PB14IO  : 1;
        u32 PB13PUP : 1;  u32 PB13IO  : 1;
        u32 PB12PUP : 1;  u32 PB12IO  : 1;
        u32 PB11PUP : 1;  u32 PB11IO  : 1;
        u32 PB10PUP : 1;  u32 PB10IO  : 1;
        u32 PB9PUP  : 1;  u32 PB9IO   : 1;
        u32 PB8PUP  : 1;  u32 PB8IO   : 1;
        u32 PB7PUP  : 1;  u32 PB7IO   : 1;
        u32 PB6PUP  : 1;  u32 PB6IO   : 1;
        u32 PB5PUP  : 1;  u32 PB5IO   : 1;
        u32 PB4PUP  : 1;  u32 PB4IO   : 1;
        u32 PB3PUP  : 1;  u32 PB3IO   : 1;
        u32 PB2PUP  : 1;  u32 PB2IO   : 1;
        u32 PB1PUP  : 1;  u32 PB1IO   : 1;
        u32 PB0PUP  : 1;  u32 PB0IO   : 1;
#endif
    };

    u32 full;
};

extern PCTRA_type BSC_PCTRA;

// ============================================================
// PDTRA – Port Data Register A (16-bit)
// One bit per GPIO pin PB0–PB15.
// ============================================================
union PDTRA_type
{
    struct
    {
#if HOST_ENDIAN == ENDIAN_LITTLE
        u16 PB0DT  : 1;
        u16 PB1DT  : 1;
        u16 PB2DT  : 1;
        u16 PB3DT  : 1;
        u16 PB4DT  : 1;
        u16 PB5DT  : 1;
        u16 PB6DT  : 1;
        u16 PB7DT  : 1;

        u16 PB8DT  : 1;
        u16 PB9DT  : 1;
        u16 PB10DT : 1;
        u16 PB11DT : 1;
        u16 PB12DT : 1;
        u16 PB13DT : 1;
        u16 PB14DT : 1;
        u16 PB15DT : 1;
#else
        u16 PB15DT : 1;
        u16 PB14DT : 1;
        u16 PB13DT : 1;
        u16 PB12DT : 1;
        u16 PB11DT : 1;
        u16 PB10DT : 1;
        u16 PB9DT  : 1;
        u16 PB8DT  : 1;

        u16 PB7DT  : 1;
        u16 PB6DT  : 1;
        u16 PB5DT  : 1;
        u16 PB4DT  : 1;
        u16 PB3DT  : 1;
        u16 PB2DT  : 1;
        u16 PB1DT  : 1;
        u16 PB0DT  : 1;
#endif
    };

    u16 full;
};

extern PDTRA_type BSC_PDTRA;

// ============================================================
// PCTRB – Port Control Register B (32-bit)
// Controls GPIO pins PB16–PB19 (bits 0-7); bits 8-31 reserved.
// ============================================================
union PCTRB_type
{
    struct
    {
#if HOST_ENDIAN == ENDIAN_LITTLE
        u32 PB16IO  : 1;  u32 PB16PUP : 1;
        u32 PB17IO  : 1;  u32 PB17PUP : 1;
        u32 PB18IO  : 1;  u32 PB18PUP : 1;
        u32 PB19IO  : 1;  u32 PB19PUP : 1;
        u32 res_0   : 1;
        u32 res_1   : 1;
        u32 res_2   : 1;
        u32 res_3   : 1;
        u32 res_4   : 1;
        u32 res_5   : 1;
        u32 res_6   : 1;
        u32 res_7   : 1;
        u32 res_8   : 1;
        u32 res_9   : 1;
        u32 res_10  : 1;
        u32 res_11  : 1;
        u32 res_12  : 1;
        u32 res_13  : 1;
        u32 res_14  : 1;
        u32 res_15  : 1;
        u32 res_16  : 1;
        u32 res_17  : 1;
        u32 res_18  : 1;
        u32 res_19  : 1;
        u32 res_20  : 1;
        u32 res_21  : 1;
        u32 res_22  : 1;
        u32 res_23  : 1;
#else
        u32 res_23  : 1;
        u32 res_22  : 1;
        u32 res_21  : 1;
        u32 res_20  : 1;
        u32 res_19  : 1;
        u32 res_18  : 1;
        u32 res_17  : 1;
        u32 res_16  : 1;
        u32 res_15  : 1;
        u32 res_14  : 1;
        u32 res_13  : 1;
        u32 res_12  : 1;
        u32 res_11  : 1;
        u32 res_10  : 1;
        u32 res_9   : 1;
        u32 res_8   : 1;
        u32 res_7   : 1;
        u32 res_6   : 1;
        u32 res_5   : 1;
        u32 res_4   : 1;
        u32 res_3   : 1;
        u32 res_2   : 1;
        u32 res_1   : 1;
        u32 res_0   : 1;
        u32 PB19PUP : 1;  u32 PB19IO  : 1;
        u32 PB18PUP : 1;  u32 PB18IO  : 1;
        u32 PB17PUP : 1;  u32 PB17IO  : 1;
        u32 PB16PUP : 1;  u32 PB16IO  : 1;
#endif
    };

    u32 full;
};

extern PCTRB_type BSC_PCTRB;

// ============================================================
// PDTRB – Port Data Register B (16-bit)
// Data bits for GPIO pins PB16–PB19; bits 4-15 reserved.
// ============================================================
union PDTRB_type
{
    struct
    {
#if HOST_ENDIAN == ENDIAN_LITTLE
        u16 PB16DT : 1;
        u16 PB17DT : 1;
        u16 PB18DT : 1;
        u16 PB19DT : 1;
        u16 res_0  : 1;
        u16 res_1  : 1;
        u16 res_2  : 1;
        u16 res_3  : 1;
        u16 res_4  : 1;
        u16 res_5  : 1;
        u16 res_6  : 1;
        u16 res_7  : 1;
        u16 res_8  : 1;
        u16 res_9  : 1;
        u16 res_10 : 1;
        u16 res_11 : 1;
#else
        u16 res_11 : 1;
        u16 res_10 : 1;
        u16 res_9  : 1;
        u16 res_8  : 1;
        u16 res_7  : 1;
        u16 res_6  : 1;
        u16 res_5  : 1;
        u16 res_4  : 1;
        u16 res_3  : 1;
        u16 res_2  : 1;
        u16 res_1  : 1;
        u16 res_0  : 1;
        u16 PB19DT : 1;
        u16 PB18DT : 1;
        u16 PB17DT : 1;
        u16 PB16DT : 1;
#endif
    };

    u16 full;
};

extern PDTRB_type BSC_PDTRB;

// ============================================================
// GPIOIC – GPIO Interrupt Control Register (16-bit)
// One enable bit per pin PB0–PB15.
// ============================================================
union GPIOIC_type
{
    struct
    {
#if HOST_ENDIAN == ENDIAN_LITTLE
        u16 PTIREN0  : 1;
        u16 PTIREN1  : 1;
        u16 PTIREN2  : 1;
        u16 PTIREN3  : 1;
        u16 PTIREN4  : 1;
        u16 PTIREN5  : 1;
        u16 PTIREN6  : 1;
        u16 PTIREN7  : 1;

        u16 PTIREN8  : 1;
        u16 PTIREN9  : 1;
        u16 PTIREN10 : 1;
        u16 PTIREN11 : 1;
        u16 PTIREN12 : 1;
        u16 PTIREN13 : 1;
        u16 PTIREN14 : 1;
        u16 PTIREN15 : 1;
#else
        u16 PTIREN15 : 1;
        u16 PTIREN14 : 1;
        u16 PTIREN13 : 1;
        u16 PTIREN12 : 1;
        u16 PTIREN11 : 1;
        u16 PTIREN10 : 1;
        u16 PTIREN9  : 1;
        u16 PTIREN8  : 1;

        u16 PTIREN7  : 1;
        u16 PTIREN6  : 1;
        u16 PTIREN5  : 1;
        u16 PTIREN4  : 1;
        u16 PTIREN3  : 1;
        u16 PTIREN2  : 1;
        u16 PTIREN1  : 1;
        u16 PTIREN0  : 1;
#endif
    };

    u16 full;
};

extern GPIOIC_type BSC_GPIOIC;