#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Framebuffer API */
void fb_init(uint64_t addr, uint32_t width, uint32_t height, uint32_t pitch, uint8_t bpp, uint8_t type);
void fb_putpixel(uint32_t x, uint32_t y, uint32_t color);
void fb_clear(uint32_t color);
void fb_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

#ifdef __cplusplus
}
#endif
