#include "cdi.h"

extern "C" int get_debug_loop();

#define CDI_V2   0x80000004u
#define CDI_V3   0x80000005u
#define CDI_V35  0x80000006u

struct CdiTrack
{
    u32 FAD;
    u32 file_offset;   // byte offset in file where data sectors begin (after pregap)
    u32 sector_size;
    u32 length;        // data sectors (not counting pregap)
    u8  ctrl;
    u8  mode;
    u8  session;
};

static CdiTrack    cdi_tracks[101];
static u32         cdi_track_count;
static SessionInfo cdi_ses;
static TocInfo     cdi_toc;
static DiscType    cdi_disc_type;
static u8          cdi_sec_buf[2352];
static FILE*       cdi_fp;
static u32         cdi_leadout_fad;

// ─── sector reading ──────────────────────────────────────────────────────────

// GD-ROM high-density area starts at FAD 45150.
// Selfboot CDI discs store game data starting at FAD 150 (single density).
// When the BIOS thinks it's reading a GD-ROM HD area (FAD >= 45150),
// remap those reads to FAD 150+ so IP.BIN and the filesystem are served correctly.
#define CDI_GD_HD_START  45150u
#define CDI_GD_LD_START    150u
#define CDI_GD_REMAP_OFF (CDI_GD_HD_START - CDI_GD_LD_START)  // 45000

static void cdi_ReadOneSector(u8* out, u32 sector, u32 out_size)
{
    // FAD remapping removed: CdRom disc type means BIOS reads FAD 150 directly.

    for (s32 i = (s32)cdi_track_count - 1; i >= 0; i--)
    {
        if (cdi_tracks[i].FAD <= sector)
        {
            u32 byte_off = cdi_tracks[i].file_offset
                         + (sector - cdi_tracks[i].FAD) * cdi_tracks[i].sector_size;
            fseek(cdi_fp, byte_off, SEEK_SET);
            fread(cdi_sec_buf, cdi_tracks[i].sector_size, 1, cdi_fp);

            // ── IP.BIN header injection ───────────────────────────────────────
            // Selfboot CDIs built with BootDreams store the IP.TMPL bootstrap
            // but leave the first 0x300 bytes (disc header metadata) as zeros.
            // The Dreamcast BIOS reads FAD 150 (sector 0 of the data track) and
            // checks for "SEGA SEGAKATANA" at offset 0.  If it finds zeros, it
            // falls back to the BIOS menu.  We patch a valid header here so the
            // bootstrap can proceed.  The header fields below match ChuChu Rocket US.
            if (sector == 150 && cdi_sec_buf[0] == 0 && cdi_sec_buf[1] == 0
                && cdi_tracks[i].ctrl == 0x04)
            {
                // Check whether the whole first 16 bytes are zero (blank template)
                bool blank = true;
                for (int z = 0; z < 16; z++) if (cdi_sec_buf[z]) { blank = false; break; }
                if (blank)
                {
                    printf("[CDI] Injecting IP.BIN header into blank FAD 150 sector\n");
                    // Hardware ID  (0x000)
                    memcpy(cdi_sec_buf + 0x000, "SEGA SEGAKATANA ", 16);
                    // Maker ID     (0x010)
                    memcpy(cdi_sec_buf + 0x010, "SEGA ENTERPRISES", 16);
                    // Device info  (0x020) — selfboot CDIs are standard CDs
                    memcpy(cdi_sec_buf + 0x020, "CD-ROM1/1       ", 16);
                    // Area symbols (0x030)
                    memcpy(cdi_sec_buf + 0x030, "JUE             ", 16);
                    // Peripherals  (0x040)
                    memcpy(cdi_sec_buf + 0x040, "E000F10         ", 16);
                    // Product No   (0x050)
                    memcpy(cdi_sec_buf + 0x050, "T-8101N   ", 10);
                    // Version      (0x05A)
                    memcpy(cdi_sec_buf + 0x05A, "V1.002", 6);
                    // Release date (0x060)
                    memcpy(cdi_sec_buf + 0x060, "20000208  ", 10);
                    // Boot file    (0x070)
                    memcpy(cdi_sec_buf + 0x070, "1ST_READ.BIN    ", 16);
                    // Publisher    (0x080)
                    memcpy(cdi_sec_buf + 0x080, "SEGA ENTERPRISES", 16);
                    // Title        (0x090)
                    memcpy(cdi_sec_buf + 0x090, "ChuChu Rocket!                          ", 40);
                }
            }

            printf("[CDI] cdi_ReadOneSector FAD=%u track%d off=0x%08X sz=%u data: %02X%02X%02X%02X%02X%02X%02X%02X\n",
                   sector, i, byte_off, cdi_tracks[i].sector_size,
                   cdi_sec_buf[0],cdi_sec_buf[1],cdi_sec_buf[2],cdi_sec_buf[3],
                   cdi_sec_buf[4],cdi_sec_buf[5],cdi_sec_buf[6],cdi_sec_buf[7]);
            ConvertSector(cdi_sec_buf, out, cdi_tracks[i].sector_size, out_size, sector);
            return;
        }
    }
    printf("[CDI] WARN: sector FAD=%u not found in any track\n", sector);
    memset(out, 0, out_size);
}

