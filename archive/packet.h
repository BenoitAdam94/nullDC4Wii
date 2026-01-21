#ifndef PACKET_H
#define PACKET_H

#include <cstdint>
#include <cstring>
#include "tamtypes.h"  // Pour les types u8, u16, u32, etc.

//--- Types de base pour les paquets PVR ---
namespace PVR {
    // Types de commandes PVR (basés sur la documentation Dreamcast)
    enum CommandType : u32 {
        CMD_USER_CLIP          = 0x00,  // User clip plane
        CMD_OBJECT_LIST        = 0x01,  // Début d'une liste d'objets
        CMD_POLYGON_HEADER     = 0x02,  // En-tête de polygone
        CMD_VERTEX             = 0x03,  // Données de sommet
        CMD_TEXTURE_HEADER     = 0x04,  // En-tête de texture
        CMD_TEXTURE_DATA       = 0x05,  // Données de texture
        CMD_END_OF_LIST        = 0xFF,  // Fin de la liste de commandes
    };

    // Types de primitives graphiques
    enum PrimitiveType : u32 {
        PRIM_TRIANGLE_LIST     = 0x00,
        PRIM_TRIANGLE_STRIP    = 0x01,
        PRIM_QUAD_LIST         = 0x02,
        PRIM_LINE_LIST         = 0x03,
    };

    // Formats de texture
    enum TextureFormat : u32 {
        TEX_FORMAT_RGB565      = 0x00,
        TEX_FORMAT_ARGB4444    = 0x01,
        TEX_FORMAT_ARGB1555    = 0x02,
        TEX_FORMAT_YUV422      = 0x03,
    };

    //--- Structures de paquets ---
    // En-tête générique pour tous les paquets
    struct PacketHeader {
        u32 command;   // Type de commande (CommandType)
        u32 size;      // Taille totale du paquet (en mots de 32 bits)
    };

    // Paquet pour une liste d'objets
    struct ObjectListPacket {
        PacketHeader header;
        u32 objectCount;  // Nombre d'objets dans la liste
        u32* objectPtrs;  // Pointeurs vers les objets (adresses mémoire)
    };

    // Paquet pour un en-tête de polygone
    struct PolygonHeaderPacket {
        PacketHeader header;
        u32 primitiveType;  // Type de primitive (PrimitiveType)
        u32 vertexCount;    // Nombre de sommets
        u32 textureId;     // ID de la texture associée
        u32 shaderParams;  // Paramètres de shader (optionnel)
    };

    // Paquet pour un sommet
    struct VertexPacket {
        PacketHeader header;
        float x, y, z;      // Coordonnées du sommet
        float u, v;         // Coordonnées de texture
        u32 color;          // Couleur (format ARGB)
    };

    // Paquet pour un en-tête de texture
    struct TextureHeaderPacket {
        PacketHeader header;
        u32 width;           // Largeur de la texture
        u32 height;         // Hauteur de la texture
        TextureFormat format; // Format de la texture
        u32 dataSize;       // Taille des données de texture (en octets)
    };

    // Paquet pour les données de texture
    struct TextureDataPacket {
        PacketHeader header;
        u8* data;           // Données brutes de la texture
    };

    //--- Fonctions utilitaires ---
    // Initialise un paquet générique
    template<typename T>
    T* CreatePacket(u32 command, u32 size) {
        T* packet = new T();
        packet->header.command = command;
        packet->header.size = size;
        return packet;
    }

    // Libère la mémoire d'un paquet
    template<typename T>
    void FreePacket(T* packet) {
        if (packet != nullptr) {
            delete packet;
        }
    }

    //--- Exemple d'utilisation ---
    /*
    void ExampleUsage() {
        // Créer un paquet pour un polygone
        PolygonHeaderPacket* polyPacket = CreatePacket<PolygonHeaderPacket>(CMD_POLYGON_HEADER, sizeof(PolygonHeaderPacket) / 4);
        polyPacket->primitiveType = PRIM_TRIANGLE_LIST;
        polyPacket->vertexCount = 3;
        polyPacket->textureId = 0;

        // Créer un paquet pour un sommet
        VertexPacket* vertexPacket = CreatePacket<VertexPacket>(CMD_VERTEX, sizeof(VertexPacket) / 4);
        vertexPacket->x = 1.0f;
        vertexPacket->y = 2.0f;
        vertexPacket->z = 3.0f;
        vertexPacket->u = 0.5f;
        vertexPacket->v = 0.5f;
        vertexPacket->color = 0xFFFFFFFF;  // Blanc

        // Libérer les paquets après utilisation
        FreePacket(polyPacket);
        FreePacket(vertexPacket);
    }
    */
}

#endif // PACKET_H
