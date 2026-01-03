#pragma once
#include <stdint.h>
#include <stddef.h>

#define BUDDY_MAX_ORDER 10 /* ajustabil: ex. 2^10 * 4KB = 4MB */
void buddy_init(uint32_t total_frames);
void* buddy_alloc_page(int order); /* returnează adresă fizică (page aligned) */
void buddy_free_page(void* phys, int order);