void cdi_DriveReadSector(u8* buff, u32 StartSector, u32 SectorCount, u32 secsz)
{
    printf("[CDI] ReadSector FAD=%u count=%u secsz=%u\n",
           StartSector, SectorCount, secsz);
    while (SectorCount--)
    {
        cdi_ReadOneSector(buff, StartSector, secsz);
        buff += secsz;
        StartSector++;
    }
}

// ─── TOC / session info ──────────────────────────────────────────────────────

void cdi_DriveGetTocInfo(TocInfo* toc, DiskArea area)
{
    memcpy(toc, &cdi_toc, sizeof(TocInfo));

    if (area == DoubleDensity)
    {
        // No high-density area on a standard CD — return empty
        printf("[CDI] GetTocInfo(DoubleDensity) => empty\n");
        memset(toc, 0xFF, sizeof(TocInfo));
        toc->LeadOut.FAD     = cdi_leadout_fad;
        toc->LeadOut.Control = 0x04;
        toc->LeadOut.Addr    = 0;
        toc->LeadOut.Session = 0;
        return;
    }

    // SingleDensity
    printf("[CDI] GetTocInfo(SingleDensity): FistTrack=%u LastTrack=%u LeadOutFAD=%u\n",
           toc->FistTrack, toc->LastTrack, toc->LeadOut.FAD);
    for (u32 i = 0; i < cdi_track_count; i++)
    {
        printf("[CDI]   toc track[%u]: FAD=%u ctrl=0x%02X addr=0x%02X session=%u\n",
               i+1, toc->tracks[i].FAD,
               toc->tracks[i].Control,
               toc->tracks[i].Addr,
               toc->tracks[i].Session);
    }
}

u32 cdi_DriveGetDiscType()
{
    printf("[CDI] GetDiscType => 0x%X (%s)\n",
           (u32)cdi_disc_type,
           cdi_disc_type == GdRom ? "GdRom" : "CdRom");
    return cdi_disc_type;
}

void cdi_GetSessionsInfo(SessionInfo* sessions)
{
    memcpy(sessions, &cdi_ses, sizeof(SessionInfo));
    printf("[CDI] GetSessionsInfo: count=%u endFAD=%u\n",
           cdi_ses.SessionCount, cdi_ses.SessionsEndFAD);
    for (u32 i = 0; i < cdi_ses.SessionCount; i++)
        printf("[CDI]   session[%u]: startTrack=%u FAD=%u\n",
               i, cdi_ses.SessionStart[i], cdi_ses.SessionFAD[i]);
}

// ─── helpers ─────────────────────────────────────────────────────────────────

static u16 read_u16(FILE* f)
{
    u8 b[2] = {0};
    fread(b, 1, 2, f);
    return (u16)b[0] | ((u16)b[1] << 8);
}

static u32 read_u32(FILE* f)
{
    u8 b[4] = {0};
    fread(b, 1, 4, f);
    return (u32)b[0] | ((u32)b[1]<<8) | ((u32)b[2]<<16) | ((u32)b[3]<<24);
}

static void skip(FILE* f, int n) { fseek(f, n, SEEK_CUR); }

// ─── header parser ───────────────────────────────────────────────────────────

