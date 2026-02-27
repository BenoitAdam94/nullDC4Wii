/*
	SCIF (Serial Communication Interface with FIFO)
	Used for serial I/O on the Dreamcast, externally available via the serial cable.

	In the emulator, serial output is discarded (or optionally printed to stdout
	in debug builds). Reads always report no pending data.
*/

#include "types.h"
#include "dc/mem/sh4_internal_reg.h"
#include "scif.h"

//SCIF SCSMR2 0xFFE80000 0x1FE80000 16 0x0000 0x0000 Held Held Pclk
SCSMR2_type SCIF_SCSMR2;

//SCIF SCBRR2 0xFFE80004 0x1FE80004 8 0xFF 0xFF Held Held Pclk
u8 SCIF_SCBRR2;

//SCIF SCSCR2 0xFFE80008 0x1FE80008 16 0x0000 0x0000 Held Held Pclk
SCSCR2_type SCIF_SCSCR2;

//SCIF SCFTDR2 0xFFE8000C 0x1FE8000C 8 Undefined Undefined Held Held Pclk
u8 SCIF_SCFTDR2;

//SCIF SCFSR2 0xFFE80010 0x1FE80010 16 0x0060 0x0060 Held Held Pclk
// BUG FIX: was incorrectly typed as SCSCR2_type -- must be SCFSR2_type
SCFSR2_type SCIF_SCFSR2;

//SCIF SCFRDR2 0xFFE80014 0x1FE80014 8 Undefined Undefined Held Held Pclk
// Read ONLY
u8 SCIF_SCFRDR2;

//SCIF SCFCR2 0xFFE80018 0x1FE80018 16 0x0000 0x0000 Held Held Pclk
SCFCR2_type SCIF_SCFCR2;

// Read ONLY
//SCIF SCFDR2 0xFFE8001C 0x1FE8001C 16 0x0000 0x0000 Held Held Pclk
SCFDR2_type SCIF_SCFDR2;

//SCIF SCSPTR2 0xFFE80020 0x1FE80020 16 0x0000 0x0000 Held Held Pclk
SCSPTR2_type SCIF_SCSPTR2;

//SCIF SCLSR2 0xFFE80024 0x1FE80024 16 0x0000 0x0000 Held Held Pclk
SCLSR2_type SCIF_SCLSR2;


// Write to the DC's serial transmit register.
// Define SCIF_DEBUG_OUTPUT to forward characters to the host console --
// very handy for DC homebrew/BIOS debug prints.
static void SerialWrite(u32 data)
{
#ifdef SCIF_DEBUG_OUTPUT
	putchar((int)(data & 0xFF));
#endif
	// Serial TX is otherwise intentionally ignored.
}

// SCIF_SCFSR2 read -- report TDFE and TEND set (TX FIFO empty, TX complete),
// all RX/error flags clear. This matches the hardware idle state.
static u32 ReadSerialStatus()
{
	return 0x60; // TDFE | TEND
}

// SCIF_SCFDR2 read -- both TX and RX FIFOs are empty
static u32 Read_SCFDR2()
{
	return 0x0000;
}

// SCIF_SCFRDR2 read -- no received data pending
static u32 ReadSerialData()
{
	return 0x00;
}


