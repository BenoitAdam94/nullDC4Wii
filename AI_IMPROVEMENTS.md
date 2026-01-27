
# Files improved by AI

"Version" correspond to the released version (boot.dol) this file has been took account on. If the released version isn't out yet, it means it will be implemented in the next version.

## Core / Emulator Framework

| File | Function | Version | AI Improvement |
|------|----------|---------|----------------|
| main.cpp | Emulator entry point and initialization | 0.03 | Various AI improvement |
| dc.cpp | Dreamcast core system orchestration | | |
| nullDC.cpp | Main emulator implementation and loop | | |
| common.cpp | Shared utilities and helpers | | |
| config.cpp | Configuration handling | | |
| driver.cpp | Platform-specific driver glue | | |
| ui.cpp | User interface layer | | |

## Memory Management

| File | Function | Version | AI Improvement |
|------|----------|---------|----------------|
| _vmem.cpp | Virtual memory backend | | |
| memutil.cpp | Memory utility functions | | |
| mmu.cpp | Memory Management Unit emulation | | |
| sh4_mem.cpp | SH4 memory access logic | | |
| sh4_area0.cpp | SH4 Area 0 memory mapping | | |
| local_cache.cpp | Memory caching system | | |

## SH4 CPU Core

| File | Function | Version | AI Improvement |
|------|----------|---------|----------------|
| sh4_cpu.cpp | SH4 CPU core implementation | | |
| sh4_interpreter.cpp | SH4 instruction interpreter | | |
| sh4_fpu.cpp | SH4 floating point unit | | |
| sh4_if.cpp | SH4 interface layer | | |
| sh4_opcode_list.cpp | SH4 opcode definitions | | |
| sh4_registers.cpp | SH4 register definitions | | |
| sh4_internal_reg.cpp | SH4 internal registers | | |
| decoder.cpp | Instruction decoder | | |
| shil.cpp | SH4 intermediate language (JIT layer) | | |

## SH4 System Controllers

| File | Function | Version | AI Improvement |
|------|----------|---------|----------------|
| bsc.cpp | Bus State Controller | | |
| ccn.cpp | Cache Control | | |
| cpg.cpp | Clock Pulse Generator | | |
| dmac.cpp | DMA Controller | | |
| intc.cpp | Interrupt Controller | | |
| rtc.cpp | Real Time Clock | | |
| sci.cpp | Serial Communication Interface | | |
| scif.cpp | Serial Communication Interface with FIFO | | |
| tmu.cpp | Timer Management Unit | | |
| ubc.cpp | User Break Controller | | |

## AICA (Sound System)

| File | Function | Version | AI Improvement |
|------|----------|---------|----------------|
| aica_if.cpp | AICA sound interface | | |
| aica_hle.cpp | AICA high-level emulation | | |
| aica_hax.cpp | AICA experimental hacks | | |
| EmptyAICA.cpp | Stub AICA implementation | | |

## Graphics – PowerVR (PVR)

| File | Function | Version | AI Improvement |
|------|----------|---------|----------------|
| pvr_if.cpp | PVR interface layer | | |
| pvr_sb_regs.cpp | PVR system bus registers | | |
| pvrLock.cpp | PVR synchronization mechanisms | | |
| drkPvr.cpp | PVR renderer backend | | |
| Renderer_if.cpp | Renderer abstraction interface | | |
| regs.cpp | PVR register definitions | | |
| SPG.cpp | Sync Pulse Generator | | |
| ta.cpp | Tile Accelerator | | |
| tex_decode.cpp | Texture decoding logic | | |

## Rendering Backends

| File | Function | Version | AI Improvement |
|------|----------|---------|----------------|
| glesRend.cpp | OpenGL ES renderer | | |
| gsRend.cpp | Generic graphics renderer | | |
| gxRend.cpp | GX renderer (Wii / GameCube) | | |
| softRend.cpp | Software renderer | | |
| nullRend.cpp | Null renderer (no rendering output) | | |

## GD-ROM / Storage / Disc Images

| File | Function | Version | AI Improvement |
|------|----------|---------|----------------|
| gdromv3.cpp | GD-ROM device emulation | | |
| gdrom_response.cpp | GD-ROM command responses | | |
| iso.cpp | ISO image handling | | |
| iso9660.cpp | ISO9660 filesystem parsing | | |
| ImgReader.cpp | Disc image reader | | |
| cdi.cpp | CDI image format support | | |
| mds.cpp | MDS image format support | | |
| mds_reader.cpp | MDS file reader | | |
| ioctl.cpp | Low-level device I/O control | | |

## Maple Bus (Controllers & Peripherals)

| File | Function | Version | AI Improvement |
|------|----------|---------|----------------|
| maple_if.cpp | Maple bus interface | | |
| maple_cfg.cpp | Maple device configuration | | |
| maple_devs.cpp | Maple device definitions | | |
| maple_helper.cpp | Maple helper utilities | | |
| drkMapleDevices.cpp | Custom Maple device implementations | | |
| nullExtDev.cpp | Stub external Maple device | | |

## System Bus & ASIC

| File | Function | Version | AI Improvement |
|------|----------|---------|----------------|
| asic.cpp | ASIC interrupt and event controller | | |
| sb.cpp | System bus implementation | | |

## Plugins & Extensions

| File | Function | Version | AI Improvement |
|------|----------|---------|----------------|
| plugin_manager.cpp | Plugin management system | | |
| plugin_types.cpp | Plugin type definitions | | |
| blockmanager.cpp | RAM Buffer - Memory and block manager | 0.04 | Claude AI improvement |

## Networking

| File | Function | Version | AI Improvement |
|------|----------|---------|----------------|
| remote_tcp.cpp | Remote TCP communication | | |

## Platform-Specific (Wii)

| File | Function | Version | AI Improvement |
|------|----------|---------|----------------|
| wii_driver.cpp | Wii hardware driver | | |
| wii_types.cpp | Wii-specific type definitions | | |
