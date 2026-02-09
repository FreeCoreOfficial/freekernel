/* user/libpetal/src/libpetal.c */
#include "../include/petal.h"

static inline int syscall0(int num) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(num));
  return ret;
}

static inline int syscall1(int num, uint32_t a1) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(a1));
  return ret;
}

static inline int syscall2(int num, uint32_t a1, uint32_t a2) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(a1), "c"(a2));
  return ret;
}

static inline int syscall5(int num, uint32_t a1, uint32_t a2, uint32_t a3,
                           uint32_t a4, uint32_t a5) {
  int ret;
  asm volatile("int $0x80"
               : "=a"(ret)
               : "a"(num), "b"(a1), "c"(a2), "d"(a3), "S"(a4), "D"(a5));
  return ret;
}

void p_exit(int code) { syscall1(SYS_EXIT, (uint32_t)code); }

void p_write(const char *s) { syscall1(SYS_WRITE, (uint32_t)(uintptr_t)s); }

void *p_wm_create_window(int w, int h, int x, int y, const char *title) {
  return (void *)(uintptr_t)syscall5(SYS_WM_CREATE_WINDOW, (uint32_t)w,
                                     (uint32_t)h, (uint32_t)x, (uint32_t)y,
                                     (uint32_t)(uintptr_t)title);
}

void p_wm_destroy_window(void *win) {
  syscall1(SYS_WM_DESTROY_WINDOW, (uint32_t)(uintptr_t)win);
}

void p_wm_mark_dirty() { syscall0(SYS_WM_MARK_DIRTY); }

void p_wm_get_pos(void *win, int *x, int *y) {
  uint32_t ret = (uint32_t)syscall1(SYS_WM_GET_POS, (uint32_t)(uintptr_t)win);
  if (x)
    *x = (int)(ret >> 16);
  if (y)
    *y = (int)(ret & 0xFFFF);
}

void p_draw_rect_fill(void *win, int x, int y, int w, int h, uint32_t color) {
  uint32_t packed_h_color = (color << 16) | (h & 0xFFFF);
  syscall5(SYS_FLY_DRAW_RECT_FILL, (uint32_t)(uintptr_t)win, (uint32_t)x,
           (uint32_t)y, (uint32_t)w, packed_h_color);
}

void p_draw_text(void *win, int x, int y, const char *text, uint32_t color) {
  syscall5(SYS_FLY_DRAW_TEXT, (uint32_t)(uintptr_t)win, (uint32_t)x,
           (uint32_t)y, (uint32_t)(uintptr_t)text, color);
}

int p_get_event(p_input_event_t *ev) {
  return syscall1(SYS_GET_EVENT, (uint32_t)(uintptr_t)ev);
}
