## Core / General Infrastructure

| File | Function | Emulation Level | Accuracy Level | Danger Zone | Version | Notes |
|------|----------|-----------------|----------------|-------------|---------|-------|
| main.cpp | Application entry point | N/A | N/A | No |  |  |
| nullDC.cpp | Emulator core initialization | N/A | Approximate | No |  |  |
| dc.cpp | Dreamcast system management | Mixed | Timing-sensitive | Yes |  | Boot order, resets |
| config.cpp | Configuration handling | N/A | N/A | No |  |  |
| common.cpp | Common utilities | N/A | N/A | No |  |  |
| stdclass.cpp | Standard utility classes | N/A | N/A | No |  |  |
| cl.cpp | Command-line handling | N/A | N/A | No |  |  |

## Memory / MMU / System Bus

| File | Function | Emulation Level | Accuracy Level | Danger Zone | Version | Notes |
|------|----------|-----------------|----------------|-------------|---------|-------|
| _vmem.cpp | Virtual memory system | LLE | Timing-sensitive | Yes |  | Address mirroring |
| memutil.cpp | Memory utilities | N/A | N/A | No |  |  |
| mmu.cpp | Memory Management Unit | LLE | Timing-sensitive | Yes |  | TLB, faults |
| sb.cpp | System Bus | LLE | Timing-sensitive | Yes |  | DMA / contention |
| blockmanager.cpp | Memory block management | N/A | Approximate | No |  |  |
| local_cache.cpp | Local cache handling | LLE | Approximate | Yes |  | Cache coherency |

## SH4 CPU Core

| File | Function | Emulation Level | Accuracy Level | Danger Zone | Version | Notes |
|------|----------|-----------------|----------------|-------------|---------|-------|
| sh4_cpu.cpp | SH4 CPU core | LLE | Timing-sensitive | Yes |  | Pipeline effects |
| sh4_fpu.cpp | SH4 Floating Point Unit | LLE | Approximate | Yes |  | IEEE edge cases |
| sh4_if.cpp | SH4 interface layer | Mixed | Approximate | No |  |  |
| sh4_interpreter.cpp | SH4 interpreter | HLE | Approximate | No |  | Reference path |
| sh4_opcode_list.cpp | SH4 opcode definitions | LLE | Cycle-accurate | No |  |  |
| sh4_registers.cpp | SH4 register definitions | LLE | Cycle-accurate | No |  |  |
| sh4_mem.cpp | SH4 memory access | LLE | Timing-sensitive | Yes |  | Load/store timing |
| sh4_area0.cpp | SH4 Area0 memory mapping | LLE | Timing-sensitive | Yes |  | MMIO overlap |
| sh4_internal_reg.cpp | SH4 internal registers | LLE | Cycle-accurate | No |  |  |

## SH4 Internal Peripherals

| File | Function | Emulation Level | Accuracy Level | Danger Zone | Version | Notes |
|------|----------|-----------------|----------------|-------------|---------|-------|
| bsc.cpp | Bus State Controller | LLE | Timing-sensitive | Yes |  |  |
| ccn.cpp | Cache Control | LLE | Approximate | Yes |  | Cache invalidation |
| cpg.cpp | Clock Pulse Generator | LLE | Timing-sensitive | Yes |  | Clock ratios |
| dmac.cpp | DMA Controller | LLE | Timing-sensitive | Yes |  | Burst timing |
| intc.cpp | Interrupt Controller | LLE | Timing-sensitive | Yes |  | Priority handling |
| rtc.cpp | Real-Time Clock | LLE | Approximate | No |  |  |
| sci.cpp | Serial Communication Interface | LLE | Approximate | No |  |  |
| scif.cpp | Serial Communication Interface with FIFO | LLE | Approximate | No |  |  |
| tmu.cpp | Timer Management Unit | LLE | Timing-sensitive | Yes |  | Timer drift |
| ubc.cpp | User Break Controller | LLE | Approximate | No |  | Debug only |

## Maple Bus (Controllers & Peripherals)

| File | Function | Emulation Level | Accuracy Level | Danger Zone | Version | Notes |
|------|----------|-----------------|----------------|-------------|---------|-------|
| maple_if.cpp | Maple bus interface | LLE | Timing-sensitive | Yes |  | Polling timing |
| maple_cfg.cpp | Maple configuration | HLE | Approximate | No |  |  |
| maple_devs.cpp | Maple devices | HLE | Approximate | No |  | Controllers |
| maple_helper.cpp | Maple helper functions | HLE | Approximate | No |  |  |
| drkMapleDevices.cpp | Extended Maple devices | HLE | Approximate | No |  |  |
| nullExtDev.cpp | Null external device | HLE | N/A | No |  |  |

