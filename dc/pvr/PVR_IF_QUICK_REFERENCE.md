# PVR Interface Quick Reference Guide

## Overview
The PowerVR (PVR) interface handles graphics operations for the Dreamcast emulator, including YUV video conversion, texture uploads, and VRAM access.

---

## Key Components

### 1. YUV Video Converter
Converts YUV 4:2:0 video macroblocks to YUYV format for texture mapping.

**Format: YUV 4:2:0**
- Macroblock: 16x16 pixels
- Size: 384 bytes per block
- Layout: 64 bytes U + 64 bytes V + 256 bytes Y
- Output: YUYV packed format (2 bytes per pixel)

**Data Flow:**
```
Input (YUV 4:2:0) → Temp Buffer → Convert → VRAM (YUYV)
```

### 2. Address Space Routing

**TAWrite/TAWriteSQ address routing:**

| Address Range | Destination | Purpose |
|--------------|-------------|---------|
| 0x0000000 - 0x07FFFFF | TA Polygon | 3D geometry data |
| 0x0800000 - 0x0FFFFFF | YUV Converter | Video texture data |
| 0x1000000 - 0x1FFFFFF | Direct VRAM | Raw texture/framebuffer |

### 3. Register Map

| Address | Name | Purpose |
|---------|------|---------|
| 0x5F8148 | TA_YUV_TEX_BASE | VRAM destination address |
| 0x5F814C | TA_YUV_TEX_CTRL | YUV configuration |
| 0x5F8150 | TA_YUV_TEX_CNT | Blocks completed counter |

---

## Function Reference

### YUV Converter Functions

#### `YUV_init()`
**Purpose:** Initialize YUV converter state from hardware registers

**Called when:**
- TA_YUV_TEX_BASE register is written
- Converter needs reset after completion

**Side effects:**
- Resets all YUV state variables
- Reads configuration from registers
- Calculates texture dimensions

#### `YUV_ConvertMacroBlock()`
**Purpose:** Convert one 16x16 macroblock from YUV to YUYV

**Algorithm:**
```
For each pixel pair (x, x+1) in 16x16 block:
  1. Get Y0 from luminance plane
  2. Get U from chrominance plane (shared)
  3. Get Y1 from luminance plane
  4. Get V from chrominance plane (shared)
  5. Pack as YUYV and write to VRAM
```

**Triggers interrupt when:** All blocks complete

#### `YUV_data(u32* data, u32 count)`
**Purpose:** Process incoming YUV data stream

**Parameters:**
- `data`: Pointer to YUV data
- `count`: Number of 32-byte blocks

**Behavior:**
- Buffers partial blocks
- Converts complete blocks automatically
- Handles block boundaries correctly

### Register Access

#### `pvr_readreg_TA(u32 addr, u32 sz)`
**Returns:**
- `TA_YUV_TEX_CNT`: Number of completed blocks
- Other registers: Delegated to plugin

#### `pvr_writereg_TA(u32 addr, u32 data, u32 sz)`
**Special handling:**
- `TA_YUV_TEX_BASE`: Triggers YUV_init()

### VRAM Access

#### Read/Write Functions
```cpp
u16 pvr_read_area1_16(u32 addr);   // 16-bit read
u32 pvr_read_area1_32(u32 addr);   // 32-bit read
void pvr_write_area1_16(u32 addr, u16 data);  // 16-bit write
void pvr_write_area1_32(u32 addr, u32 data);  // 32-bit write
```

**Note:** 8-bit operations not supported by hardware

**Address conversion:**
- All addresses converted from 32-bit to 64-bit space
- Uses `vramlock_ConvOffset32toOffset64()`
- Respects memory banking

### DMA Interface

#### `TAWrite(u32 address, u32* data, u32 count)`
**Purpose:** Bulk DMA transfer to TA

**Count:** Number of 32-byte blocks

**Routes to:**
- TA polygon path (libPvr_TaDMA)
- YUV converter (YUV_data)
- Direct VRAM (memcpy)

#### `TAWriteSQ(u32 address, u32* data)`
**Purpose:** Store Queue optimized write

**Size:** Always 32 bytes (1 block)

**Same routing as TAWrite**

---

## YUV Format Details

### YUV 4:2:0 Memory Layout

```
Macroblock (384 bytes):
┌──────────────────┐
│ U (64 bytes)     │  8x8 chrominance
│    8x8 grid      │
├──────────────────┤
│ V (64 bytes)     │  8x8 chrominance
│    8x8 grid      │
├──────────────────┤
│ Y (256 bytes)    │  16x16 luminance
│   16x16 grid     │  (4 x 8x8 blocks)
└──────────────────┘
```

