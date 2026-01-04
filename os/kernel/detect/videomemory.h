// kernel/detect/videomemory.h
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* returns detected video memory in MB */
uint32_t video_detect_mb(uint32_t multiboot_magic, uint32_t multiboot_addr);

/* panics if video memory < minimum */
void video_check_or_panic(uint32_t multiboot_magic, uint32_t multiboot_addr);

#ifdef __cplusplus
}
#endif
