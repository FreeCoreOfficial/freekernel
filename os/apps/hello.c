/* apps/hello.c */
#include "../user/libpetal/include/petal.h"

void _start() {
  void *win = p_wm_create_window(240, 120, 300, 200, "Petal Demo");
  if (win) {
    p_draw_rect_fill(win, 10, 30, 220, 80, 0xFFFFFFFF);
    p_draw_text(win, 20, 40, "Standalone Hello!", 0xFF000000);
    p_draw_text(win, 20, 60, "Click me or type...", 0xFF444444);
    p_wm_mark_dirty();

    p_input_event_t ev;
    while (1) {
      if (p_get_event(&ev)) {
        if (ev.type == P_INPUT_MOUSE_CLICK && ev.pressed) {
          p_draw_rect_fill(win, 10, 30, 220, 80, 0xFF00FF00); /* Flash green */
          p_draw_text(win, 20, 40, "Clicked!", 0xFF000000);
          p_wm_mark_dirty();
        } else if (ev.type == P_INPUT_KEYBOARD && ev.pressed) {
          if (ev.keycode == 'q')
            break;
          p_draw_text(win, 20, 80, "Key pressed!", 0xFF0000FF);
          p_wm_mark_dirty();
        }
      }
    }
  }
  p_write("Petal app exiting...\n");
  p_exit(0);
}