### YUYV Output Format

```
Pixel pair (4 bytes):
┌────┬────┬────┬────┐
│ Y0 │ U  │ Y1 │ V  │
└────┴────┴────┴────┘
  Pixel 0  | Pixel 1
```

**Characteristics:**
- 2 pixels per 4 bytes
- Chrominance shared between pixel pairs
- Horizontal 2:1 subsampling
- Direct hardware texture format

---

## State Variables

### Global YUV State
```cpp
YUV_tempdata[128]  // Temp buffer (512 bytes)
YUV_index          // Current position in buffer
YUV_dest           // VRAM destination address
YUV_doneblocks     // Completed block count
YUV_blockcount     // Total blocks to process

YUV_x_curr         // Current X position (pixels)
YUV_y_curr         // Current Y position (pixels)
YUV_x_size         // Output width (pixels)
YUV_y_size         // Output height (pixels)
```

---

## Common Patterns

### Processing YUV Video Frame

```cpp
// 1. Configure converter
pvr_writereg_TA(TA_YUV_TEX_BASE, vram_addr, 4);
pvr_writereg_TA(TA_YUV_TEX_CTRL, config, 4);

// 2. Send data (happens automatically via DMA)
TAWrite(0x00800000, yuv_data, block_count);

// 3. Wait for interrupt
// holly_YUV_DMA interrupt fires when complete

// 4. Texture ready in VRAM at vram_addr
```

### Direct VRAM Write

```cpp
// Write texture directly to VRAM
TAWrite(0x01000000 | vram_offset, texture_data, size_in_32byte_blocks);
```

### Store Queue Optimization

```cpp
// SH4 Store Queue optimized path
for (int i = 0; i < data_blocks; i++) {
    TAWriteSQ(dest_address, &data[i * 8]);  // 8 u32s = 32 bytes
}
```

---

## Error Handling

### Common Issues

**"YUV decoder not initialized"**
- Cause: YUV_data called before register setup
- Solution: Auto-initializes, but should set registers first

**"8-bit VRAM reads/writes not supported"**
- Cause: Code attempting 8-bit access
- Solution: Use 16-bit or 32-bit access only

**"YUV 4:2:2 format not supported"**
- Cause: Format bit set in TA_YUV_TEX_CTRL
- Solution: Use YUV 4:2:0 format (bit 24 = 0)

---

## Hardware Notes

### Real Dreamcast Behavior

1. **YUV Converter:**
   - Operates independently
   - Raises interrupt on completion
   - Supports both 4:2:0 and 4:2:2 (emulator: 4:2:0 only)

2. **VRAM Access:**
   - 64-bit interleaved addressing
   - 16-bit and 32-bit access only
   - Respects lock modes

3. **TA Interface:**
   - Accepts polygon lists, textures, and YUV
   - 32-byte transfer granularity
   - Store Queue optimizations available

---

## Performance Tips

1. **Batch YUV Data:**
   - Send multiple blocks per TAWrite
   - Reduces function call overhead

2. **Use Store Queue:**
   - TAWriteSQ for single block writes
   - Hardware optimized path

3. **Align Data:**
   - 32-byte alignment recommended
   - Improves cache performance

4. **Minimize State Changes:**
   - Group similar operations
   - Reduce register writes

---

## Debug Checklist

When debugging PVR issues:

- [ ] Register values correct?
- [ ] Address routing correct?
- [ ] Block counts match?
- [ ] YUV format supported (4:2:0)?
- [ ] VRAM destination valid?
- [ ] Interrupts enabled?
- [ ] Data alignment correct?
- [ ] Lock modes respected?

---

## Constants Quick Reference

```cpp
// YUV Formats
YUV_BLOCK_SIZE_420 = 384   // bytes per macroblock
YUV_BLOCK_SIZE_422 = 512   // bytes per macroblock
YUV_MACROBLOCK_SIZE = 16   // pixels per dimension

// Address Ranges
TA_POLY_RANGE = 0x000000 - 0x7FFFFF    // 8MB
YUV_CONV_RANGE = 0x800000 - 0xFFFFFF   // 8MB
VRAM_WRITE_RANGE = 0x1000000+          // 16MB+

// Registers
TA_YUV_TEX_BASE = 0x5F8148
TA_YUV_TEX_CTRL = 0x5F814C
TA_YUV_TEX_CNT = 0x5F8150
PVR_BASE = 0x005F8000
```

---

## See Also

- PVR_IMPROVEMENTS.md - Detailed improvement documentation
- pvr_if.h - Header file with full API
- pvr_if.cpp - Implementation details
- Hardware documentation for PowerVR2DC