/*
 * Observed layout at hdr_pos (from hex dump of chuchurocket.cdi):
 *
 * +0x00  u16  track_count_session0        (= 1)
 * +0x02  u16  disc_type_raw               (= 0x0012)
 * +0x04  u8[8] unknown
 * +0x0C  u32  0xFFFF0000  ─┐ 8-byte separator
 * +0x10  u32  0x0000FFFF  ─┘
 * +0x14  u32  session_count               (= 1)
 * +0x18  u32  0xFFFFFFFF  end marker
 * +0x1C  u32  timestamp
 * +0x20  u8   filename_len
 * +0x21  char filename[filename_len]
 *        u8[11]  pad
 *        u32  pregap       (sectors)
 *        u32  length       (data sectors)
 *        u8[6] unknown
 *        u32  mode         (0=audio, 1=Mode1, 2=Mode2/XA)
 *        u8[12] unknown
 *        u32  lba          (raw LBA 0-based; FAD = lba+150)
 *        u32  total_length (pregap + length)
 *        u8[16] unknown
 *        u32  sector_size  (2048/2336/2352/2448)
 *        u32  extra        (V3.5 only)
 *        u8[12] trailing
 *
 * After all tracks: u32 session_end_marker
 * Tail: u32 disc_type, u32 sessions_end_lba, u32 pad, u32 leadout_lba
 */

