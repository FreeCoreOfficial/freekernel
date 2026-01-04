/* file: kernel/detect/videomemory.h */
#pragma once
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/* Minimum video capabilities required by Chrysalis */
#define VIDEO_MIN_WIDTH 1024U
#define VIDEO_MIN_HEIGHT 768U
#define VIDEO_MIN_BPP 24U


/* returns detected video memory in MB (best-effort) */
uint32_t video_detect_mb(uint32_t multiboot_magic, uint32_t multiboot_addr);


/* panics if video capabilities are insufficient (width/height/bpp) */
void video_check_or_panic(uint32_t multiboot_magic, uint32_t multiboot_addr);


#ifdef __cplusplus
}
#endif