void scif_Init()
{
	// SCSMR2 - Serial mode register (R/W, 16-bit)
	SCIF[(SCIF_SCSMR2_addr & 0xFF) >> 2].flags         = REG_16BIT_READWRITE | REG_READ_DATA | REG_WRITE_DATA;
	SCIF[(SCIF_SCSMR2_addr & 0xFF) >> 2].readFunction  = 0;
	SCIF[(SCIF_SCSMR2_addr & 0xFF) >> 2].writeFunction = 0;
	SCIF[(SCIF_SCSMR2_addr & 0xFF) >> 2].data16        = &SCIF_SCSMR2.full;

	// SCBRR2 - Bit rate register (R/W, 8-bit)
	SCIF[(SCIF_SCBRR2_addr & 0xFF) >> 2].flags         = REG_8BIT_READWRITE | REG_READ_DATA | REG_WRITE_DATA;
	SCIF[(SCIF_SCBRR2_addr & 0xFF) >> 2].readFunction  = 0;
	SCIF[(SCIF_SCBRR2_addr & 0xFF) >> 2].writeFunction = 0;
	SCIF[(SCIF_SCBRR2_addr & 0xFF) >> 2].data8         = &SCIF_SCBRR2;

	// SCSCR2 - Serial control register (R/W, 16-bit)
	SCIF[(SCIF_SCSCR2_addr & 0xFF) >> 2].flags         = REG_16BIT_READWRITE | REG_READ_DATA | REG_WRITE_DATA;
	SCIF[(SCIF_SCSCR2_addr & 0xFF) >> 2].readFunction  = 0;
	SCIF[(SCIF_SCSCR2_addr & 0xFF) >> 2].writeFunction = 0;
	SCIF[(SCIF_SCSCR2_addr & 0xFF) >> 2].data16        = &SCIF_SCSCR2.full;

	// SCFTDR2 - Serial transmit FIFO data register (Write ONLY, 8-bit)
	SCIF[(SCIF_SCFTDR2_addr & 0xFF) >> 2].flags         = REG_8BIT_READWRITE;
	SCIF[(SCIF_SCFTDR2_addr & 0xFF) >> 2].readFunction  = 0;
	SCIF[(SCIF_SCFTDR2_addr & 0xFF) >> 2].writeFunction = SerialWrite;
	SCIF[(SCIF_SCFTDR2_addr & 0xFF) >> 2].data8         = &SCIF_SCFTDR2;

	// SCFSR2 - Serial FIFO status register (R/W, 16-bit; reads via callback)
	SCIF[(SCIF_SCFSR2_addr & 0xFF) >> 2].flags         = REG_16BIT_READWRITE | REG_WRITE_DATA;
	SCIF[(SCIF_SCFSR2_addr & 0xFF) >> 2].readFunction  = ReadSerialStatus;
	SCIF[(SCIF_SCFSR2_addr & 0xFF) >> 2].writeFunction = 0;
	SCIF[(SCIF_SCFSR2_addr & 0xFF) >> 2].data16        = &SCIF_SCFSR2.full;

	// SCFRDR2 - Serial receive FIFO data register (Read ONLY, 8-bit)
	SCIF[(SCIF_SCFRDR2_addr & 0xFF) >> 2].flags         = REG_8BIT_READWRITE;
	SCIF[(SCIF_SCFRDR2_addr & 0xFF) >> 2].readFunction  = ReadSerialData;
	SCIF[(SCIF_SCFRDR2_addr & 0xFF) >> 2].writeFunction = 0;
	SCIF[(SCIF_SCFRDR2_addr & 0xFF) >> 2].data8         = &SCIF_SCFRDR2;

	// SCFCR2 - Serial FIFO control register (R/W, 16-bit)
	SCIF[(SCIF_SCFCR2_addr & 0xFF) >> 2].flags         = REG_16BIT_READWRITE | REG_READ_DATA | REG_WRITE_DATA;
	SCIF[(SCIF_SCFCR2_addr & 0xFF) >> 2].readFunction  = 0;
	SCIF[(SCIF_SCFCR2_addr & 0xFF) >> 2].writeFunction = 0;
	SCIF[(SCIF_SCFCR2_addr & 0xFF) >> 2].data16        = &SCIF_SCFCR2.full;

	// SCFDR2 - Serial FIFO data count register (Read ONLY, 16-bit)
	SCIF[(SCIF_SCFDR2_addr & 0xFF) >> 2].flags         = REG_16BIT_READWRITE;
	SCIF[(SCIF_SCFDR2_addr & 0xFF) >> 2].readFunction  = Read_SCFDR2;
	SCIF[(SCIF_SCFDR2_addr & 0xFF) >> 2].writeFunction = 0;
	SCIF[(SCIF_SCFDR2_addr & 0xFF) >> 2].data16        = &SCIF_SCFDR2.full;

	// SCSPTR2 - Serial port register (R/W, 16-bit)
	SCIF[(SCIF_SCSPTR2_addr & 0xFF) >> 2].flags         = REG_16BIT_READWRITE | REG_READ_DATA | REG_WRITE_DATA;
	SCIF[(SCIF_SCSPTR2_addr & 0xFF) >> 2].readFunction  = 0;
	SCIF[(SCIF_SCSPTR2_addr & 0xFF) >> 2].writeFunction = 0;
	SCIF[(SCIF_SCSPTR2_addr & 0xFF) >> 2].data16        = &SCIF_SCSPTR2.full;

	// SCLSR2 - Line status register (R/W, 16-bit)
	SCIF[(SCIF_SCLSR2_addr & 0xFF) >> 2].flags         = REG_16BIT_READWRITE | REG_READ_DATA | REG_WRITE_DATA;
	SCIF[(SCIF_SCLSR2_addr & 0xFF) >> 2].readFunction  = 0;
	SCIF[(SCIF_SCLSR2_addr & 0xFF) >> 2].writeFunction = 0;
	SCIF[(SCIF_SCLSR2_addr & 0xFF) >> 2].data16        = &SCIF_SCLSR2.full;
}

void scif_Reset(bool Manual)
{
	/*
	Reset values per SH7091 hardware manual:
	SCSMR2  H'FFE80000  16  H'0000
	SCBRR2  H'FFE80004   8  H'FF
	SCSCR2  H'FFE80008  16  H'0000
	SCFTDR2 H'FFE8000C   8  Undefined
	SCFSR2  H'FFE80010  16  H'0060
	SCFRDR2 H'FFE80014   8  Undefined
	SCFCR2  H'FFE80018  16  H'0000
	SCFDR2  H'FFE8001C  16  H'0000
	SCSPTR2 H'FFE80020  16  H'0000
	SCLSR2  H'FFE80024  16  H'0000
	*/
	SCIF_SCSMR2.full  = 0x0000;
	SCIF_SCBRR2       = 0xFF;    // BUG FIX: was missing from original reset
	SCIF_SCSCR2.full  = 0x0000;  // BUG FIX: was missing from original reset
	SCIF_SCFSR2.full  = 0x0060;  // BUG FIX: reset value is 0x0060, not 0x0000
	SCIF_SCFCR2.full  = 0x0000;
	SCIF_SCFDR2.full  = 0x0000;
	SCIF_SCSPTR2.full = 0x0000;
	SCIF_SCLSR2.full  = 0x0000;
}

void scif_Term()
{
	// Nothing to clean up -- no resources are allocated by scif_Init().
}
