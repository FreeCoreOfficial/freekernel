#include "../../../mem/kmalloc.h"
#include "widgets.h"
#include <stddef.h>

typedef struct {
  const uint32_t *pixels;
} image_data_t;

static void image_draw(fly_widget_t *w, surface_t *surf, int x, int y) {
  image_data_t *d = (image_data_t *)w->internal_data;
  if (!d || !d->pixels)
    return;

  for (int py = 0; py < w->h; py++) {
    for (int px = 0; px < w->w; px++) {
      int screen_x = x + px;
      int screen_y = y + py;
      if (screen_x >= 0 && screen_x < (int)surf->width && screen_y >= 0 &&
          screen_y < (int)surf->height) {

        uint32_t color = d->pixels[py * w->w + px];
        /* Alpha blending helper (simple constant alpha or ARGB check) */
        uint8_t a = (color >> 24) & 0xFF;
        if (a > 128) {
          surf->pixels[screen_y * surf->width + screen_x] = color;
        }
      }
    }
  }
}

fly_widget_t *fly_image_create(int w, int h, const uint32_t *pixels) {
  fly_widget_t *widget = fly_widget_create();
  if (!widget)
    return NULL;

  image_data_t *d = (image_data_t *)kmalloc(sizeof(image_data_t));
  if (d) {
    d->pixels = pixels;
    widget->internal_data = d;
  }

  widget->w = w;
  widget->h = h;
  widget->on_draw = image_draw;

  return widget;
}

void fly_image_set_data(fly_widget_t *w, const uint32_t *pixels) {
  if (w && w->internal_data) {
    image_data_t *d = (image_data_t *)w->internal_data;
    d->pixels = pixels;
  }
}
