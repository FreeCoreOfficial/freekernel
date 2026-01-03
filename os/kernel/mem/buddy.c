#include "buddy.h"
#include <stddef.h>
#include <stdint.h>
#include "../memory/pmm.h" /* folosește/înlocuiește pmm_alloc_frame dacă ai deja */

typedef struct free_block {
    struct free_block* next;
} free_block_t;

static free_block_t* free_lists[BUDDY_MAX_ORDER + 1];
static uint32_t frames_total;
static uint32_t frame_size = 0x1000;

void buddy_init(uint32_t total_frames) {
    frames_total = total_frames;
    for (int i=0;i<=BUDDY_MAX_ORDER;i++) free_lists[i] = NULL;

    /* inițializează ca o singură zonă de ordin max care acoperă toată memoria disponibilă */
    /* presupunem că total_frames este putere de 2 pentru simplitate sau rotunjim */
    int order = 0;
    uint32_t n = 1;
    while ((1u<<order) < total_frames) order++;
    if (order > BUDDY_MAX_ORDER) order = BUDDY_MAX_ORDER;

    /* alloc fizic din pmm (sau folosește memoria statică) */
    for (uint32_t i=0;i<(1u<<order);i++){
        void* addr = (void*)(0x100000 + i*frame_size); /* vezi nota: adaptează la layout */
        free_block_t* b = (free_block_t*)addr;
        b->next = free_lists[order];
        free_lists[order] = b;
    }
}

/* utilitare pentru buddy: calculează buddy address etc. (simplificat pentru exemplu) */
void* buddy_alloc_page(int order) {
    if (order < 0 || order > BUDDY_MAX_ORDER) return NULL;
    for (int o = order; o <= BUDDY_MAX_ORDER; ++o) {
        if (free_lists[o]) {
            free_block_t* blk = free_lists[o];
            free_lists[o] = blk->next;
            /* split dacă trebuie */
            while (o > order) {
                --o;
                void* buddy = (void*)((uintptr_t)blk + (1u<<o) * frame_size);
                free_block_t* nb = (free_block_t*)buddy;
                nb->next = free_lists[o];
                free_lists[o] = nb;
            }
            return (void*)blk;
        }
    }
    return NULL;
}

void buddy_free_page(void* phys, int order) {
    if (!phys) return;
    if (order < 0 || order > BUDDY_MAX_ORDER) return;
    free_block_t* blk = (free_block_t*)phys;
    blk->next = free_lists[order];
    free_lists[order] = blk;
}
