#include "compositor.h"
#include "../drivers/mouse.h"
#include "../mem/kmalloc.h"
#include "../string.h"
#include "framebuffer.h" /* fb_putpixel, fb_clear, fb_get_info */
#include "gpu.h"

/* Import serial logging */
extern void serial(const char *fmt, ...);

#define MAX_RESOLUTION_WIDTH 1280
#define MAX_RESOLUTION_HEIGHT 800
static uint32_t back_buffer[MAX_RESOLUTION_WIDTH * MAX_RESOLUTION_HEIGHT];

void compositor_init(void) { serial("[COMPOSITOR] Initialized.\n"); }

void compositor_render_surfaces(surface_t **surfaces, int count) {
  uint32_t fb_w = 0, fb_h = 0, fb_pitch = 0;
  fb_get_info(&fb_w, &fb_h, &fb_pitch, 0, 0);

  if (fb_w == 0 || fb_h == 0) {
    return;
  }

  if (fb_w > MAX_RESOLUTION_WIDTH)
    fb_w = MAX_RESOLUTION_WIDTH;
  if (fb_h > MAX_RESOLUTION_HEIGHT)
    fb_h = MAX_RESOLUTION_HEIGHT;

  /* Background color: Chrysalis Slate */
  uint32_t bg_color = 0x0012171D;

  gpu_device_t *gpu = gpu_get_primary();
  uint8_t *fb_base = (gpu && gpu->virt_addr) ? (uint8_t *)gpu->virt_addr : 0;

  /* 1. Clear back buffer */
  for (uint32_t i = 0; i < fb_w * fb_h; i++) {
    back_buffer[i] = bg_color;
  }

  /* 2. Render all surfaces to back buffer */
  for (int surf_idx = 0; surf_idx < count; surf_idx++) {
    surface_t *s = surfaces[surf_idx];
    if (!s || !s->visible)
      continue;

    int s_x = s->x;
    int s_y = s->y;
    int s_w = s->width;
    int s_h = s->height;

    for (int y = 0; y < s_h; y++) {
      int screen_y = s_y + y;
      if (screen_y < 0 || screen_y >= (int)fb_h)
        continue;

      for (int x = 0; x < s_w; x++) {
        int screen_x = s_x + x;
        if (screen_x < 0 || screen_x >= (int)fb_w)
          continue;

        uint32_t pixel = s->pixels[y * s_w + x];
        back_buffer[screen_y * fb_w + screen_x] = pixel;
      }
    }
  }

  /* 3. Render mouse cursor on top of back buffer */
  int mx, my;
  mouse_get_coords(&mx, &my);
  const uint8_t *cursor = mouse_get_cursor_bitmap();

  for (int cy = 0; cy < 16; cy++) {
    int screen_y = my + cy;
    if (screen_y < 0 || screen_y >= (int)fb_h)
      continue;

    for (int cx = 0; cx < 16; cx++) {
      int screen_x = mx + cx;
      if (screen_x < 0 || screen_x >= (int)fb_w)
        continue;

      uint8_t type = cursor[cy * 16 + cx];
      if (type == 1) {
        back_buffer[screen_y * fb_w + screen_x] = 0xFF000000; /* Black border */
      } else if (type == 2) {
        back_buffer[screen_y * fb_w + screen_x] = 0xFFFFFFFF; /* White fill */
      }
    }
  }

  /* 4. Copy entire back buffer to framebuffer in one operation */
  if (fb_base) {
    /* Fast path: copy entire frame at once */
    for (uint32_t y = 0; y < fb_h; y++) {
      memcpy(fb_base + (y * gpu->pitch), &back_buffer[y * fb_w], fb_w * 4);
    }
  } else {
    /* Slow path: use putpixel */
    for (uint32_t y = 0; y < fb_h; y++) {
      for (uint32_t x = 0; x < fb_w; x++) {
        fb_putpixel(x, y, back_buffer[y * fb_w + x]);
      }
    }
  }
}
