#pragma once
#include "types.h"

// Size and address mask for the SH4 on-chip scratchpad RAM (8KB)
#define OnChipRAM_SIZE  (0x2000)
#define OnChipRAM_MASK  (OnChipRAM_SIZE - 1)

// Store queue buffer: 64 bytes, 256-byte aligned (hardware requirement)
extern ALIGN(256) u8 sq_both[64];

// SH4 internal peripheral register arrays.
// Each entry is a RegisterStruct covering one 32-bit aligned slot.
extern Array<RegisterStruct> CCN;   // Cache/MMU control         : 14 registers
extern Array<RegisterStruct> UBC;   // User breakpoint controller :  9 registers
extern Array<RegisterStruct> BSC;   // Bus state controller       : 18 registers
extern Array<RegisterStruct> DMAC;  // DMA controller             : 17 registers
extern Array<RegisterStruct> CPG;   // Clock/power generator      :  5 registers
extern Array<RegisterStruct> RTC;   // Real-time clock            : 16 registers
extern Array<RegisterStruct> INTC;  // Interrupt controller       :  4 registers
extern Array<RegisterStruct> TMU;   // Timer unit                 : 12 registers
extern Array<RegisterStruct> SCI;   // Serial communication (SCI) :  8 registers
extern Array<RegisterStruct> SCIF;  // Serial comm. FIFO (SCIF)   : 10 registers

// Lifecycle
void sh4_internal_reg_Init();
void sh4_internal_reg_Reset(bool Manual);
void sh4_internal_reg_Term();

// Hash function: extracts bits [23:16] of an Area7 address for fast switch dispatch.
// All SH4 peripheral modules have unique values in this bit range.
#define A7_REG_HASH(addr) (((addr) >> 16) & 0x00FF)

// ---------------------------------------------------------------------------
// CCN — Cache/MMU control registers
// ---------------------------------------------------------------------------
#define CCN_BASE_addr    0x1F000000
#define CCN_PTEH_addr    0x1F000000  // Page table entry high (32-bit)
#define CCN_PTEL_addr    0x1F000004  // Page table entry low  (32-bit)
#define CCN_TTB_addr     0x1F000008  // Translation table base (32-bit)
#define CCN_TEA_addr     0x1F00000C  // TLB exception address  (32-bit, held on reset)
#define CCN_MMUCR_addr   0x1F000010  // MMU control register   (32-bit, reset=0)
#define CCN_BASRA_addr   0x1F000014  // Break address A        (8-bit)
#define CCN_BASRB_addr   0x1F000018  // Break address B        (8-bit)
#define CCN_CCR_addr     0x1F00001C  // Cache control register (32-bit, reset=0)
#define CCN_TRA_addr     0x1F000020  // TRAPA exception register (32-bit)
#define CCN_EXPEVT_addr  0x1F000024  // Exception event        (32-bit, reset=0x000, manrst=0x020)
#define CCN_INTEVT_addr  0x1F000028  // Interrupt event        (32-bit)
#define CCN_PTEA_addr    0x1F000034  // Page table entry asid  (32-bit)
#define CCN_QACR0_addr   0x1F000038  // Queue address ctrl 0   (32-bit)
#define CCN_QACR1_addr   0x1F00003C  // Queue address ctrl 1   (32-bit)

// ---------------------------------------------------------------------------
// UBC — User breakpoint controller registers
// ---------------------------------------------------------------------------
#define UBC_BASE_addr    0x1F200000
#define UBC_BARA_addr    0x1F200000  // Break address A        (32-bit)
#define UBC_BAMRA_addr   0x1F200004  // Break addr mask A      (8-bit)
#define UBC_BBRA_addr    0x1F200008  // Break bus cycle A      (16-bit, reset=0)
#define UBC_BARB_addr    0x1F20000C  // Break address B        (32-bit)
#define UBC_BAMRB_addr   0x1F200010  // Break addr mask B      (8-bit)
#define UBC_BBRB_addr    0x1F200014  // Break bus cycle B      (16-bit, reset=0)
#define UBC_BDRB_addr    0x1F200018  // Break data B           (32-bit)
#define UBC_BDMRB_addr   0x1F20001C  // Break data mask B      (32-bit)
#define UBC_BRCR_addr    0x1F200020  // Break control          (16-bit, reset=0)

