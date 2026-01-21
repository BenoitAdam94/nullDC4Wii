#ifndef TAMTYPES_H
#define TAMTYPES_H

#include <stdint.h>
#include <stdbool.h>

// Types de base
typedef int8_t         s8;
typedef int16_t        s16;
typedef int32_t        s32;
typedef int64_t        s64;
typedef uint8_t        u8;
typedef uint16_t       u16;
typedef uint32_t       u32;
typedef uint64_t       u64;
typedef float          f32;
typedef double         f64;
typedef volatile u32   vu32;
typedef volatile u16   vu16;
typedef volatile u8    vu8;

// Types pour les adresses mémoire
typedef u32            addr32;
typedef u64            addr64;

// Types pour les registres
typedef u32            reg32;
typedef u64            reg64;

// Types pour les couleurs (RGBA, etc.)
typedef struct {
    u8 r, g, b, a;
} color8888;

// Types pour les vecteurs 2D/3D
typedef struct {
    f32 x, y;
} vec2f;

typedef struct {
    f32 x, y, z;
} vec3f;

typedef struct {
    f32 x, y, z, w;
} vec4f;

// Macros utiles
#define BIT(n)          (1U << (n))
#define SET_BIT(var, n) ((var) |= BIT(n))
#define CLR_BIT(var, n) ((var) &= ~BIT(n))
#define TST_BIT(var, n) ((var) & BIT(n))

// Macros pour l'alignement mémoire
// #define ALIGN(n, align)     (((n) + (align) - 1) & ~((align) - 1))
// #define ALIGN16(n) __attribute__((aligned(16))) // Already defined
// #define ALIGN32(n) __attribute__((aligned(32))) // Already defined
#define ALIGN_4(n)          ALIGN(n, 4)
#define ALIGN_8(n)          ALIGN(n, 8)
#define ALIGN_16(n)         ALIGN(n, 16)
#define ALIGN_32(n)         ALIGN(n, 32)

// Macros pour les tailles courantes
#define KB(n)              ((n) * 1024)
#define MB(n)              ((n) * 1024 * 1024)

// Macros pour les conversions d'endianness
#define SWAP16(val)        (((val >> 8) & 0xFF) | ((val & 0xFF) << 8))
#define SWAP32(val)        (((val >> 24) & 0xFF) | ((val >> 8) & 0xFF00) | ((val & 0xFF00) << 8) | ((val & 0xFF) << 24))
#define SWAP64(val)        (((val >> 56) & 0xFF) | ((val >> 40) & 0xFF00) | ((val >> 24) & 0xFF0000) | ((val >> 8) & 0xFF000000) | \
                           ((val & 0xFF000000) << 8) | ((val & 0xFF0000) << 24) | ((val & 0xFF00) << 40) | ((val & 0xFF) << 56))

// Macros pour les registres mémoire
#define READ32(addr)      (*(vu32*)(addr))
#define WRITE32(addr, val) (*(vu32*)(addr) = (val))
#define READ16(addr)      (*(vu16*)(addr))
#define WRITE16(addr, val) (*(vu16*)(addr) = (val))
#define READ8(addr)       (*(vu8*)(addr))
#define WRITE8(addr, val) (*(vu8*)(addr) = (val))

// Définitions pour les flags et états
#define TRUE              1
#define FALSE             0

#endif // TAMTYPES_H