## GD-ROM / Disk Images

| File | Function | Emulation Level | Accuracy Level | Danger Zone | Version | Notes |
|------|----------|-----------------|----------------|-------------|---------|-------|
| gdromv3.cpp | GD-ROM emulation (v3) | Mixed | Timing-sensitive | Yes |  | Seek timing |
| gdrom_response.cpp | GD-ROM response handling | HLE | Approximate | No |  |  |
| cdi.cpp | CDI image support | HLE | Approximate | No |  |  |
| iso.cpp | ISO image handling | HLE | Approximate | No |  |  |
| iso9660.cpp | ISO9660 filesystem | HLE | Approximate | No |  |  |
| ImgReader.cpp | Disk image reader | HLE | Approximate | No |  |  |
| mds.cpp | MDS image support | HLE | Approximate | No |  |  |
| mds_reader.cpp | MDS image reader | HLE | Approximate | No |  |  |
| ioctl.cpp | IO control layer | LLE | Timing-sensitive | Yes |  | OS interaction |

## AICA Audio System

| File | Function | Emulation Level | Accuracy Level | Danger Zone | Version | Notes |
|------|----------|-----------------|----------------|-------------|---------|-------|
| aica_if.cpp | AICA interface | Mixed | Timing-sensitive | Yes |  | Sync with SH4 |
| aica_hle.cpp | AICA high-level emulation | HLE | Approximate | Yes |  | Audio glitches |
| aica_hax.cpp | AICA-specific hacks | HLE | Inaccurate | Yes |  | Game-specific |
| EmptyAICA.cpp | Dummy AICA implementation | HLE | N/A | No |  |  |

## PVR Graphics Core

| File | Function | Emulation Level | Accuracy Level | Danger Zone | Version | Notes |
|------|----------|-----------------|----------------|-------------|---------|-------|
| pvr_if.cpp | PVR interface | Mixed | Timing-sensitive | Yes |  | FIFO sync |
| pvr_sb_regs.cpp | PVR system bus registers | LLE | Cycle-accurate | No |  |  |
| pvrLock.cpp | PVR synchronization | LLE | Timing-sensitive | Yes |  | Threading |
| drkPvr.cpp | PVR core implementation | Mixed | Approximate | Yes |  | Rendering order |
| SPG.cpp | System Power Generator | LLE | Timing-sensitive | Yes |  | VBlank timing |
| ta.cpp | Tile Accelerator | LLE | Timing-sensitive | Yes |  | Polygon binning |
| tex_decode.cpp | Texture decoding | HLE | Approximate | No |  |  |

## Renderers

| File | Function | Emulation Level | Accuracy Level | Danger Zone | Version | Notes |
|------|----------|-----------------|----------------|-------------|---------|-------|
| Renderer_if.cpp | Renderer interface | N/A | N/A | No |  |  |
| nullRend.cpp | Null renderer | N/A | N/A | No |  |  |
| softRend.cpp | Software renderer | HLE | Approximate | No |  | Reference |
| glesRend.cpp | OpenGL ES renderer | HLE | Approximate | Yes |  | Driver-dependent |
| gsRend.cpp | GS renderer | HLE | Approximate | Yes |  | Platform quirks |
| gxRend.cpp | GX renderer (Wii) | HLE | Approximate | Yes |  | Fixed-function |
| regs.cpp | Graphics registers | LLE | Cycle-accurate | No |  |  |

## Plugin System

| File | Function | Emulation Level | Accuracy Level | Danger Zone | Version | Notes |
|------|----------|-----------------|----------------|-------------|---------|-------|
| plugin_manager.cpp | Plugin management | N/A | N/A | No |  |  |
| plugin_types.cpp | Plugin type definitions | N/A | N/A | No |  |  |
| driver.cpp | Generic plugin driver | N/A | Approximate | No |  |  |
| decoder.cpp | Instruction / stream decoder | Mixed | Approximate | Yes |  | JIT impact |
| shil.cpp | SH4 Intermediate Language | HLE | Approximate | Yes |  | Reordering |

## Network / UI / Platform-Specific

| File | Function | Emulation Level | Accuracy Level | Danger Zone | Version | Notes |
|------|----------|-----------------|----------------|-------------|---------|-------|
| remote_tcp.cpp | Remote TCP communication | HLE | Approximate | No |  |  |
| ui.cpp | User interface | HLE | N/A | No |  |  |
| wii_driver.cpp | Wii-specific driver | HLE | Approximate | Yes |  | Platform hacks |
| wii_types.cpp | Wii-specific type definitions | N/A | N/A | No |  |  |
