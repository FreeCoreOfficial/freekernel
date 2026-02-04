#include "image_viewer_app.h"
#include "../ui/wm/wm.h"
#include "../ui/flyui/draw.h"
#include "../ui/flyui/bmp.h"
#include "../cmds/fat.h"
#include "../string.h"

static window_t* img_win = NULL;
static char img_path[128] = "";

static void img_close(window_t* win) {
    (void)win;
    if (img_win) {
        wm_destroy_window(img_win);
        img_win = NULL;
        wm_mark_dirty();
    }
}

static void draw_image(surface_t* s, const char* path) {
    surface_clear(s, 0xFF000000);
    if (path && *path) {
        fat_automount();
        fly_load_bmp_to_surface(s, path);
    }
}

static void img_on_resize(window_t* win) {
    if (!win || !win->surface) return;
    draw_image(win->surface, img_path);
    wm_mark_dirty();
}

void image_viewer_app_create(const char* path) {
    if (path) {
        strncpy(img_path, path, sizeof(img_path));
        img_path[sizeof(img_path) - 1] = 0;
    }

    if (img_win) {
        wm_restore_window(img_win);
        wm_focus_window(img_win);
        draw_image(img_win->surface, img_path);
        return;
    }

    surface_t* s = surface_create(400, 300);
    draw_image(s, img_path);

    img_win = wm_create_window(s, 100, 100);
    if (img_win) {
        wm_set_title(img_win, "Image Viewer");
        wm_set_on_close(img_win, img_close);
        wm_set_on_resize(img_win, img_on_resize);
    }
}

bool image_viewer_app_handle_event(input_event_t* ev) {
    if (!img_win) return false;
    if (ev->type == INPUT_MOUSE_CLICK && ev->pressed) {
        int lx = ev->mouse_x - img_win->x;
        int ly = ev->mouse_y - img_win->y;
        if (lx >= img_win->w - 20 && ly < 24) {
            img_close(img_win);
            return true;
        }
    }
    return false;
}

window_t* image_viewer_app_get_window(void) { return img_win; }