// ---------------------------------------------------------------------------
// BSC — Bus state controller registers
// ---------------------------------------------------------------------------
#define BSC_BASE_addr    0x1F800000
#define BSC_BCR1_addr    0x1F800000  // Bus control 1          (32-bit, reset=0)
#define BSC_BCR2_addr    0x1F800004  // Bus control 2          (16-bit, reset=0x3FFC)
#define BSC_WCR1_addr    0x1F800008  // Wait control 1         (32-bit, reset=0x77777777)
#define BSC_WCR2_addr    0x1F80000C  // Wait control 2         (32-bit, reset=0xFFFEEFFF)
#define BSC_WCR3_addr    0x1F800010  // Wait control 3         (32-bit, reset=0x07777777)
#define BSC_MCR_addr     0x1F800014  // Memory control         (32-bit, reset=0)
#define BSC_PCR_addr     0x1F800018  // PCMCIA control         (16-bit, reset=0)
#define BSC_RTCSR_addr   0x1F80001C  // Refresh timer control  (16-bit, reset=0)
#define BSC_RTCNT_addr   0x1F800020  // Refresh timer counter  (16-bit, reset=0)
#define BSC_RTCOR_addr   0x1F800024  // Refresh timer compare  (16-bit, reset=0)
#define BSC_RFCR_addr    0x1F800028  // Refresh count          (16-bit, reset=0)
#define BSC_PCTRA_addr   0x1F80002C  // Port control A         (32-bit, reset=0)
#define BSC_PDTRA_addr   0x1F800030  // Port data A            (16-bit, undefined reset)
#define BSC_PCTRB_addr   0x1F800040  // Port control B         (32-bit, reset=0)
#define BSC_PDTRB_addr   0x1F800044  // Port data B            (16-bit, undefined reset)
#define BSC_GPIOIC_addr  0x1F800048  // GPIO interrupt control (16-bit, reset=0)
#define BSC_SDMR2_addr   0x1F900000  // DRAM mode register 2   (8-bit, write-only)
#define BSC_SDMR3_addr   0x1F940000  // DRAM mode register 3   (8-bit, write-only)

// ---------------------------------------------------------------------------
// DMAC — DMA controller registers (4 channels, 4 regs each + DMAOR)
// ---------------------------------------------------------------------------
#define DMAC_BASE_addr    0x1FA00000
#define DMAC_SAR0_addr    0x1FA00000  // Source address ch.0
#define DMAC_DAR0_addr    0x1FA00004  // Destination address ch.0
#define DMAC_DMATCR0_addr 0x1FA00008  // Transfer count ch.0
#define DMAC_CHCR0_addr   0x1FA0000C  // Channel control ch.0   (reset=0)
#define DMAC_SAR1_addr    0x1FA00010
#define DMAC_DAR1_addr    0x1FA00014
#define DMAC_DMATCR1_addr 0x1FA00018
#define DMAC_CHCR1_addr   0x1FA0001C  // (reset=0)
#define DMAC_SAR2_addr    0x1FA00020
#define DMAC_DAR2_addr    0x1FA00024
#define DMAC_DMATCR2_addr 0x1FA00028
#define DMAC_CHCR2_addr   0x1FA0002C  // (reset=0)
#define DMAC_SAR3_addr    0x1FA00030
#define DMAC_DAR3_addr    0x1FA00034
#define DMAC_DMATCR3_addr 0x1FA00038
#define DMAC_CHCR3_addr   0x1FA0003C  // (reset=0)
#define DMAC_DMAOR_addr   0x1FA00040  // DMA operation register  (reset=0)

// ---------------------------------------------------------------------------
// CPG — Clock/power generator registers
// ---------------------------------------------------------------------------
#define CPG_BASE_addr    0x1FC00000
#define CPG_FRQCR_addr   0x1FC00000  // Frequency control       (16-bit)
#define CPG_STBCR_addr   0x1FC00004  // Standby control         (8-bit,  reset=0)
#define CPG_WTCNT_addr   0x1FC00008  // Watchdog timer count    (8/16-bit, reset=0)
#define CPG_WTCSR_addr   0x1FC0000C  // Watchdog timer control  (8/16-bit, reset=0)
#define CPG_STBCR2_addr  0x1FC00010  // Standby control 2       (8-bit,  reset=0)

// ---------------------------------------------------------------------------
// RTC — Real-time clock registers
// ---------------------------------------------------------------------------
#define RTC_BASE_addr    0x1FC80000
#define RTC_R64CNT_addr  0x1FC80000  // 64Hz counter            (8-bit)
#define RTC_RSECCNT_addr 0x1FC80004  // Second counter          (8-bit, BCD)
#define RTC_RMINCNT_addr 0x1FC80008  // Minute counter          (8-bit, BCD)
#define RTC_RHRCNT_addr  0x1FC8000C  // Hour counter            (8-bit, BCD)
#define RTC_RWKCNT_addr  0x1FC80010  // Day-of-week counter     (8-bit)
#define RTC_RDAYCNT_addr 0x1FC80014  // Day counter             (8-bit, BCD)
#define RTC_RMONCNT_addr 0x1FC80018  // Month counter           (8-bit, BCD)
#define RTC_RYRCNT_addr  0x1FC8001C  // Year counter            (16-bit, BCD)
#define RTC_RSECAR_addr  0x1FC80020  // Second alarm            (8-bit)
#define RTC_RMINAR_addr  0x1FC80024  // Minute alarm            (8-bit)
#define RTC_RHRAR_addr   0x1FC80028  // Hour alarm              (8-bit)
#define RTC_RWKAR_addr   0x1FC8002C  // Day-of-week alarm       (8-bit)
#define RTC_RDAYAR_addr  0x1FC80030  // Day alarm               (8-bit)
#define RTC_RMONAR_addr  0x1FC80034  // Month alarm             (8-bit)
#define RTC_RCR1_addr    0x1FC80038  // RTC control 1           (8-bit, reset=0)
#define RTC_RCR2_addr    0x1FC8003C  // RTC control 2           (8-bit, reset=0x09)

