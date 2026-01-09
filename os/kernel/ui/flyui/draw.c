#include "draw.h"
#include "../../video/surface.h"
#include "../../video/font8x16.h"

static void put_pixel(surface_t* surf, int x, int y, uint32_t color) {
    if (x < 0 || y < 0 || x >= (int)surf->width || y >= (int)surf->height) return;
    surf->pixels[y * surf->width + x] = color;
}

void fly_draw_rect_fill(surface_t* surf, int x, int y, int w, int h, uint32_t color) {
    if (!surf) return;
    
    /* Clip */
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)surf->width) w = (int)surf->width - x;
    if (y + h > (int)surf->height) h = (int)surf->height - y;
    
    if (w <= 0 || h <= 0) return;

    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            surf->pixels[(y + j) * surf->width + (x + i)] = color;
        }
    }
}

void fly_draw_rect_outline(surface_t* surf, int x, int y, int w, int h, uint32_t color) {
    /* Top & Bottom */
    fly_draw_rect_fill(surf, x, y, w, 1, color);
    fly_draw_rect_fill(surf, x, y + h - 1, w, 1, color);
    /* Left & Right */
    fly_draw_rect_fill(surf, x, y, 1, h, color);
    fly_draw_rect_fill(surf, x + w - 1, y, 1, h, color);
}

static void draw_char(surface_t* surf, int x, int y, char c, uint32_t color) {
    const uint8_t* glyph = &font8x16[(uint8_t)c * 16];
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 8; j++) {
            if (glyph[i] & (1 << (7-j))) {
                put_pixel(surf, x + j, y + i, color);
            }
        }
    }
}

void fly_draw_text(surface_t* surf, int x, int y, const char* text, uint32_t color) {
    if (!text) return;
    int cx = x;
    while (*text) {
        draw_char(surf, cx, y, *text, color);
        cx += 8;
        text++;
    }
}