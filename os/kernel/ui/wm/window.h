#pragma once
#include <stdint.h>
#include "../../video/surface.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct window {
    uint32_t id;
    surface_t* surface;

    int x, y;
    int w, h;
    int z;

    uint32_t flags;
    void* userdata;
    
    struct window* next;
} window_t;

window_t* window_create(surface_t* surface, int x, int y);
void window_destroy(window_t* win);

#ifdef __cplusplus
}
#endif