// ---------------------------------------------------------------------------
// INTC — Interrupt controller registers
// ---------------------------------------------------------------------------
#define INTC_BASE_addr   0x1FD00000
#define INTC_ICR_addr    0x1FD00000  // Interrupt control       (16-bit, reset=0)
#define INTC_IPRA_addr   0x1FD00004  // Interrupt priority A    (16-bit, reset=0)
#define INTC_IPRB_addr   0x1FD00008  // Interrupt priority B    (16-bit, reset=0)
#define INTC_IPRC_addr   0x1FD0000C  // Interrupt priority C    (16-bit, reset=0)

// ---------------------------------------------------------------------------
// TMU — Timer unit registers (3 timers)
// ---------------------------------------------------------------------------
#define TMU_BASE_addr    0x1FD80000
#define TMU_TOCR_addr    0x1FD80000  // Timer output control    (8-bit,  reset=0)
#define TMU_TSTR_addr    0x1FD80004  // Timer start             (8-bit,  reset=0)
#define TMU_TCOR0_addr   0x1FD80008  // Timer constant 0        (32-bit, reset=0xFFFFFFFF)
#define TMU_TCNT0_addr   0x1FD8000C  // Timer counter 0         (32-bit, reset=0xFFFFFFFF)
#define TMU_TCR0_addr    0x1FD80010  // Timer control 0         (16-bit, reset=0)
#define TMU_TCOR1_addr   0x1FD80014  // Timer constant 1
#define TMU_TCNT1_addr   0x1FD80018  // Timer counter 1
#define TMU_TCR1_addr    0x1FD8001C  // Timer control 1
#define TMU_TCOR2_addr   0x1FD80020  // Timer constant 2
#define TMU_TCNT2_addr   0x1FD80024  // Timer counter 2
#define TMU_TCR2_addr    0x1FD80028  // Timer control 2
#define TMU_TCPR2_addr   0x1FD8002C  // Input capture register  (32-bit)

// ---------------------------------------------------------------------------
// SCI — Serial communication interface (non-FIFO) registers
// ---------------------------------------------------------------------------
#define SCI_BASE_addr     0x1FE00000
#define SCI_SCSMR1_addr   0x1FE00000  // Serial mode             (8-bit, reset=0)
#define SCI_SCBRR1_addr   0x1FE00004  // Bit rate                (8-bit, reset=0xFF)
#define SCI_SCSCR1_addr   0x1FE00008  // Serial control          (8-bit, reset=0)
#define SCI_SCTDR1_addr   0x1FE0000C  // Transmit data           (8-bit, reset=0xFF)
#define SCI_SCSSR1_addr   0x1FE00010  // Serial status           (8-bit, reset=0x84)
#define SCI_SCRDR1_addr   0x1FE00014  // Receive data            (8-bit, reset=0)
#define SCI_SCSCMR1_addr  0x1FE00018  // Smart card mode         (8-bit, reset=0)
#define SCI_SCSPTR1_addr  0x1FE0001C  // Serial port             (8-bit, reset=0)

// ---------------------------------------------------------------------------
// SCIF — Serial communication interface with FIFO registers
// ---------------------------------------------------------------------------
#define SCIF_BASE_addr     0x1FE80000
#define SCIF_SCSMR2_addr   0x1FE80000  // Serial mode             (16-bit, reset=0)
#define SCIF_SCBRR2_addr   0x1FE80004  // Bit rate                (8-bit,  reset=0xFF)
#define SCIF_SCSCR2_addr   0x1FE80008  // Serial control          (16-bit, reset=0)
#define SCIF_SCFTDR2_addr  0x1FE8000C  // FIFO transmit data      (8-bit)
#define SCIF_SCFSR2_addr   0x1FE80010  // FIFO status             (16-bit, reset=0x0060)
#define SCIF_SCFRDR2_addr  0x1FE80014  // FIFO receive data       (8-bit)
#define SCIF_SCFCR2_addr   0x1FE80018  // FIFO control            (16-bit, reset=0)
#define SCIF_SCFDR2_addr   0x1FE8001C  // FIFO data count         (16-bit, reset=0)
#define SCIF_SCSPTR2_addr  0x1FE80020  // Serial port             (16-bit, reset=0)
#define SCIF_SCLSR2_addr   0x1FE80024  // Line status             (16-bit, reset=0)

// ---------------------------------------------------------------------------
// UDI — User debug interface (not present on Dreamcast hardware)
// ---------------------------------------------------------------------------
#define UDI_BASE_addr    0x1FF00000
#define UDI_SDIR_addr    0x1FF00000  // Scan data in/reset      (16-bit, reset=0xFFFF)
#define UDI_SDDR_addr    0x1FF00008  // Scan data               (32-bit)

// ---------------------------------------------------------------------------
// Virtual memory mapping helpers
// ---------------------------------------------------------------------------
void map_area7_init();
void map_area7(u32 base);
void map_p4();
