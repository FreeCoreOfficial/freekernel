#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BUDDY_MAX_ORDER 10

void buddy_init(uint32_t total_frames);
void* buddy_alloc_page(int order);
void  buddy_free_page(void* phys, int order);

#ifdef __cplusplus
}
#endif
