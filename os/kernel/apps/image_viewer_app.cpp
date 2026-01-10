#include "image_viewer_app.h"
#include "../ui/wm/wm.h"
#include "../ui/flyui/draw.h"
#include "../ui/flyui/bmp.h"
#include "../cmds/fat.h"

static window_t* img_win = NULL;

void image_viewer_app_create(const char* path) {
    if (img_win) {
        wm_destroy_window(img_win);
        img_win = NULL;
    }

    surface_t* s = surface_create(400, 300);
    surface_clear(s, 0xFF000000);
    
    /* Title Bar */
    fly_draw_rect_fill(s, 0, 0, 400, 24, 0xFF404040);
    fly_draw_text(s, 5, 4, "Image Viewer", 0xFFFFFFFF);
    fly_draw_rect_fill(s, 380, 4, 16, 16, 0xFFC0C0C0);
    fly_draw_text(s, 384, 4, "X", 0xFF000000);

    if (path) {
        fat_automount();
        fly_load_bmp_to_surface(s, path);
        /* Redraw title bar on top of image */
        fly_draw_rect_fill(s, 0, 0, 400, 24, 0xFF404040);
        fly_draw_text(s, 5, 4, path, 0xFFFFFFFF);
        fly_draw_rect_fill(s, 380, 4, 16, 16, 0xFFC0C0C0);
        fly_draw_text(s, 384, 4, "X", 0xFF000000);
    }

    img_win = wm_create_window(s, 100, 100);
}

bool image_viewer_app_handle_event(input_event_t* ev) {
    if (!img_win) return false;
    if (ev->type == INPUT_MOUSE_CLICK && ev->pressed) {
        int lx = ev->mouse_x - img_win->x;
        int ly = ev->mouse_y - img_win->y;
        if (lx >= img_win->w - 20 && ly < 24) {
            wm_destroy_window(img_win);
            img_win = NULL;
            wm_mark_dirty();
            return true;
        }
    }
    return false;
}

window_t* image_viewer_app_get_window(void) { return img_win; }