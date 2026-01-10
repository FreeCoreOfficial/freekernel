#include "paint_app.h"
#include "../ui/wm/wm.h"
#include "../ui/flyui/draw.h"
#include "app_manager.h"

static window_t* paint_win = NULL;
static uint32_t brush_color = 0xFF000000;

void paint_app_create(void) {
    if (paint_win) return;
    surface_t* s = surface_create(400, 300);
    surface_clear(s, 0xFFFFFFFF);

    fly_draw_rect_fill(s, 0, 0, 400, 24, 0xFF800080);
    fly_draw_text(s, 5, 4, "Paint", 0xFFFFFFFF);
    fly_draw_rect_fill(s, 380, 4, 16, 16, 0xFFC0C0C0);
    fly_draw_text(s, 384, 4, "X", 0xFF000000);

    /* Palette */
    fly_draw_rect_fill(s, 0, 270, 400, 30, 0xFFC0C0C0);
    fly_draw_rect_fill(s, 10, 275, 20, 20, 0xFF000000); /* Black */
    fly_draw_rect_fill(s, 40, 275, 20, 20, 0xFFFF0000); /* Red */
    fly_draw_rect_fill(s, 70, 275, 20, 20, 0xFF00FF00); /* Green */
    fly_draw_rect_fill(s, 100, 275, 20, 20, 0xFF0000FF); /* Blue */
    fly_draw_rect_fill(s, 130, 275, 20, 20, 0xFFFFFFFF); /* White (Eraser) */
    fly_draw_rect_outline(s, 130, 275, 20, 20, 0xFF000000);

    /* Clear Button */
    fly_draw_rect_fill(s, 340, 275, 50, 20, 0xFFC0C0C0);
    fly_draw_text(s, 345, 278, "Clear", 0xFF000000);

    paint_win = wm_create_window(s, 50, 50);
    app_register("Paint", paint_win);
}

bool paint_app_handle_event(input_event_t* ev) {
    if (!paint_win) return false;
    
    int lx = ev->mouse_x - paint_win->x;
    int ly = ev->mouse_y - paint_win->y;

    if (ev->type == INPUT_MOUSE_CLICK && ev->pressed) {
        if (lx >= 380 && ly < 24) {
            wm_destroy_window(paint_win);
            app_unregister(paint_win);
            paint_win = NULL;
            wm_mark_dirty();
            return true;
        }
        /* Palette Pick */
        if (ly >= 275) {
            if (lx >= 10 && lx <= 30) brush_color = 0xFF000000;
            else if (lx >= 40 && lx <= 60) brush_color = 0xFFFF0000;
            else if (lx >= 70 && lx <= 90) brush_color = 0xFF00FF00;
            else if (lx >= 100 && lx <= 120) brush_color = 0xFF0000FF;
            else if (lx >= 130 && lx <= 150) brush_color = 0xFFFFFFFF;
            
            /* Clear Button */
            if (lx >= 340 && lx <= 390) {
                fly_draw_rect_fill(paint_win->surface, 0, 24, 400, 246, 0xFFFFFFFF);
                wm_mark_dirty();
            }
        }
    } else if (ev->type == INPUT_MOUSE_MOVE && ev->pressed) {
        if (ly > 24 && ly < 270) {
            /* Draw 2x2 pixel */
            surface_put_pixel(paint_win->surface, lx, ly, brush_color);
            surface_put_pixel(paint_win->surface, lx+1, ly, brush_color);
            surface_put_pixel(paint_win->surface, lx, ly+1, brush_color);
            surface_put_pixel(paint_win->surface, lx+1, ly+1, brush_color);
            wm_mark_dirty();
        }
    }
    return false;
}

window_t* paint_app_get_window(void) { return paint_win; }