/*
    Dreamcast RTC (Real-Time Clock) emulation
    SH4 internal RTC mapped at 0xFFC80000

    Registers:
      R64CNT  - 64Hz counter (read-only in HW, incremented by emulator)
      RSECCNT - seconds   (0-59, BCD)
      RMINCNT - minutes   (0-59, BCD)
      RHRCNT  - hours     (0-23, BCD)
      RWKCNT  - weekday   (0-6)
      RDAYCNT - day       (1-31, BCD)
      RMONCNT - month     (1-12, BCD)
      RYRCNT  - year      (BCD, e.g. 0x1999)
      RSECAR  - second alarm
      RMINAR  - minute alarm
      RHRAR   - hour alarm
      RWKAR   - weekday alarm
      RDAYAR  - day alarm
      RMONAR  - month alarm
      RCR1    - control register 1
      RCR2    - control register 2
*/

#include "types.h"
#include "dc/mem/sh4_internal_reg.h"
#include "rtc.h"

#include <ctime>

// ---------------------------------------------------------------------------
// Register storage
// ---------------------------------------------------------------------------
u8  RTC_R64CNT  = 0;
u8  RTC_RSECCNT = 0;
u8  RTC_RMINCNT = 0;
u8  RTC_RHRCNT  = 0;
u8  RTC_RWKCNT  = 0;
u8  RTC_RDAYCNT = 0;
u8  RTC_RMONCNT = 0;
u16 RTC_RYRCNT  = 0;
u8  RTC_RSECAR  = 0;
u8  RTC_RMINAR  = 0;
u8  RTC_RHRAR   = 0;
u8  RTC_RWKAR   = 0;
u8  RTC_RDAYAR  = 0;
u8  RTC_RMONAR  = 0;
u8  RTC_RCR1    = 0;
u8  RTC_RCR2    = 0;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static inline u8 toBCD(int v)
{
    return (u8)(((v / 10) << 4) | (v % 10));
}

// Register setup helpers — avoids copy-paste errors
static void rtc_reg8(u32 addr, u8* data)
{
    u32 idx = (addr & 0xFF) >> 2;
    RTC[idx].flags         = REG_8BIT_READWRITE | REG_READ_DATA | REG_WRITE_DATA;
    RTC[idx].readFunction  = 0;
    RTC[idx].writeFunction = 0;
    RTC[idx].data8         = data;
}

static void rtc_reg16(u32 addr, u16* data)
{
    u32 idx = (addr & 0xFF) >> 2;
    RTC[idx].flags         = REG_16BIT_READWRITE | REG_READ_DATA | REG_WRITE_DATA;
    RTC[idx].readFunction  = 0;
    RTC[idx].writeFunction = 0;
    RTC[idx].data16        = data;
}

// Seed RTC registers from the host system clock so games see a plausible time.
static void rtc_sync_host_time()
{
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    if (!t) return;

    RTC_RSECCNT = toBCD(t->tm_sec);
    RTC_RMINCNT = toBCD(t->tm_min);
    RTC_RHRCNT  = toBCD(t->tm_hour);
    RTC_RWKCNT  = (u8)t->tm_wday;           // 0=Sun … 6=Sat
    RTC_RDAYCNT = toBCD(t->tm_mday);
    RTC_RMONCNT = toBCD(t->tm_mon + 1);     // tm_mon is 0-based
    // Year as 4-digit BCD, e.g. 2024 → 0x2024
    int year    = t->tm_year + 1900;
    RTC_RYRCNT  = (u16)(((year / 1000) << 12) |
                        (((year / 100) % 10) << 8) |
                        (((year / 10)  % 10) << 4) |
                        (year % 10));
}

// ---------------------------------------------------------------------------
// Init / Reset / Term
// ---------------------------------------------------------------------------

