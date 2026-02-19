# BSC Changes & Improvement Notes

## What Was Changed

### bsc.h

| # | What | Where | Why |
|---|------|-------|-----|
| 1 | Bitfield types `u32` → `u16` | `BCR2_type`, `PCR_type`, `RTCSR_type`, `RTCNT_type`, `RTCOR_type`, `RFCR_type`, `PDTRA_type`, `PDTRB_type`, `GPIOIC_type` | These are 16-bit registers. Using `u32` bitfields inside a `u16 full` union is undefined behavior — the compiler may pad or misalign the fields |
| 2 | Field renamed `A1SZ1` → `A0SZ1` | `BCR2_type` | Typo in the original — this is the second size bit for area 0, not area 1 |
| 3 | Added cable type constants | Top of file | `DC_CABLE_VGA`, `DC_CABLE_RGB`, `DC_CABLE_COMPOSITE` — documents the magic values shifted into bits [9:8] of `read_BSC_PDTRA` |
| 4 | Improved comments throughout | All union types | Clarified read-only pins, Dreamcast-specific reset values, the RTCSR/RTSCR manual typo, and pin field encoding |

### bsc.cpp

| # | What | Where | Why |
|---|------|-------|-----|
| 5 | `bsc_Reset` now zeroes GPIO masks | `bsc_Reset()` | `mask_read`, `mask_write`, `mask_pull_up`, `vreg_2` were left stale after a reset, which could cause wrong cable detection behavior on the next boot |
| 6 | GPIO globals made `static` | File scope | They were never declared `extern` in the header, so nothing outside should access them. `static` enforces this and avoids accidental linkage |
| 7 | `read_BSC_PDTRA` rewritten for clarity | `read_BSC_PDTRA()` | **Logic is identical** — same conditions, same return values. Restructured into clean `if/else if` with comments explaining each PCTRA pin configuration physically |
| 8 | `write_BSC_PCTRA` cleaned up | `write_BSC_PCTRA()` | **Logic is identical** — replaced raw bit expressions with named booleans (`is_out`, `pull_up`), removed commented-out dead code |
| 9 | `BSC_REG_INIT_DATA` helper macro | `bsc_Init()` | Eliminates ~40 lines of repetitive boilerplate. The actual flag/pointer values are byte-for-byte the same as before |
| 10 | `Manual` parameter marked unused | `bsc_Reset()` | `/*Manual*/` suppresses compiler warnings without changing the function signature |
| 11 | All dead commented-out code removed | Throughout | Old `printf` debug blocks, the large commented `/**` block in `read_BSC_PDTRA`, and `//u32 port_out_data` |

---

## What Was NOT Changed (same behavior)

- All big-endian bitfield layouts — the `#else` paths for Wii/PowerPC are untouched
- `read_BSC_PDTRA` return values — all four PCTRA/PDTRA conditions produce identical results
- `write_BSC_PCTRA` pin direction logic — mask computation is identical
- All register reset values in `bsc_Reset`
- All register wiring in `bsc_Init` — flags, function pointers, and data pointers are the same

---

## What Could Still Be Improved

### Correctness / Accuracy

- **`read_BSC_PDTRB` is a fatal stub** — it calls `die()` on any read. If a game ever touches PDTRB this will crash. A safer fallback would be to log a warning and return 0 rather than aborting.

- **BCR2 read-only pins not enforced** — `A0SZ0_inp` and `A0SZ1_inp` are documented as hardware input pins (read-only). Currently software can write to them freely. A write handler could mask them out to preserve their values.

- **RFCR should auto-increment** — on real hardware the Refresh Count Register increments each time a refresh cycle occurs. Currently it's a static value that software can read/write freely, which is fine for most games but technically inaccurate.

- **RTCNT should count** — similar to RFCR, the refresh timer counter is supposed to increment at a rate determined by RTCSR clock select bits. Not emulated.

### Code Quality

- **`read_BSC_PDTRA` magic values are not fully documented** — the PCTRA/PDTRA probe sequences come from Chankast reverse engineering, not the official SH4 manual. Worth cross-referencing with other DC emulators (lxdream, reicast) to confirm all cases are covered, especially for RGB cable detection.

- **`bsc_Init` macro could be a function** — the `BSC_REG_INIT_DATA` macro works but an inline helper function would give better type safety and debugger visibility.

- **No serialization (save states)** — none of the BSC register state or GPIO masks (`vreg_2` etc.) appear to be saved/loaded as part of save states. If your emulator supports save states, BSC state should be included.

- **`BSC_PCTRB` / `BSC_PDTRB` have no write handler** — port B pins (PB16–PB19) go entirely unhandled. On Dreamcast these pins are used by the AICA sound chip interface. Low priority since most software doesn't probe them, but worth noting.

- **Magic address arithmetic `(addr & 0xFF) >> 2`** — this index calculation is repeated everywhere and assumes the BSC register block is always at a fixed alignment. A named helper or static assert confirming the layout would make it more robust.
