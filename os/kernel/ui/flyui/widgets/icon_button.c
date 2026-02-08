#include "../../../apps/icons/icons.h"
#include "../../../mem/kmalloc.h"
#include "../draw.h"
#include "../theme.h"
#include "widgets.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct {
  fly_on_click_t on_click;
  bool pressed;
  int icon_id;
  char *text;
} icon_button_data_t;

static uint32_t shade_color(uint32_t c, int delta) {
  int a = (c >> 24) & 0xFF;
  int r = (c >> 16) & 0xFF;
  int g = (c >> 8) & 0xFF;
  int b = c & 0xFF;
  r += delta;
  g += delta;
  b += delta;
  if (r < 0)
    r = 0;
  if (r > 255)
    r = 255;
  if (g < 0)
    g = 0;
  if (g > 255)
    g = 255;
  if (b < 0)
    b = 0;
  if (b > 255)
    b = 255;
  return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) |
         (uint32_t)b;
}

static void icon_button_draw(fly_widget_t *w, surface_t *surf, int x, int y) {
  icon_button_data_t *d = (icon_button_data_t *)w->internal_data;
  if (!d)
    return;

  fly_theme_t *th = theme_get();
  uint32_t base = w->bg_color;
  if (d->pressed)
    base = shade_color(base, -20);

  uint32_t top = shade_color(base, 18);
  uint32_t bot = shade_color(base, -18);
  uint32_t border = th->color_lo_2;

  fly_draw_rect_vgradient(surf, x, y, w->w, w->h, top, bot);
  fly_draw_rect_outline(surf, x, y, w->w, w->h, border);

  const icon_image_t *ic = icon_get(d->icon_id);
  if (ic && ic->pixels) {
    int target_size = w->h - 4;
    if (w->w - 4 < target_size)
      target_size = w->w - 4;

    int ix = x + (w->w - target_size) / 2;
    int iy = y + (w->h - target_size) / 2;

    for (int py = 0; py < target_size; py++) {
      for (int px = 0; px < target_size; px++) {
        int src_x = (px * ic->w) / target_size;
        int src_y = (py * ic->h) / target_size;

        if (src_x >= (int)ic->w)
          src_x = ic->w - 1;
        if (src_y >= (int)ic->h)
          src_y = ic->h - 1;

        uint32_t color = ic->pixels[src_y * ic->w + src_x];
        uint8_t a = (color >> 24) & 0xFF;

        if (a > 128) {
          int screen_x = ix + px;
          int screen_y = iy + py;
          if (screen_x >= 0 && screen_x < (int)surf->width && screen_y >= 0 &&
              screen_y < (int)surf->height) {
            surf->pixels[screen_y * surf->width + screen_x] = color;
          }
        }
      }
    }
  }
}

static bool icon_button_event(fly_widget_t *w, fly_event_t *e) {
  icon_button_data_t *d = (icon_button_data_t *)w->internal_data;
  if (!d)
    return false;

  if (e->type == FLY_EVENT_MOUSE_DOWN) {
    d->pressed = true;
    return true;
  } else if (e->type == FLY_EVENT_MOUSE_UP) {
    bool was_pressed = d->pressed;
    d->pressed = false;
    if (was_pressed && d->on_click) {
      extern void serial(const char *fmt, ...);
      serial("[FLYUI] icon button clicked at (%d,%d)\n", e->mx, e->my);
      d->on_click(w);
    }
    return true;
  }
  return false;
}

fly_widget_t *fly_icon_button_create(int icon_id, const char *text) {
  fly_widget_t *w = fly_widget_create();
  if (!w)
    return NULL;

  icon_button_data_t *d =
      (icon_button_data_t *)kmalloc(sizeof(icon_button_data_t));
  if (d) {
    d->icon_id = icon_id;
    d->pressed = false;
    d->on_click = NULL;
    d->text = NULL;
    if (text) {
      /* Text support can be added later if needed for desktop icons */
    }
    w->internal_data = d;
  }

  w->w = 32;
  w->h = 32;
  w->bg_color = 0xFF253341;
  w->on_draw = icon_button_draw;
  w->on_event = icon_button_event;

  return w;
}
