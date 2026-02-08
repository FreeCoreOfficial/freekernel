#include "../../../mem/kmalloc.h"
#include "../../../string.h"
#include "../draw.h"
#include "../theme.h"
#include "widgets.h"

extern void serial(const char *fmt, ...);

typedef struct {
  fly_on_click_t on_click;
  bool pressed;
  char *text;
} button_data_t;

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

static void fly_draw_line(surface_t *surf, int x0, int y0, int x1, int y1,
                          uint32_t color) {
  int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
  int sx = (x0 < x1) ? 1 : -1;
  int dy = (y1 > y0) ? -(y1 - y0) : -(y0 - y1);
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx + dy;
  int e2;

  for (;;) {
    if (x0 >= 0 && x0 < (int)surf->width && y0 >= 0 && y0 < (int)surf->height) {
      surf->pixels[y0 * surf->width + x0] = color;
    }
    if (x0 == x1 && y0 == y1)
      break;
    e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

static void button_draw(fly_widget_t *w, surface_t *surf, int x, int y) {
  button_data_t *d = (button_data_t *)w->internal_data;
  uint32_t bg = w->bg_color;
  uint32_t fg = w->fg_color;
  fly_theme_t *th = theme_get();
  uint32_t border = th->color_lo_2;
  uint32_t edge = th->color_lo_1;

  if (d && d->pressed) {
    /* Slight inset effect */
    x += 1;
    y += 1;
    bg = shade_color(bg, -16);
  }

  uint32_t top = shade_color(bg, 18);
  uint32_t bot = shade_color(bg, -18);
  fly_draw_rect_vgradient(surf, x, y, w->w, w->h, top, bot);
  fly_draw_rect_outline(surf, x, y, w->w, w->h, border);

  /* Soft inner edge */
  fly_draw_line(surf, x + 1, y + 1, x + w->w - 2, y + 1, edge);
  fly_draw_line(surf, x + 1, y + 1, x + 1, y + w->h - 2, edge);

  /* Draw text centered */
  if (d && d->text) {
    int text_w = strlen(d->text) * 8;
    int tx = x + (w->w - text_w) / 2;
    int ty = y + (w->h - 16) / 2;
    fly_draw_text(surf, tx, ty, d->text, fg);
  }
}

static bool button_event(fly_widget_t *w, fly_event_t *e) {
  button_data_t *d = (button_data_t *)w->internal_data;
  if (!d)
    return false;

  if (e->type == FLY_EVENT_MOUSE_DOWN) {
    d->pressed = true;
    serial("[FLYUI] button clicked: %s\n", d ? d->text : "???");
    return true;
  } else if (e->type == FLY_EVENT_MOUSE_UP) {
    bool was_pressed = d->pressed;
    d->pressed = false;
    if (was_pressed && d->on_click) {
      d->on_click(w);
    }
    return true;
  }
  return false;
}

fly_widget_t *fly_button_create(const char *text) {
  fly_widget_t *w = fly_widget_create();
  button_data_t *d = (button_data_t *)kmalloc(sizeof(button_data_t));
  if (d) {
    d->text = (char *)kmalloc(strlen(text) + 1);
    strcpy(d->text, text);
    d->pressed = false;
    w->internal_data = d;
  }
  w->on_draw = button_draw;
  w->on_event = button_event;
  w->bg_color = 0xFFE6EDF5; /* cool button surface */
  w->fg_color = theme_get()->color_text;
  return w;
}

void fly_button_set_callback(fly_widget_t *w, fly_on_click_t on_click) {
  if (w && w->internal_data) {
    button_data_t *d = (button_data_t *)w->internal_data;
    d->on_click = on_click;
  }
}
