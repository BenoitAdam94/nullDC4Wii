# Wii Memory Architecture & Dreamcast Emulation Guide

This document outlines the differences between the Wii's memory banks and provides a strategic recommendation for memory mapping when emulating the Sega Dreamcast.

---

## 1. Wii Memory Overview: MEM1 vs. MEM2

The Wii utilizes a split-bus memory architecture inherited and expanded from the GameCube.

| Feature | MEM1 (Internal) | MEM2 (External) |
| :--- | :--- | :--- |
| **Size** | 24 MB | 64 MB |
| **Type** | 1T-SRAM | GDDR3 SDRAM |
| **Bus Width** | 64-bit | 32-bit |
| **Latency** | Ultra-low (High Speed) | High (Moderate Speed) |
| **Primary Use** | GPU Framebuffers, Display Lists | Textures, Audio, General Data |

### Key Distinction
* **MEM1** is physically located on the "Hollywood" GPU die. It acts more like a massive L3 cache with extremely high bandwidth.
* **MEM2** is "bulk" storage. While larger, accessing it incurs a latency penalty that can slow down CPU-intensive tasks.

---

## 2. Comparison: Dreamcast vs. Wii

To emulate a Dreamcast, you must map its physical components to the Wii’s resources.

* **Dreamcast System RAM:** 16 MB
* **Dreamcast Video RAM (VRAM):** 8 MB
* **Dreamcast Sound RAM:** 2 MB
* **Total Target:** ~26 MB

---

## 3. Recommended Caching Strategy

For optimal performance in a Dreamcast emulator, use the following mapping:

### **Cache Textures in MEM2**
While MEM2 has higher latency, it is the correct place for the **Texture Cache (VRAM)** for several reasons:

1.  **Space Preservation:** MEM1 (24 MB) is too small to comfortably fit the Dreamcast’s System RAM (16 MB) *and* its VRAM (8 MB) while still leaving room for the Wii's own framebuffers and OS overhead.
2.  **CPU Performance:** The Dreamcast's SH-4 CPU is the most difficult part to emulate. By placing the **16 MB System RAM in MEM1**, you ensure the Wii CPU can access emulated memory with the lowest possible latency.
3.  **GPU Throughput:** The Wii's GX GPU is efficient at pulling texture data from MEM2. The latency penalty of MEM2 is less impactful for texture mapping than it is for raw CPU instruction execution.

### **Suggested Memory Map**
* **MEM1 (24 MB):**
    * Emulated SH-4 Main RAM (16 MB)
    * Critical Emulation Kernel/Hot-code
    * Wii EFB (Embedded Frame Buffer)
* **MEM2 (64 MB):** * **Texture Cache / VRAM (8 MB)**
    * Sound RAM (2 MB)
    * Asset Buffers (CD-ROM images/GDI)
    * Emulator UI and File System overhead

---

> **Pro Tip:** If you encounter specific "heavy" games, you can implement a **hybrid cache**. Keep the active textures in a small pool in MEM1 and overflow the rest to MEM2.