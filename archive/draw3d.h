#ifndef DRAW3D_H
#define DRAW3D_H

#include <cstdint>
#include "tamtypes.h"  // Pour les types u8, u16, u32, etc.
#include "graph.h"     // Pour les structures de framebuffer et zbuffer
#include "draw.h"      // Pour les structures de primitives et couleurs

//--- Définitions des modes de rendu 3D ---
typedef enum {
    DRAW3D_MODE_WIREFRAME = 0,  // Mode fil de fer
    DRAW3D_MODE_SOLID,         // Mode plein
    DRAW3D_MODE_TEXTURED,      // Mode texturé
    DRAW3D_MODE_MAX            // Nombre total de modes
} Draw3DMode;

//--- Définitions des types de lumière ---
typedef enum {
    DRAW3D_LIGHT_DIRECTIONAL = 0,  // Lumière directionnelle
    DRAW3D_LIGHT_POINT,           // Lumière ponctuelle
    DRAW3D_LIGHT_SPOT,            // Lumière spot
    DRAW3D_LIGHT_AMBIENT,          // Lumière ambiante
    DRAW3D_LIGHT_MAX              // Nombre total de types de lumière
} Draw3DLightType;

//--- Structure pour une lumière ---
typedef struct {
    Draw3DLightType type;         // Type de lumière
    float x, y, z;               // Position ou direction de la lumière
    float r, g, b;               // Couleur de la lumière
    float range;                  // Portée de la lumière (pour les lumières ponctuelles et spots)
    float spotAngle;              // Angle du cône (pour les lumières spots)
    float attenuation0;           // Atténuation constante
    float attenuation1;           // Atténuation linéaire
    float attenuation2;           // Atténuation quadratique
} Draw3DLight;

//--- Structure pour un matériau ---
typedef struct {
    float ambient[4];   // Couleur ambiante (RGBA)
    float diffuse[4];   // Couleur diffuse (RGBA)
    float specular[4];  // Couleur spéculaire (RGBA)
    float shininess;     // Brillance
} Draw3DMaterial;

//--- Structure pour une matrice de transformation ---
typedef struct {
    float m[4][4];      // Matrice 4x4
} Draw3DMatrix;

//--- Structure pour une scène 3D ---
typedef struct {
    Draw3DMatrix viewMatrix;      // Matrice de vue
    Draw3DMatrix projMatrix;      // Matrice de projection
    Draw3DLight lights[8];        // Tableau de lumières
    u32 lightCount;               // Nombre de lumières actives
    Draw3DMaterial material;      // Matériau courant
} Draw3DScene;

//--- Fonctions pour initialiser une scène 3D ---
void draw3d_init_scene(Draw3DScene* scene) {
    // Initialise une scène 3D avec des valeurs par défaut
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            scene->viewMatrix.m[i][j] = (i == j) ? 1.0f : 0.0f;  // Matrice identité
            scene->projMatrix.m[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
    scene->lightCount = 0;
    scene->material.ambient[0] = scene->material.ambient[1] = scene->material.ambient[2] = 0.2f;
    scene->material.ambient[3] = 1.0f;
    scene->material.diffuse[0] = scene->material.diffuse[1] = scene->material.diffuse[2] = 0.8f;
    scene->material.diffuse[3] = 1.0f;
    scene->material.specular[0] = scene->material.specular[1] = scene->material.specular[2] = 0.0f;
    scene->material.specular[3] = 1.0f;
    scene->material.shininess = 32.0f;
}

//--- Fonctions pour ajouter une lumière à une scène ---
void draw3d_add_light(Draw3DScene* scene, Draw3DLight light) {
    if (scene->lightCount < 8) {
        scene->lights[scene->lightCount] = light;
        scene->lightCount++;
    }
}

//--- Fonctions pour définir le matériau courant ---
void draw3d_set_material(Draw3DScene* scene, Draw3DMaterial material) {
    scene->material = material;
}

//--- Fonctions pour définir la matrice de vue ---
void draw3d_set_view_matrix(Draw3DScene* scene, Draw3DMatrix matrix) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            scene->viewMatrix.m[i][j] = matrix.m[i][j];
        }
    }
}

//--- Fonctions pour définir la matrice de projection ---
void draw3d_set_projection_matrix(Draw3DScene* scene, Draw3DMatrix matrix) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            scene->projMatrix.m[i][j] = matrix.m[i][j];
        }
    }
}

//--- Fonctions pour dessiner une primitive 3D ---
void draw3d_draw_primitive(prim_t* prim, color_t* colors, xyz_t* vertices, texel_t* texcoords, u32 vertexCount, Draw3DScene* scene) {
    // Appliquer les transformations et dessiner la primitive
    // Sur Wii, cela pourrait utiliser des fonctions GX pour le rendu 3D
}

//--- Fonctions pour dessiner un triangle texturé ---
void draw3d_draw_textured_triangle(xyz_t* vertices, texel_t* texcoords, color_t* colors, Draw3DScene* scene) {
    // Dessiner un triangle avec des coordonnées de texture
    // Utiliser les fonctions de rendu bas niveau de la Wii (GX)
}

//--- Fonctions pour dessiner une ligne 3D ---
void draw3d_draw_line(xyz_t* start, xyz_t* end, color_t* color, Draw3DScene* scene) {
    // Dessiner une ligne en 3D
}

//--- Fonctions pour dessiner un point 3D ---
void draw3d_draw_point(xyz_t* point, color_t* color, Draw3DScene* scene) {
    // Dessiner un point en 3D
}

//--- Fonctions pour appliquer une transformation à un sommet ---
void draw3d_transform_vertex(xyz_t* vertex, Draw3DMatrix* matrix) {
    // Appliquer une transformation à un sommet 3D
    float x = vertex->x * matrix->m[0][0] + vertex->y * matrix->m[1][0] + vertex->z * matrix->m[2][0] + matrix->m[3][0];
    float y = vertex->x * matrix->m[0][1] + vertex->y * matrix->m[1][1] + vertex->z * matrix->m[2][1] + matrix->m[3][1];
    float z = vertex->x * matrix->m[0][2] + vertex->y * matrix->m[1][2] + vertex->z * matrix->m[2][2] + matrix->m[3][2];
    float w = vertex->x * matrix->m[0][3] + vertex->y * matrix->m[1][3] + vertex->z * matrix->m[2][3] + matrix->m[3][3];

    if (w != 0.0f) {
        vertex->x = x / w;
        vertex->y = y / w;
        vertex->z = z / w;
    }
}

//--- Fonctions pour calculer l'éclairage d'un sommet ---
void draw3d_calculate_lighting(xyz_t* vertex, color_t* color, Draw3DScene* scene) {
    // Calculer l'éclairage pour un sommet en utilisant les lumières de la scène
    // Appliquer le matériau courant
}

#endif // DRAW3D_H
