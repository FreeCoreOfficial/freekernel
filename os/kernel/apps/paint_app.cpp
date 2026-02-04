#include "paint_app.h"
#include "../ui/wm/wm.h"
#include "../ui/flyui/draw.h"
#include "app_manager.h"

static window_t* paint_win = NULL;
static uint32_t brush_color = 0xFF000000;
static bool paint_drawing = false;

#define PALETTE_H 30
#define TITLE_H 24

static void draw_paint_ui(surface_t* s) {
    int w = (int)s->width;
    int h = (int)s->height;
    int palette_y = h - PALETTE_H;
    if (palette_y < TITLE_H + 10) palette_y = TITLE_H + 10;

    surface_clear(s, 0xFFFFFFFF);

    /* Palette */
    fly_draw_rect_fill(s, 0, palette_y, w, PALETTE_H, 0xFFC0C0C0);
    fly_draw_rect_fill(s, 10, palette_y + 5, 20, 20, 0xFF000000); /* Black */
    fly_draw_rect_fill(s, 40, palette_y + 5, 20, 20, 0xFFFF0000); /* Red */
    fly_draw_rect_fill(s, 70, palette_y + 5, 20, 20, 0xFF00FF00); /* Green */
    fly_draw_rect_fill(s, 100, palette_y + 5, 20, 20, 0xFF0000FF); /* Blue */
    fly_draw_rect_fill(s, 130, palette_y + 5, 20, 20, 0xFFFFFFFF); /* White (Eraser) */
    fly_draw_rect_outline(s, 130, palette_y + 5, 20, 20, 0xFF000000);

    /* Clear Button */
    int clear_x = w - 60;
    if (clear_x < 160) clear_x = 160;
    fly_draw_rect_fill(s, clear_x, palette_y + 5, 50, 20, 0xFFC0C0C0);
    fly_draw_text(s, clear_x + 5, palette_y + 8, "Clear", 0xFF000000);
}

static void paint_close(window_t* win) {
    (void)win;
    if (paint_win) {
        wm_destroy_window(paint_win);
        app_unregister(paint_win);
        paint_win = NULL;
        wm_mark_dirty();
    }
}

static void paint_on_resize(window_t* win) {
    if (!win || !win->surface) return;
    draw_paint_ui(win->surface);
    wm_mark_dirty();
}

void paint_app_create(void) {
    if (paint_win) {
        wm_restore_window(paint_win);
        wm_focus_window(paint_win);
        return;
    }
    surface_t* s = surface_create(400, 300);
    draw_paint_ui(s);

    paint_win = wm_create_window(s, 50, 50);
    if (paint_win) {
        wm_set_title(paint_win, "Paint");
        wm_set_on_close(paint_win, paint_close);
        wm_set_on_resize(paint_win, paint_on_resize);
        app_register("Paint", paint_win);
    }
}

bool paint_app_handle_event(input_event_t* ev) {
    if (!paint_win) return false;
    
    int lx = ev->mouse_x - paint_win->x;
    int ly = ev->mouse_y - paint_win->y;
    int w = (int)paint_win->surface->width;
    int h = (int)paint_win->surface->height;
    int palette_y = h - PALETTE_H;
    if (palette_y < TITLE_H + 10) palette_y = TITLE_H + 10;

    if (ev->type == INPUT_MOUSE_CLICK && ev->pressed) {
        /* Palette Pick */
        if (ly >= palette_y) {
            if (lx >= 10 && lx <= 30) brush_color = 0xFF000000;
            else if (lx >= 40 && lx <= 60) brush_color = 0xFFFF0000;
            else if (lx >= 70 && lx <= 90) brush_color = 0xFF00FF00;
            else if (lx >= 100 && lx <= 120) brush_color = 0xFF0000FF;
            else if (lx >= 130 && lx <= 150) brush_color = 0xFFFFFFFF;
            
            /* Clear Button */
            int clear_x = w - 60;
            if (clear_x < 160) clear_x = 160;
            if (lx >= clear_x && lx <= clear_x + 50) {
                fly_draw_rect_fill(paint_win->surface, 0, TITLE_H, w, palette_y - TITLE_H, 0xFFFFFFFF);
                wm_mark_dirty();
            }
        }
        if (ly > TITLE_H && ly < palette_y) {
            paint_drawing = true;
        }
    } else if (ev->type == INPUT_MOUSE_CLICK && !ev->pressed) {
        paint_drawing = false;
    } else if (ev->type == INPUT_MOUSE_MOVE && ev->pressed) {
        if (paint_drawing && ly > TITLE_H && ly < palette_y) {
            /* Draw 2x2 pixel */
            surface_put_pixel(paint_win->surface, lx, ly, brush_color);
            surface_put_pixel(paint_win->surface, lx+1, ly, brush_color);
            surface_put_pixel(paint_win->surface, lx, ly+1, brush_color);
            surface_put_pixel(paint_win->surface, lx+1, ly+1, brush_color);
            wm_mark_dirty();
        }
    } else if (ev->type == INPUT_MOUSE_MOVE) {
        if (paint_drawing && ly > TITLE_H && ly < palette_y) {
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