void rtc_Init()
{
    rtc_reg8 (RTC_R64CNT_addr,  &RTC_R64CNT);
    rtc_reg8 (RTC_RSECCNT_addr, &RTC_RSECCNT);
    rtc_reg8 (RTC_RMINCNT_addr, &RTC_RMINCNT);
    rtc_reg8 (RTC_RHRCNT_addr,  &RTC_RHRCNT);
    rtc_reg8 (RTC_RWKCNT_addr,  &RTC_RWKCNT);
    rtc_reg8 (RTC_RDAYCNT_addr, &RTC_RDAYCNT);
    rtc_reg8 (RTC_RMONCNT_addr, &RTC_RMONCNT);
    rtc_reg16(RTC_RYRCNT_addr,  &RTC_RYRCNT);
    rtc_reg8 (RTC_RSECAR_addr,  &RTC_RSECAR);
    rtc_reg8 (RTC_RMINAR_addr,  &RTC_RMINAR);
    rtc_reg8 (RTC_RHRAR_addr,   &RTC_RHRAR);
    rtc_reg8 (RTC_RWKAR_addr,   &RTC_RWKAR);
    rtc_reg8 (RTC_RDAYAR_addr,  &RTC_RDAYAR);  // BUG FIX: was RTC_R64CNT_addr
    rtc_reg8 (RTC_RMONAR_addr,  &RTC_RMONAR);
    rtc_reg8 (RTC_RCR1_addr,    &RTC_RCR1);
    rtc_reg8 (RTC_RCR2_addr,    &RTC_RCR2);

    rtc_sync_host_time();
}

void rtc_Reset(bool Manual)
{
    // Per SH4 hardware manual: RCR1 resets to 0x00, RCR2 to 0x09 on power-on.
    // All time registers are "Held" (unchanged) across a manual reset.
    RTC_RCR1 = 0x00;
    RTC_RCR2 = 0x09;
}

void rtc_Term()
{
    // Nothing to clean up — registers are globals.
}

// ---------------------------------------------------------------------------
// Tick — call this at 64 Hz from the emulator main loop
// Increments R64CNT and, when it wraps, advances the time registers.
// ---------------------------------------------------------------------------
void rtc_Tick()
{
    // R64CNT counts 0-63 then wraps (6-bit counter per SH4 spec)
    RTC_R64CNT = (RTC_R64CNT + 1) & 0x3F;
    if (RTC_R64CNT != 0)
        return;

    // One second has elapsed — advance BCD time registers
    int sec  = ((RTC_RSECCNT >> 4) * 10) + (RTC_RSECCNT & 0x0F);
    int min  = ((RTC_RMINCNT >> 4) * 10) + (RTC_RMINCNT & 0x0F);
    int hr   = ((RTC_RHRCNT  >> 4) * 10) + (RTC_RHRCNT  & 0x0F);
    int day  = ((RTC_RDAYCNT >> 4) * 10) + (RTC_RDAYCNT & 0x0F);
    int mon  = ((RTC_RMONCNT >> 4) * 10) + (RTC_RMONCNT & 0x0F);

    if (++sec < 60) { RTC_RSECCNT = toBCD(sec); return; }
    sec = 0; RTC_RSECCNT = 0;

    if (++min < 60) { RTC_RMINCNT = toBCD(min); return; }
    min = 0; RTC_RMINCNT = 0;

    if (++hr < 24)  { RTC_RHRCNT  = toBCD(hr);  return; }
    hr = 0; RTC_RHRCNT = 0;

    // Advance weekday (0-6)
    RTC_RWKCNT = (RTC_RWKCNT + 1) % 7;

    // Days-per-month table (ignores leap years for simplicity)
    static const int days_in_month[] =
        { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int max_day = (mon >= 1 && mon <= 12) ? days_in_month[mon] : 31;

    if (++day <= max_day) { RTC_RDAYCNT = toBCD(day); return; }
    day = 1; RTC_RDAYCNT = toBCD(1);

    if (++mon <= 12) { RTC_RMONCNT = toBCD(mon); return; }
    mon = 1; RTC_RMONCNT = toBCD(1);

    // Increment BCD year
    int y = (int)RTC_RYRCNT;
    int year = ((y >> 12) & 0xF) * 1000 +
               ((y >>  8) & 0xF) * 100  +
               ((y >>  4) & 0xF) * 10   +
               ( y        & 0xF);
    year++;
    RTC_RYRCNT = (u16)(((year / 1000) << 12) |
                       (((year / 100) % 10) << 8) |
                       (((year / 10)  % 10) << 4) |
                       (year % 10));
}
