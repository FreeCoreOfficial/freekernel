#include "image_viewer_app.h"
#include "../cmds/fat.h"
#include "../string.h"
#include "../ui/flyui/bmp.h"
#include "../ui/flyui/draw.h"
#include "../ui/wm/wm.h"

static window_t *img_win = NULL;
static surface_t *cached_img = NULL;
static char img_path[256] = "";

static void img_close(window_t *win) {
  (void)win;
  if (img_win) {
    wm_destroy_window(img_win);
    img_win = NULL;
  }
  if (cached_img) {
    surface_destroy(cached_img);
    cached_img = NULL;
  }
  wm_mark_dirty();
}

static void draw_content(surface_t *s) {
  surface_clear(s, 0xFF000000);
  if (cached_img) {
    /* Draw 1:1 at the top-left */
    fly_draw_surface_scaled(s, cached_img, 0, 0, cached_img->width,
                            cached_img->height);
  } else {
    fly_draw_text(s, 10, 40, "No image loaded.", 0xFFFFFFFF);
  }
}

void image_viewer_app_create(const char *path) {
  if (path) {
    if (strcmp(path, img_path) != 0 || cached_img == NULL) {
      strncpy(img_path, path, sizeof(img_path));
      img_path[sizeof(img_path) - 1] = 0;

      if (cached_img) {
        surface_destroy(cached_img);
        cached_img = NULL;
      }

      fat_automount();
      cached_img = fly_load_bmp(path);
    }
  }

  /* If window already exists, we might need to recreate it if size changed,
     but usually we just update the content if path didn't change.
     Actually, if path changed, we should probably recreate window to match new
     size. */
  if (img_win) {
    /* If we are here and path changed, let's just close and reopen for
     * simplicity of size matching */
    if (path) {
      img_close(img_win);
    } else {
      wm_restore_window(img_win);
      wm_focus_window(img_win);
      return;
    }
  }

  uint32_t w = 400;
  uint32_t h = 300;
  if (cached_img) {
    w = cached_img->width;
    h = cached_img->height;
    /* Limit to screen-friendly sizes to avoid OOM or lag */
    if (w > 1024)
      w = 1024;
    if (h > 768)
      h = 768;
    if (w < 100)
      w = 100;
    if (h < 50)
      h = 50;
  }

  surface_t *s = surface_create(w, h);
  draw_content(s);

  img_win = wm_create_window(s, 100, 100);
  if (img_win) {
    wm_set_title(img_win, "Image Viewer");
    wm_set_on_close(img_win, img_close);
    /* Disable resize and maximize as requested */
    wm_set_window_flags(img_win, WIN_FLAG_NO_RESIZE | WIN_FLAG_NO_MAXIMIZE);
  }
}

bool image_viewer_app_handle_event(input_event_t *ev) {
  if (!img_win)
    return false;
  if (ev->type == INPUT_MOUSE_CLICK && ev->pressed) {
    int lx = ev->mouse_x - img_win->x;
    int ly = ev->mouse_y - img_win->y;
    if (lx >= (int)img_win->w - 20 && ly < 24) {
      img_close(img_win);
      return true;
    }
  }
  return false;
}

window_t *image_viewer_app_get_window(void) { return img_win; }
