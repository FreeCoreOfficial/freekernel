#pragma once
#include "../../video/surface.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct window {
  uint32_t id;
  surface_t *surface;

  int x, y;
  int w, h;
  int z;

  uint32_t flags;
  void *userdata;

  uint8_t state;
  int restore_x;
  int restore_y;
  int restore_w;
  int restore_h;

  void (*on_close)(struct window *win);
  void (*on_resize)(struct window *win);
  char title[32];

  struct window *next;
} window_t;

enum { WIN_STATE_NORMAL = 0, WIN_STATE_MAXIMIZED = 1, WIN_STATE_MINIMIZED = 2 };

enum {
  WIN_FLAG_NO_DECOR = 1 << 0,
  WIN_FLAG_NO_RESIZE = 1 << 1,
  WIN_FLAG_STAY_BOTTOM = 1 << 2,
  WIN_FLAG_NO_DRAG = 1 << 3,
  WIN_FLAG_NO_MAXIMIZE = 1 << 4
};

window_t *window_create(surface_t *surface, int x, int y);
void window_destroy(window_t *win);

#ifdef __cplusplus
}
#endif
