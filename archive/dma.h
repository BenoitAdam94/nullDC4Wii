#ifndef DMA_H
#define DMA_H

#include <cstdint>
#include "tamtypes.h"  // Pour les types u8, u16, u32, etc.

//--- Définitions des canaux DMA (simulés pour la Dreamcast) ---
typedef enum {
    DMA_CHANNEL_GIF = 0,  // Canal pour les commandes graphiques (équivalent PS2, mais adapté pour PVR)
    DMA_CHANNEL_VIF,      // Canal pour les données de sommets (non utilisé sur Dreamcast, mais gardé pour compatibilité)
    DMA_CHANNEL_MAX       // Nombre total de canaux
} DMACHANNEL;

//--- Structure pour un paquet DMA (similaire à la PS2, mais adapté pour PVR) ---
typedef struct {
    u32* data;            // Pointeur vers les données à transférer
    u32 size;             // Taille des données (en octets)
    u32 address;          // Adresse de destination (dans la VRAM ou la mémoire GPU)
    u32 flags;            // Drapeaux pour le transfert (ex: synchronisation)
} DMAPacket;

//--- Fonctions pour initialiser les canaux DMA ---
void dma_channel_initialize(DMACHANNEL channel, void* buffer, u32 size) {
    // Sur Dreamcast/Wii, cette fonction peut être une simple initialisation de buffer.
    // Pas de matériel DMA dédié comme sur PS2, donc on simule.
}

//--- Fonctions pour envoyer des données via DMA ---
void dma_channel_send_normal(DMACHANNEL channel, u32* data, u32 size, u32 address, u32 flags) {
    // Sur Dreamcast, les transferts vers le PVR sont souvent gérés par des fonctions comme _vmem_write.
    // Sur Wii, on peut utiliser des fonctions comme GX ou des copies mémoire directes.

    // Exemple : Copier les données vers la VRAM (simulé)
    u8* dest = (u8*)address;
    memcpy(dest, data, size);
}

void dma_channel_fast_waits(DMACHANNEL channel) {
    // Sur Dreamcast/Wii, cette fonction peut être une simple synchronisation.
    // Pas de matériel DMA dédié, donc on attend juste que la copie soit terminée.
}

//--- Fonctions pour attendre la fin d'un transfert DMA ---
void dma_wait_fast() {
    // Sur Dreamcast/Wii, cette fonction peut être vide ou simplement une barrière mémoire.
    // Le PVR et la Wii n'ont pas de DMA matériel comme la PS2.
}

//--- Fonctions utilitaires pour gérer les paquets DMA ---
DMAPacket* dma_packet_init(u32 size, u32 type) {
    // Allouer un paquet DMA (simulé)
    DMAPacket* packet = new DMAPacket();
    packet->data = new u32[size / sizeof(u32)];
    packet->size = size;
    packet->address = 0;  // À définir par l'appelant
    packet->flags = type;
    return packet;
}

void packet_free(DMAPacket* packet) {
    // Libérer la mémoire d'un paquet DMA
    if (packet != nullptr) {
        delete[] packet->data;
        delete packet;
    }
}

//--- Définitions pour la compatibilité avec le code PS2 ---
#define PACKET_NORMAL 0  // Type de paquet normal

#endif // DMA_H