static bool cdi_ParseHeader(FILE* f, u32 version, long file_size, u32 hdr_offset)
{
    printf("\n********************************CDI********************************\n");
    printf("[CDI] ParseHeader: version=0x%08X (%s) pos=0x%08lX\n",
           version,
           version==CDI_V2?"V2":version==CDI_V3?"V3":"V3.5",
           ftell(f));

    // ── Session-0 prefix ─────────────────────────────────────────────────────
    u16 first_track_count = read_u16(f);
    u16 disc_type_raw     = read_u16(f);
    skip(f, 8);

    u32 sep_lo = read_u32(f);
    u32 sep_hi = read_u32(f);
    printf("[CDI] sep=0x%08X 0x%08X (expect 0xFFFF0000 / 0x0000FFFF)\n", sep_lo, sep_hi);

    u32 session_count = read_u32(f);
    u32 end_marker    = read_u32(f);
    skip(f, 4); // timestamp

    printf("[CDI] session_count=%u end_marker=0x%08X disc_type_raw=0x%04X first_track_count=%u\n",
           session_count, end_marker, disc_type_raw, first_track_count);

    if (session_count == 0 || session_count > 4)
    {
        printf("[CDI] ERROR: implausible session_count %u\n", session_count);
        printf("********************************END********************************\n\n");
        return false;
    }

    // CDI selfboot discs are always standard CDs
    // Selfboot CDI discs must report GdRom so BIOS uses the GD-ROM boot path.
    // With CdRom (0x10), SecNumber.DiscFormat = 1 and BIOS enters audio-CD mode.
    // With GdRom (0x80), SecNumber.DiscFormat = 8 and BIOS issues SPI_CD_READ FAD 150.
    cdi_disc_type = CdRom;
    printf("[CDI] disc type: CdRom (MIL-CD selfboot path reads FAD 150 directly)\n");

    // Initialize TOC to 0xFF (unused slots per GDI convention)
    memset(&cdi_toc, 0xFF, sizeof(cdi_toc));
    memset(&cdi_ses, 0, sizeof(cdi_ses));

    u32  first_data_ssize = 0;   // sector_size of track 0 (set after first track parsed)
    cdi_track_count       = 0;

    for (u32 ses = 0; ses < session_count; ses++)
    {
        u32 track_count;
        if (ses == 0)
        {
            track_count = first_track_count;
        }
        else
        {
            track_count = read_u16(f);
            skip(f, 2 + 8 + 8 + 4 + 4 + 4);
        }

        printf("[CDI] Session %u: %u tracks\n", ses, track_count);

        if (track_count == 0 || track_count > 99)
        {
            printf("[CDI] ERROR: implausible track_count=%u\n", track_count);
            printf("********************************END********************************\n\n");
            return false;
        }

        for (u32 trk = 0; trk < track_count; trk++)
        {
            u8 fname_len = 0;
            fread(&fname_len, 1, 1, f);
            char fname[256] = {0};
            fread(fname, 1, fname_len < 255 ? fname_len : 254, f);
            if (fname_len >= 255) skip(f, fname_len - 254);
            skip(f, 11);

            u32 pregap      = read_u32(f);
            u32 length      = read_u32(f);
            skip(f, 6);
            u32 mode        = read_u32(f);
            skip(f, 12);
            u32 lba         = read_u32(f);
            u32 total_len   = read_u32(f);
            skip(f, 16);
            u32 sector_size = read_u32(f);  // often garbage in CDI; we override below

            if (version == CDI_V35) skip(f, 4);
            skip(f, 12);

            // Determine track type from mode field.
            // CDI mode: 0=audio, 1=Mode1, 2=Mode2, etc. Values >=2 → data.
            // The ssize field is unreliable in many CDI files; derive from track type.
            u8  ctrl    = (mode == 0) ? 0x00 : 0x04;
            u32 fad     = lba + 150;

            // CDI selfboot discs store sectors as:
            //   Data tracks:  2048 bytes/sector (cooked Mode1, no raw headers)
            //   Audio tracks: 2352 bytes/sector (raw 16-bit stereo 44.1kHz)
            // Override any garbage ssize with the correct derived value.
            if (ctrl == 0x04)
                sector_size = 2048;
            else
                sector_size = 2352;

            // Save the first track's sector_size to convert subsequent LBAs to bytes.
            if (cdi_track_count == 0)
                first_data_ssize = sector_size;

            // CDI file layout: data begins at byte 0.
            // The 'lba' field is the absolute LBA from disc start (track 1 = LBA 0).
            // All sectors before track N are first_data_ssize bytes each, so:
            //   file_start = lba * first_data_ssize
            //
            // Pregap handling:
            //   Track 1 (first track): pregap is NOT physically stored in the file.
            //     The file starts directly at the first data sector (IP.BIN / FAD 150).
            //     → file_offset = file_start (no pregap skip)
            //   Track 2+ (subsequent tracks): pregap silence IS stored in the file
            //     between the preceding track's data and this track's data.
            //     → file_offset = file_start + pregap * sector_size
            long file_start  = (long)lba * first_data_ssize;
            long file_offset = file_start
                             + (cdi_track_count > 0 ? (long)pregap * sector_size : 0);

            printf("[CDI]   Track %u (ses%u trk%u): '%s'\n"
                   "[CDI]     pregap=%u length=%u total=%u mode=%u lba=%u fad=%u\n"
                   "[CDI]     sector_size=%u ctrl=0x%02X file_offset=0x%08lX\n",
                   cdi_track_count+1, ses, trk, fname,
                   pregap, length, total_len, mode, lba, fad,
                   sector_size, ctrl, file_offset);

            // Peek at multiple candidate offsets to locate IP.BIN / PVD
            if (ctrl == 0x04)  // only for data tracks
            {
                long cur = ftell(f);
                // Try several possible sector offsets to find IP.BIN or ISO9660 PVD
                // "SEGA SEGAKATANA" = IP.BIN; "\x01CD001" = ISO9660 PVD (sector 16 of data)
                struct { long off; const char* desc; } probes[] = {
                    { 0,              "cooked_nopregap_s0"   },
                    { 16,             "raw_nopregap_s0+16"   },
                    { 2*2048,         "cooked_pregap2_s0"    },
                    { 2*2352+16,      "raw_pregap2_s0+16"    },
                    { 16*2048,        "cooked_nopregap_pvd"  },
                    { (2+16)*2048,    "cooked_pregap2_pvd"   },
                    { 16*2352+16,     "raw_nopregap_pvd"     },
                    { (2+16)*2352+16, "raw_pregap2_pvd"      },
                    // FAD-150 based offsets
                    { 150L*2048,      "fad150_cooked"        },
                    { 150L*2048+16,   "fad150_raw+16"        },
                    { 150L*2048+16*2048, "fad150_pvd_cooked" },
                    { 150L*2352+16,   "fad150_raw_s0+16"     },
                    // Large offset binary search to find non-zero data
                    { 0x100000L,      "1MB"                  },
                    { 0x2000000L,     "32MB"                 },
                    { 0x8000000L,     "128MB"                },
                };
                for (int pi = 0; pi < 15; pi++) {
                    int sk = fseek(f, probes[pi].off, SEEK_SET);
                    long pos = ftell(f);
                    u8 peek[16] = {0};
                    fread(peek, 1, 16, f);
                    printf("[CDI]     probe[%s @ 0x%lX] seek=%d pos=%ld: ", 
                           probes[pi].desc, probes[pi].off, sk, pos);
                    for (int p = 0; p < 16; p++) printf("%02X", peek[p]);
                    printf(" '");
                    for (int p = 0; p < 16; p++)
                        printf("%c", (peek[p] >= 0x20 && peek[p] < 0x7F) ? peek[p] : '.');
                    printf("'\n");
                }

                // ── IP.BIN signature scan ────────────────────────────────────────
                // Read 64KB chunks and scan for "SEGA SEGAKATANA" signature.
                // Much faster than per-sector seeks on SD card.
                // Reports file offset, inferred FAD for 2048 and 2352 stride.
                static const u8 ipbin_sig[] = {0x53,0x45,0x47,0x41,0x20,0x53,0x45,0x47,0x41,0x4B};
                static u8 scan_chunk[65536];
                bool found_ipbin = false;
                printf("[CDI]     Scanning for IP.BIN signature (SEGA SEGAKATANA)...\n");
                for (long chunk_off = 0; chunk_off < 200L*1024*1024 && !found_ipbin; chunk_off += 65536) {
                    fseek(f, chunk_off, SEEK_SET);
                    int got = (int)fread(scan_chunk, 1, 65536, f);
                    if (got < 10) break;
                    for (int bi = 0; bi <= got - 10 && !found_ipbin; bi++) {
                        if (memcmp(&scan_chunk[bi], ipbin_sig, 10) == 0) {
                            long file_off = chunk_off + bi;
                            u32 fad_2048 = (u32)(150 + file_off / 2048);
                            u32 fad_2352 = (file_off >= 16) ? (u32)(150 + (file_off - 16) / 2352) : 150;
                            printf("[CDI]     *** IP.BIN FOUND @ file_off=0x%lX ***\n", file_off);
                            printf("[CDI]     inferred FAD if 2048-stride: %u (need remap: %u -> 45150)\n",
                                   fad_2048, fad_2048);
                            printf("[CDI]     inferred FAD if 2352-stride: %u\n", fad_2352);
                            printf("[CDI]     bytes: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
                                   scan_chunk[bi],scan_chunk[bi+1],scan_chunk[bi+2],scan_chunk[bi+3],
                                   scan_chunk[bi+4],scan_chunk[bi+5],scan_chunk[bi+6],scan_chunk[bi+7],
                                   scan_chunk[bi+8],scan_chunk[bi+9],scan_chunk[bi+10],scan_chunk[bi+11],
                                   scan_chunk[bi+12],scan_chunk[bi+13],scan_chunk[bi+14],scan_chunk[bi+15]);
                            found_ipbin = true;
                        }
                    }
                }
                if (!found_ipbin)
                    printf("[CDI]     IP.BIN signature not found in first 200MB\n");

                fseek(f, cur, SEEK_SET);
            }

            if (cdi_track_count < 100)
            {
                cdi_tracks[cdi_track_count].FAD         = fad;
                cdi_tracks[cdi_track_count].file_offset = (u32)file_offset;
                cdi_tracks[cdi_track_count].sector_size = sector_size;
                cdi_tracks[cdi_track_count].length      = length;
                cdi_tracks[cdi_track_count].ctrl        = ctrl;
                cdi_tracks[cdi_track_count].mode        = (u8)mode;
                cdi_tracks[cdi_track_count].session     = (u8)ses;

                // ── Zero-preamble fixup for Track 0 ──────────────────────────────
                // Some CDI files (created by older BootDreams or CDI rippers) store
                // a large block of zero sectors at the start of Track 1 before IP.BIN.
                // This happens when the tool blanked the MIL-CD security ring data.
                // We detect this by scanning sector-by-sector for the first non-zero
                // sector, then adjust file_offset to point there.
                // The BIOS will read that sector as FAD 150 (via our FAD remapping).
                if (cdi_track_count == 0 && ctrl == 0x04)
                {
                    long cur2 = ftell(f);
                    static u8 zero_scan_buf[2048];
                    long scan_off = file_offset;
                    long scan_limit = file_offset + 1024L * sector_size; // scan first 1024 sectors
                    bool found_nonzero = false;
                    while (scan_off < scan_limit)
                    {
                        fseek(f, scan_off, SEEK_SET);
                        int got = (int)fread(zero_scan_buf, 1, sector_size, f);
                        if (got < (int)sector_size) break;
                        bool nonzero = false;
                        for (int z = 0; z < (int)sector_size; z++)
                            if (zero_scan_buf[z]) { nonzero = true; break; }
                        if (nonzero)
                        {
                            if (scan_off != file_offset)
                            {
                                printf("[CDI] Zero-preamble fixup: %ld zero sectors detected\n",
                                       (scan_off - file_offset) / sector_size);
                                printf("[CDI] Adjusting Track 0 file_offset: 0x%lX -> 0x%lX\n",
                                       file_offset, scan_off);
                                cdi_tracks[cdi_track_count].file_offset = (u32)scan_off;
                            }
                            else
                            {
                                printf("[CDI] Track 0 starts with non-zero data at offset 0x%lX (no fixup needed)\n",
                                       scan_off);
                            }
                            found_nonzero = true;
                            break;
                        }
                        scan_off += sector_size;
                    }
                    if (!found_nonzero)
                        printf("[CDI] WARN: Track 0 first 1024 sectors are all zero!\n");
                    fseek(f, cur2, SEEK_SET);
                }

                cdi_track_count++;
            }

            // (no data_offset accumulation needed; we use lba directly)
        }

        u32 sess_end = read_u32(f);
        printf("[CDI] Session %u end_marker=0x%08X\n", ses, sess_end);
    }

    // ── Tail ─────────────────────────────────────────────────────────────────
    // ── Tail: try to read leadout LBA, but it may be 0/corrupt in some CDI files ──
    u32 disc_type_tail = read_u32(f);
    u32 sessions_end   = read_u32(f);
    skip(f, 4);
    u32 leadout_lba    = read_u32(f);
    printf("[CDI] Tail (raw): disc_type_tail=0x%08X sessions_end=%u leadout_lba=%u\n",
           disc_type_tail, sessions_end, leadout_lba);

    if (leadout_lba > 150 && leadout_lba < 450000)
    {
        // Tail looks valid: use it directly.
        cdi_leadout_fad = leadout_lba + 150;
        printf("[CDI] Leadout from tail: lba=%u fad=%u\n", leadout_lba, cdi_leadout_fad);
    }
    else if (cdi_track_count > 0)
    {
        // Tail is corrupt (all-zeros is common). Compute from file size and last track.
        // All data occupies file bytes [0 .. file_size - hdr_offset).
        // Last track's data starts at file_start + pregap*ssize and runs to end of data area.
        const CdiTrack& last = cdi_tracks[cdi_track_count - 1];
        long data_area_end = (long)file_size - (long)hdr_offset;
        long last_data_start = last.file_offset;          // already past pregap
        long last_audio_bytes = data_area_end - last_data_start;
        u32  last_sectors = (last.sector_size > 0)
                            ? (u32)(last_audio_bytes / last.sector_size) : 0;
        u32  last_lba = last.FAD - 150;  // convert FAD back to LBA
        cdi_leadout_fad = last.FAD + last_sectors;
        printf("[CDI] Leadout computed: last_lba=%u last_sectors=%u fad=%u\n",
               last_lba, last_sectors, cdi_leadout_fad);
    }
    else
    {
        cdi_leadout_fad = 150;
    }

    // ── Build TOC ─────────────────────────────────────────────────────────────
    // Unused slots already 0xFF from memset above.
    for (u32 i = 0; i < cdi_track_count && i < 99; i++)
    {
        cdi_toc.tracks[i].FAD     = cdi_tracks[i].FAD;
        cdi_toc.tracks[i].Control = cdi_tracks[i].ctrl;
        cdi_toc.tracks[i].Addr    = 0;
        cdi_toc.tracks[i].Session = cdi_tracks[i].session + 1;
    }
    cdi_toc.FistTrack = 1;
    cdi_toc.LastTrack = (u8)cdi_track_count;
    cdi_toc.LeadOut.FAD     = cdi_leadout_fad;
    cdi_toc.LeadOut.Control = 0x04;
    cdi_toc.LeadOut.Addr    = 0;
    cdi_toc.LeadOut.Session = 0;

    // ── Build SessionInfo ─────────────────────────────────────────────────────
    // MIL-CD selfboot: 2 sessions. BIOS uses Session2 FAD to find IP.BIN.
    // SessionStart[n] = first track number in that session (1-based).
    // Both our synthetic sessions point to track 1 at FAD 150 (the data track).
    cdi_ses.SessionCount    = 2;
    cdi_ses.SessionsEndFAD  = cdi_leadout_fad;
    cdi_ses.SessionStart[0] = 1;   // session 1 first track = 1
    cdi_ses.SessionFAD[0]   = cdi_tracks[0].FAD;  // FAD 150
    cdi_ses.SessionStart[1] = 1;   // session 2 first track = 1 (same data track)
    cdi_ses.SessionFAD[1]   = cdi_tracks[0].FAD;  // FAD 150 (BIOS reads IP.BIN here)

    printf("[CDI] TOC: %u tracks FistTrack=%u LastTrack=%u LeadOutFAD=%u\n",
           cdi_track_count, cdi_toc.FistTrack, cdi_toc.LastTrack, cdi_toc.LeadOut.FAD);
    printf("[CDI] Ses: count=%u endFAD=%u start[0]=%u FAD[0]=%u start[1]=%u FAD[1]=%u\n",
           cdi_ses.SessionCount, cdi_ses.SessionsEndFAD,
           cdi_ses.SessionStart[0], cdi_ses.SessionFAD[0],
           cdi_ses.SessionStart[1], cdi_ses.SessionFAD[1]);
    printf("********************************END********************************\n\n");
    return true;
}

