#ifndef GRAPH_H
#define GRAPH_H

#include <cstdint>
#include "tamtypes.h"  // Pour les types u8, u16, u32, etc.

//--- Définitions des formats de pixel pour les framebuffers et textures ---
typedef enum {
    GRAPH_PIXEL_FORMAT_ARGB4444 = 0,  // Format ARGB 4:4:4:4 (16 bits)
    GRAPH_PIXEL_FORMAT_ARGB1555,      // Format ARGB 1:5:5:5 (16 bits)
    GRAPH_PIXEL_FORMAT_RGB565,        // Format RGB 5:6:5 (16 bits)
    GRAPH_PIXEL_FORMAT_YUV422,        // Format YUV 4:2:2 (16 bits)
    GRAPH_PIXEL_FORMAT_PAL4,          // Format palettisé 4 bits
    GRAPH_PIXEL_FORMAT_PAL8,          // Format palettisé 8 bits
    GRAPH_PIXEL_FORMAT_MAX            // Nombre total de formats
} GraphPixelFormat;

//--- Définitions des alignements mémoire pour les textures et framebuffers ---
typedef enum {
    GRAPH_ALIGN_PAGE = 0,  // Alignement sur une page (4 Ko)
    GRAPH_ALIGN_BLOCK,     // Alignement sur un bloc (64 octets)
    GRAPH_ALIGN_PIXEL      // Alignement sur un pixel (1 octet)
} GraphAlignment;

//--- Structure pour un framebuffer ---
typedef struct {
    u32 width;            // Largeur du framebuffer
    u32 height;           // Hauteur du framebuffer
    u32 address;          // Adresse du framebuffer en mémoire
    GraphPixelFormat format; // Format de pixel
    u32 mask;             // Masque (optionnel)
    u32 psm; // Was missing, MistalAI add
} framebuffer_t;

//--- Structure pour un z-buffer ---
typedef struct {
    u32 enable;           // Activer/désactiver le z-buffer
    u32 address;          // Adresse du z-buffer en mémoire
    u32 mask;             // Masque (optionnel)
    u32 zsm;              // Format du z-buffer (ex: 16 bits ou 32 bits)
    u32 method; // Was missing, MistalAI add
} zbuffer_t;

//--- Structure pour un buffer de texture ---
typedef struct {
    u32 width;            // Largeur de la texture
    u32 height;           // Hauteur de la texture
    u32 address;          // Adresse de la texture en mémoire
    GraphPixelFormat format; // Format de pixel
    u32 psm; // Was missing, MistalAI add
} texbuffer_t;

//--- Fonctions pour allouer de la mémoire VRAM ---
u32 graph_vram_allocate(u32 width, u32 height, GraphPixelFormat format, GraphAlignment alignment) {
    // Sur Dreamcast/Wii, cette fonction alloue de la mémoire pour les textures ou framebuffers.
    // Retourne une adresse simulée (à adapter selon ton système de gestion mémoire).
    static u32 vram_pointer = 0x05000000;  // Adresse de base de la VRAM (exemple)
    u32 size = width * height;

    // Calculer la taille en octets selon le format
    switch (format) {
        case GRAPH_PIXEL_FORMAT_ARGB4444:
        case GRAPH_PIXEL_FORMAT_ARGB1555:
        case GRAPH_PIXEL_FORMAT_RGB565:
        case GRAPH_PIXEL_FORMAT_YUV422:
            size *= 2;  // 2 octets par pixel
            break;
        case GRAPH_PIXEL_FORMAT_PAL4:
            size /= 2;  // 4 pixels par octet
            break;
        case GRAPH_PIXEL_FORMAT_PAL8:
            break;      // 1 octet par pixel
        default:
            size *= 4;  // Par défaut, 4 octets par pixel
            break;
    }

    // Aligner l'adresse selon l'alignement demandé
    u32 align = 1;
    switch (alignment) {
        case GRAPH_ALIGN_PAGE:
            align = 4096;  // 4 Ko
            break;
        case GRAPH_ALIGN_BLOCK:
            align = 64;    // 64 octets
            break;
        case GRAPH_ALIGN_PIXEL:
            align = 1;     // Aucun alignement
            break;
    }

    vram_pointer = (vram_pointer + align - 1) & ~(align - 1);
    u32 address = vram_pointer;
    vram_pointer += size;

    return address;
}

//--- Fonctions pour initialiser le rendu graphique ---
void graph_initialize(u32 framebuffer_address, u32 width, u32 height, GraphPixelFormat format, u32 x_offset, u32 y_offset) {
    // Initialiser le rendu graphique avec le framebuffer spécifié.
    // Sur Wii, cela pourrait configurer le GX pour utiliser ce framebuffer.
}

//--- Fonctions pour attendre la synchronisation verticale ---
void graph_wait_vsync() {
    // Attendre la synchronisation verticale (VSync).
    // Sur Wii, cela peut être implémenté avec `VID_WaitVSync()` (KOS) ou une fonction similaire.
}

//--- Définitions pour la compatibilité avec le code PS2 ---
/*
#define GS_PSM_32   GRAPH_PIXEL_FORMAT_ARGB4444  // Remplace GS_PSM_32
#define GS_PSM_24   GRAPH_PIXEL_FORMAT_RGB565    // Remplace GS_PSM_24
#define GS_PSM_16   GRAPH_PIXEL_FORMAT_ARGB1555  // Remplace GS_PSM_16
#define GS_PSM_8    GRAPH_PIXEL_FORMAT_PAL8      // Remplace GS_PSM_8
#define GS_PSM_4    GRAPH_PIXEL_FORMAT_PAL4      // Remplace GS_PSM_4
*/
#endif // GRAPH_H
