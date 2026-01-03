#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PAGE_SIZE 4096

void     pmm_init(void* mbi);
uint32_t pmm_alloc_frame(void);
void     pmm_free_frame(uint32_t phys_addr);

uint32_t pmm_total_frames(void);
uint32_t pmm_used_frames(void);

#ifdef __cplusplus
}
#endif
