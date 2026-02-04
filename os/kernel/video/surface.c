#include "surface.h"
#include "../mem/kmalloc.h"

/* Import serial logging */
extern void serial(const char *fmt, ...);

surface_t* surface_create(uint32_t width, uint32_t height) {
    surface_t* surf = (surface_t*)kmalloc(sizeof(surface_t));
    if (!surf) return 0;

    surf->width = width;
    surf->height = height;
    surf->x = 0;
    surf->y = 0;
    surf->visible = true;
    
    /* Allocate pixel buffer (32bpp) */
    surf->pixels = (uint32_t*)kmalloc(width * height * sizeof(uint32_t));
    if (!surf->pixels) {
        kfree(surf);
        return 0;
    }

    /* Initialize to transparent/black */
    for (uint32_t i = 0; i < width * height; i++) {
        surf->pixels[i] = 0xFF000000; // Opaque black default
    }

    serial("[SURFACE] Created %dx%d surface at 0x%x\n", width, height, surf);
    return surf;
}

void surface_destroy(surface_t* surface) {
    if (!surface) return;
    if (surface->pixels) kfree(surface->pixels);
    kfree(surface);
    serial("[SURFACE] Destroyed surface 0x%x\n", surface);
}

void surface_clear(surface_t* surface, uint32_t color) {
    if (!surface || !surface->pixels) return;
    uint32_t count = surface->width * surface->height;
    for (uint32_t i = 0; i < count; i++) {
        surface->pixels[i] = color;
    }
}

void surface_put_pixel(surface_t* surface, uint32_t x, uint32_t y, uint32_t color) {
    if (!surface || !surface->pixels) return;
    if (x >= surface->width || y >= surface->height) return;
    
    /* Row-major: y * width + x */
    surface->pixels[y * surface->width + x] = color;
}

bool surface_resize(surface_t* surface, uint32_t width, uint32_t height, uint32_t fill_color) {
    if (!surface) return false;
    if (width == 0 || height == 0) return false;
    if (width == surface->width && height == surface->height) return true;

    uint32_t* new_pixels = (uint32_t*)kmalloc(width * height * sizeof(uint32_t));
    if (!new_pixels) return false;

    /* Fill */
    uint32_t count = width * height;
    for (uint32_t i = 0; i < count; i++) {
        new_pixels[i] = fill_color;
    }

    /* Copy old into new (min dims) */
    uint32_t min_w = (width < surface->width) ? width : surface->width;
    uint32_t min_h = (height < surface->height) ? height : surface->height;
    for (uint32_t y = 0; y < min_h; y++) {
        uint32_t* dst = new_pixels + (y * width);
        uint32_t* src = surface->pixels + (y * surface->width);
        for (uint32_t x = 0; x < min_w; x++) {
            dst[x] = src[x];
        }
    }

    if (surface->pixels) kfree(surface->pixels);
    surface->pixels = new_pixels;
    surface->width = width;
    surface->height = height;
    return true;
}
