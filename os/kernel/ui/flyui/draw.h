#pragma once
#include "../../video/surface.h"

#ifdef __cplusplus
extern "C" {
#endif

void fly_draw_rect_fill(surface_t* surf, int x, int y, int w, int h, uint32_t color);
void fly_draw_rect_outline(surface_t* surf, int x, int y, int w, int h, uint32_t color);
void fly_draw_rect_vgradient(surface_t* surf, int x, int y, int w, int h, uint32_t top, uint32_t bottom);
void fly_draw_text(surface_t* surf, int x, int y, const char* text, uint32_t color);

#ifdef __cplusplus
}
#endif
