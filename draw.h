#ifndef DRAW_H
#define DRAW_H

#include <cstdint>
#include "tamtypes.h"  // Pour les types u8, u16, u32, etc.
#include "graph.h"     // Pour les structures comme framebuffer_t et zbuffer_t

#define PRIM_SHADE_GOURAUD 1
#define PRIM_MAP_ST 1
#define PRIM_UNFIXED 0
#define ZTEST_METHOD_GREATER_EQUAL 1

//--- Définitions des types de primitives ---
typedef enum {
    PRIM_POINT_LIST = 0,      // Liste de points
    PRIM_LINE_LIST,           // Liste de lignes
    PRIM_LINE_STRIP,          // Bande de lignes
    PRIM_TRIANGLE_LIST,       // Liste de triangles
    PRIM_TRIANGLE_STRIP,      // Bande de triangles
    PRIM_TRIANGLE_FAN,        // Éventail de triangles
    PRIM_QUAD_LIST,           // Liste de quadrilatères
    PRIM_MAX                  // Nombre total de types de primitives
    
} PrimitiveType;

//--- Définitions des types de tests de profondeur ---
typedef enum {
    DRAW_DISABLE = 0,         // Désactiver les tests
    DRAW_ENABLE              // Activer les tests
} DrawEnable;

//--- Définitions des registres pour les primitives ---
typedef enum {
    DRAW_STQ_REGLIST,     // Liste de registres STQ
    DRAW_XYZ_REGLIST,    // Liste de registres XYZ
    DRAW_RGBAQ_REGLIST    // Liste de registres RGBAQ
} DrawRegList;

//--- Structure pour une primitive ---
typedef struct {
    PrimitiveType type;      // Type de primitive
    u32 shading;             // Type d'ombrage (ex: plat ou Gouraud)
    DrawEnable mapping;       // Activation du mappage de texture
    DrawEnable fogging;       // Activation du brouillard
    DrawEnable blending;      // Activation du mélange
    DrawEnable antialiasing;  // Activation de l'anti-crénelage
    u32 mapping_type;         // Type de mappage (ex: ST ou UV)
    u32 colorfix;             // Fixation de la couleur
} prim_t;

//--- Structure pour une couleur ---
typedef struct {
    u8 r, g, b, a;           // Composantes RGBA
    float q;                 // Paramètre Q (pour les calculs de perspective)
} color_t;

//--- Structure pour une coordonnée de texture ---
typedef struct {
    float u, v;              // Coordonnées de texture (U, V)
} texel_t;

//--- Structure pour une coordonnée 3D ---
typedef struct {
    float x, y, z;           // Coordonnées 3D (X, Y, Z)
} xyz_t;

//--- Structure pour un paquet de données (similaire à la PS2) ---
typedef struct {
    u32* data;               // Pointeur vers les données
    u32 size;                // Taille des données (en mots de 32 bits)
} packet_t;

//--- Structure pour un mot de 128 bits (similaire à la PS2) ---
typedef struct {
    u32 data[4];             // Données sur 128 bits (4 mots de 32 bits)
} qword_t;

//--- Fonctions pour initialiser un paquet ---
packet_t* packet_init(u32 size, u32 type) {
    packet_t* packet = new packet_t();
    packet->data = new u32[size];
    packet->size = size;
    return packet;
}

//--- Fonctions pour libérer un paquet ---
void packet_free(packet_t* packet) {
    if (packet != nullptr) {
        delete[] packet->data;
        delete packet;
    }
}

//--- Fonctions pour démarrer une primitive ---
qword_t* draw_prim_start(qword_t* q, u32 context, prim_t* prim, color_t* color) {
    // Sur Dreamcast/Wii, cette fonction prépare les données pour le rendu d'une primitive.
    // Retourne un pointeur vers les données de la primitive.
    return q;
}

//--- Fonctions pour terminer une primitive ---
qword_t* draw_prim_end(qword_t* q, u32 reglist_count, DrawRegList reglist) {
    // Sur Dreamcast/Wii, cette fonction finalise les données de la primitive.
    return q;
}

//--- Fonctions pour désactiver les tests de profondeur ---
qword_t* draw_disable_tests(qword_t* q, u32 context, zbuffer_t* z) {
    // Désactive les tests de profondeur pour le rendu.
    return q;
}

//--- Fonctions pour activer les tests de profondeur ---
qword_t* draw_enable_tests(qword_t* q, u32 context, zbuffer_t* z) {
    // Active les tests de profondeur pour le rendu.
    return q;
}

//--- Fonctions pour effacer le framebuffer ---
qword_t* draw_clear(qword_t* q, u32 context, u32 x, u32 y, u32 width, u32 height, u8 r, u8 g, u8 b) {
    // Efface le framebuffer avec la couleur spécifiée.
    return q;
}

//--- Fonctions pour configurer l'environnement de dessin ---
qword_t* draw_setup_environment(qword_t* q, u32 context, framebuffer_t* frame, zbuffer_t* z) {
    // Configure l'environnement de dessin avec le framebuffer et le z-buffer spécifiés.
    return q;
}

//--- Fonctions pour définir un décalage de primitive ---
qword_t* draw_primitive_xyoffset(qword_t* q, u32 context, u32 xoffset, u32 yoffset) {
    // Définit un décalage pour les coordonnées des primitives.
    return q;
}

//--- Fonctions pour terminer le rendu ---
qword_t* draw_finish(qword_t* q) {
    // Finalise le rendu et prépare les données pour l'affichage.
    return q;
}

//--- Fonctions pour attendre la fin du rendu ---
void draw_wait_finish() {
    // Attend que le rendu soit terminé (simulé sur Dreamcast/Wii).
}

//--- Définitions pour la compatibilité avec le code PS2 ---
#define DRAW_STQ_REGLIST 0  // Liste de registres STQ

#endif // DRAW_H
