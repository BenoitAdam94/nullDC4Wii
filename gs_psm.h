#ifndef GS_PSM_H
#define GS_PSM_H
#define GS_ZBUF_32 0


// Formats de pixel pour la Dreamcast/PVR
// Ces formats sont compatibles avec le PowerVR de la Dreamcast et peuvent être adaptés pour la Wii.

typedef enum {
    PVR_TXRFMT_ARGB4444 = 0,  // Format ARGB 4:4:4:4 (16 bits)
    PVR_TXRFMT_ARGB1555,      // Format ARGB 1:5:5:5 (16 bits)
    PVR_TXRFMT_RGB565,       // Format RGB 5:6:5 (16 bits)
    PVR_TXRFMT_YUV422,       // Format YUV 4:2:2 (16 bits)
    PVR_TXRFMT_PAL4,         // Format palettisé 4 bits (indexé)
    PVR_TXRFMT_PAL8,         // Format palettisé 8 bits (indexé)
    PVR_TXRFMT_BUMPMAP,      // Format bump map (spécifique aux effets de lumière)
    PVR_TXRFMT_MAX           // Nombre total de formats
} PVRTextureFormat;

// Définitions pour la compatibilité avec le code existant
// Ces définitions permettent de remplacer les formats PS2 par des formats PVR
#define GS_PSM_32   PVR_TXRFMT_ARGB4444  // Remplace GS_PSM_32 (32 bits) par ARGB4444
#define GS_PSM_24   PVR_TXRFMT_RGB565    // Remplace GS_PSM_24 (24 bits) par RGB565
#define GS_PSM_16   PVR_TXRFMT_ARGB1555  // Remplace GS_PSM_16 (16 bits) par ARGB1555
#define GS_PSM_16S  PVR_TXRFMT_ARGB1555  // Remplace GS_PSM_16S (16 bits avec stencil)
#define GS_PSM_8    PVR_TXRFMT_PAL8      // Remplace GS_PSM_8 (8 bits) par PAL8
#define GS_PSM_4    PVR_TXRFMT_PAL4      // Remplace GS_PSM_4 (4 bits) par PAL4

#endif // GS_PSM_H
