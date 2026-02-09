/* kernel/arch/i386/syscall_dispatch.c */
#include "../../include/chrysalis/syscall_nums.h"
#include "../../input/input.h"
#include "../../terminal.h"
#include "../../ui/flyui/draw.h"
#include "../../ui/wm/wm.h"
#include <stdint.h>

extern "C" void schedule();

#ifdef __cplusplus
extern "C" {
#endif

/* === helper intern === */
static int sys_write(const char *s) {
  if (!s) {
    terminal_writestring("[sys_write] null pointer\n");
    return -1;
  }
  terminal_writestring(s);
  return 0;
}

int syscall_dispatch(uint32_t num, uint32_t a1, uint32_t a2, uint32_t a3,
                     uint32_t a4, uint32_t a5) {
  switch (num) {
  case SYS_WRITE:
    return sys_write((const char *)(uintptr_t)a1);

  case SYS_EXIT:
    terminal_printf("[syscall] process exit code=%d\n", a1);
    schedule();
    return 0;

  case SYS_WM_CREATE_WINDOW: {
    surface_t *s = surface_create(a1, a2);
    if (!s)
      return 0;
    window_t *win = wm_create_window(s, a3, a4);
    if (win && a5) {
      wm_set_title(win, (const char *)(uintptr_t)a5);
    }
    return (uint32_t)(uintptr_t)win;
  }

  case SYS_WM_DESTROY_WINDOW:
    wm_destroy_window((window_t *)(uintptr_t)a1);
    return 0;

  case SYS_WM_MARK_DIRTY:
    wm_mark_dirty();
    return 0;

  case SYS_WM_GET_POS: {
    window_t *win = (window_t *)(uintptr_t)a1;
    if (win)
      return (uint32_t)((win->x << 16) | (win->y & 0xFFFF));
    return 0;
  }

  case SYS_GET_EVENT: {
    input_event_t *out_ev = (input_event_t *)(uintptr_t)a1;
    if (input_pop(out_ev))
      return 1;
    return 0;
  }

  case SYS_FLY_DRAW_TEXT: {
    window_t *win = (window_t *)(uintptr_t)a1;
    if (win && win->surface) {
      fly_draw_text(win->surface, a2, a3, (const char *)(uintptr_t)a4, a5);
    }
    return 0;
  }

  case SYS_FLY_DRAW_RECT_FILL: {
    window_t *win = (window_t *)(uintptr_t)a1;
    if (win && win->surface) {
      fly_draw_rect_fill(win->surface, a2, a3, a4, (uint16_t)(a5 & 0xFFFF),
                         (uint32_t)(a5 >> 16));
    }
    return 0;
  }

  default:
    terminal_printf("[syscall] invalid syscall %d\n", num);
    return -1;
  }
}

#ifdef __cplusplus
}
#endif