// ─── init / term ─────────────────────────────────────────────────────────────

bool cdi_init(char* file)
{
    printf("\n********************************CDI********************************\n");
    printf("[CDI] cdi_init('%s')\n", file);

    FILE* f = fopen(file, "rb");
    if (!f)
    {
        printf("[CDI] ERROR: cannot open '%s'\n", file);
        printf("********************************END********************************\n\n");
        return false;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    printf("[CDI] File size: %ld bytes (%.2f MB)\n",
           file_size, (float)file_size / 1048576.0f);

    fseek(f, -8, SEEK_END);
    u32 version    = read_u32(f);
    u32 hdr_offset = read_u32(f);

    printf("[CDI] version=0x%08X (%s)  hdr_offset=%u\n",
           version,
           version==CDI_V2?"V2":version==CDI_V3?"V3":
           version==CDI_V35?"V3.5":"UNKNOWN",
           hdr_offset);

    if (version != CDI_V2 && version != CDI_V3 && version != CDI_V35)
    {
        printf("[CDI] ERROR: bad version 0x%08X\n", version);
        printf("********************************END********************************\n\n");
        fclose(f); return false;
    }

    long hdr_pos;
    if (version == CDI_V2)
    {
        hdr_pos = 0;
    }
    else
    {
        hdr_pos = file_size - (long)hdr_offset;
        printf("[CDI] hdr_pos = %ld - %u = %ld (0x%08lX)\n",
               file_size, hdr_offset, hdr_pos, hdr_pos);
        if (hdr_pos < 0 || hdr_pos >= file_size)
        {
            printf("[CDI] ERROR: hdr_pos out of bounds\n");
            printf("********************************END********************************\n\n");
            fclose(f); return false;
        }
    }

    printf("********************************END********************************\n\n");

    cdi_fp = f;  // set early so peek in ParseHeader works
    fseek(f, hdr_pos, SEEK_SET);
    if (!cdi_ParseHeader(f, version, file_size, hdr_offset))
    {
        fclose(f);
        cdi_fp = NULL;
        return false;
    }

    return true;
}

void cdi_term()
{
    if (cdi_fp) { fclose(cdi_fp); cdi_fp = NULL; }
    cdi_track_count = 0;
}
