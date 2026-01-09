#include "window.h"
#include "../../mem/kmalloc.h"

window_t* window_create(surface_t* surface, int x, int y) {
    window_t* win = (window_t*)kmalloc(sizeof(window_t));
    if (!win) return 0;
    
    win->id = 0; // Assigned by WM
    win->surface = surface;
    win->x = x;
    win->y = y;
    win->w = surface ? surface->width : 0;
    win->h = surface ? surface->height : 0;
    win->z = 0;
    win->flags = 0;
    win->userdata = 0;
    win->next = 0;
    
    return win;
}

void window_destroy(window_t* win) {
    if (win) {
        kfree(win);
    }
}