#include "compositor.h"
#include "framebuffer.h" /* fb_putpixel, fb_clear, fb_get_info */
#include "../mem/kmalloc.h"
#include "../string.h"
#include "gpu.h"

/* Import serial logging */
extern void serial(const char *fmt, ...);

/* Simple static array for surfaces */
#define MAX_SURFACES 32
static surface_t* surfaces[MAX_SURFACES];
static int surface_count = 0;

void compositor_init(void) {
    for (int i = 0; i < MAX_SURFACES; i++) {
        surfaces[i] = 0;
    }
    surface_count = 0;
    serial("[COMPOSITOR] Initialized.\n");
}

void compositor_add_surface(surface_t* surface) {
    if (!surface) return;
    
    /* Check if already added */
    for (int i = 0; i < MAX_SURFACES; i++) {
        if (surfaces[i] == surface) return;
    }

    /* Find empty slot */
    for (int i = 0; i < MAX_SURFACES; i++) {
        if (surfaces[i] == 0) {
            surfaces[i] = surface;
            surface_count++;
            serial("[COMPOSITOR] Added surface 0x%x at index %d\n", surface, i);
            return;
        }
    }
    serial("[COMPOSITOR] Error: Max surfaces reached.\n");
}

void compositor_remove_surface(surface_t* surface) {
    if (!surface) return;
    
    for (int i = 0; i < MAX_SURFACES; i++) {
        if (surfaces[i] == surface) {
            surfaces[i] = 0;
            surface_count--;
            serial("[COMPOSITOR] Removed surface 0x%x from index %d\n", surface, i);
            return;
        }
    }
}

void compositor_render(void) {
    uint32_t fb_w = 0, fb_h = 0, fb_pitch = 0;
    fb_get_info(&fb_w, &fb_h, &fb_pitch, 0, 0);

    if (fb_w == 0 || fb_h == 0) {
        return;
    }

    /* Allocate one scanline buffer (32bpp) */
    uint32_t* scanline = (uint32_t*)kmalloc(fb_w * 4);
    if (!scanline) return;

    /* Background color: Windows 1.0 Teal */
    uint32_t bg_color = 0xFF008080;
    
    gpu_device_t* gpu = gpu_get_primary();
    uint8_t* fb_base = (gpu && gpu->virt_addr) ? (uint8_t*)gpu->virt_addr : 0;

    for (uint32_t y = 0; y < fb_h; y++) {
        /* 1. Clear scanline to background */
        for (uint32_t x = 0; x < fb_w; x++) scanline[x] = bg_color;

        /* 2. Compose surfaces on this line */
        for (int i = 0; i < MAX_SURFACES; i++) {
            surface_t* s = surfaces[i];
            if (!s || !s->visible) continue;

            /* Check if surface intersects this line */
            int s_y = s->y;
            int s_h = s->height;
            if ((int)y >= s_y && (int)y < s_y + s_h) {
                /* Calculate source row */
                int src_y = (int)y - s_y;
                
                /* Calculate horizontal intersection */
                int s_x = s->x;
                int s_w = s->width;
                
                /* Determine intersection with screen width */
                int draw_start_x = (s_x < 0) ? 0 : s_x;
                int draw_end_x = (s_x + s_w > (int)fb_w) ? (int)fb_w : (s_x + s_w);
                
                if (draw_start_x < draw_end_x) {
                    /* Copy pixels */
                    uint32_t* src_row = s->pixels + (src_y * s->width);
                    int src_start_off = draw_start_x - s_x;
                    
                    for (int x = draw_start_x; x < draw_end_x; x++) {
                        /* Simple opaque copy */
                        scanline[x] = src_row[src_start_off + (x - draw_start_x)];
                    }
                }
            }
        }

        /* 3. Blit scanline to Framebuffer */
        if (fb_base) {
            /* Fast path: direct memory copy */
            memcpy(fb_base + (y * gpu->pitch), scanline, fb_w * 4);
        } else {
            /* Slow path: putpixel */
            for (uint32_t x = 0; x < fb_w; x++) {
                fb_putpixel(x, y, scanline[x]);
            }
        }
    }
    
    kfree(scanline);
}