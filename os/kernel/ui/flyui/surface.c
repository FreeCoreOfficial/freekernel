#include "../../video/surface.h"
#include "draw.h"

/* Helper to clear surface for FlyUI rendering */
void flyui_surface_clear(surface_t* surf, uint32_t color) {
    if (!surf) return;
    /* Use draw rect to fill entire surface */
    fly_draw_rect_fill(surf, 0, 0, surf->width, surf->height, color);
}