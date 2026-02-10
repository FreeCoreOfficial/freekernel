#include "draw.h"
#include "../../video/font8x16.h"
#include "../../video/surface.h"

static void put_pixel(surface_t *surf, int x, int y, uint32_t color) {
  if (x < 0 || y < 0 || x >= (int)surf->width || y >= (int)surf->height)
    return;
  surf->pixels[y * surf->width + x] = color;
}

void fly_draw_rect_fill(surface_t *surf, int x, int y, int w, int h,
                        uint32_t color) {
  if (!surf)
    return;

  /* Clip */
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > (int)surf->width)
    w = (int)surf->width - x;
  if (y + h > (int)surf->height)
    h = (int)surf->height - y;

  if (w <= 0 || h <= 0)
    return;

  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      surf->pixels[(y + j) * surf->width + (x + i)] = color;
    }
  }
}

void fly_draw_rect_outline(surface_t *surf, int x, int y, int w, int h,
                           uint32_t color) {
  /* Top & Bottom */
  fly_draw_rect_fill(surf, x, y, w, 1, color);
  fly_draw_rect_fill(surf, x, y + h - 1, w, 1, color);
  /* Left & Right */
  fly_draw_rect_fill(surf, x, y, 1, h, color);
  fly_draw_rect_fill(surf, x + w - 1, y, 1, h, color);
}

void fly_draw_rect_vgradient(surface_t *surf, int x, int y, int w, int h,
                             uint32_t top, uint32_t bottom) {
  if (!surf)
    return;
  if (h <= 0 || w <= 0)
    return;

  /* Clip */
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > (int)surf->width)
    w = (int)surf->width - x;
  if (y + h > (int)surf->height)
    h = (int)surf->height - y;
  if (w <= 0 || h <= 0)
    return;

  uint8_t a1 = (top >> 24) & 0xFF;
  uint8_t r1 = (top >> 16) & 0xFF;
  uint8_t g1 = (top >> 8) & 0xFF;
  uint8_t b1 = top & 0xFF;

  uint8_t a2 = (bottom >> 24) & 0xFF;
  uint8_t r2 = (bottom >> 16) & 0xFF;
  uint8_t g2 = (bottom >> 8) & 0xFF;
  uint8_t b2 = bottom & 0xFF;

  int denom = (h > 1) ? (h - 1) : 1;

  for (int j = 0; j < h; j++) {
    int t = (j * 255) / denom;
    uint8_t a = (uint8_t)(a1 + ((int)(a2 - a1) * t) / 255);
    uint8_t r = (uint8_t)(r1 + ((int)(r2 - r1) * t) / 255);
    uint8_t g = (uint8_t)(g1 + ((int)(g2 - g1) * t) / 255);
    uint8_t b = (uint8_t)(b1 + ((int)(b2 - b1) * t) / 255);
    uint32_t color =
        ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    for (int i = 0; i < w; i++) {
      surf->pixels[(y + j) * surf->width + (x + i)] = color;
    }
  }
}

static void draw_char(surface_t *surf, int x, int y, char c, uint32_t color) {
  const uint8_t *glyph = &font8x16[(uint8_t)c * 16];
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 8; j++) {
      if (glyph[i] & (1 << (7 - j))) {
        put_pixel(surf, x + j, y + i, color);
      }
    }
  }
}

void fly_draw_text(surface_t *surf, int x, int y, const char *text,
                   uint32_t color) {
  if (!text)
    return;
  int cx = x;
  while (*text) {
    draw_char(surf, cx, y, *text, color);
    cx += 8;
    text++;
  }
}

void fly_draw_surface_scaled(surface_t *dest, surface_t *src, int x, int y,
                             int w, int h) {
  if (!dest || !src || w <= 0 || h <= 0)
    return;

  /* Simple Nearest-Neighbor Scaling */
  for (int dy = 0; dy < h; dy++) {
    int sy = (dy * src->height) / h;
    int dest_y = y + dy;
    if (dest_y < 0 || dest_y >= (int)dest->height)
      continue;

    for (int dx = 0; dx < w; dx++) {
      int sx = (dx * src->width) / w;
      int dest_x = x + dx;
      if (dest_x < 0 || dest_x >= (int)dest->width)
        continue;

      dest->pixels[dest_y * dest->width + dest_x] =
          src->pixels[sy * src->width + sx];
    }
  }
}
