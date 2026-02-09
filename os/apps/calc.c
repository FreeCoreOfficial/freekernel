/* apps/calc.c */
#include "../user/libpetal/include/petal.h"

#define CALC_W 200
#define CALC_H 260
#define BTN_SIZE 40
#define GAP 5
#define DISP_H 40

static char display_buf[32] = "0";
static int32_t current_val = 0;
static int32_t stored_val = 0;
static char pending_op = 0;
static int new_entry = 1;

/* itoa helper */
void itoa_simple(int32_t n, char *s) {
  if (n == 0) {
    s[0] = '0';
    s[1] = '\0';
    return;
  }
  int i = 0;
  int sign = n;
  if (n < 0)
    n = -n;
  while (n > 0) {
    s[i++] = (n % 10) + '0';
    n /= 10;
  }
  if (sign < 0)
    s[i++] = '-';
  s[i] = '\0';
  /* reverse */
  for (int j = 0; j < i / 2; j++) {
    char temp = s[j];
    s[j] = s[i - j - 1];
    s[i - j - 1] = temp;
  }
}

static void draw_calc_ui(void *win) {
  /* Background */
  p_draw_rect_fill(win, 0, 0, CALC_W, CALC_H, 0xFFE0E0E0);

  /* Display */
  p_draw_rect_fill(win, GAP, 30, CALC_W - 2 * GAP, DISP_H, 0xFFFFFFFF);
  // Draw text in display
  p_draw_text(win, GAP + 5, 30 + 12, display_buf, 0xFF000000);

  /* Buttons */
  const char *labels[] = {"7", "8", "9", "/", "4", "5", "6", "*",
                          "1", "2", "3", "-", "C", "0", "=", "+"};

  int start_y = 80;
  for (int i = 0; i < 16; i++) {
    int row = i / 4;
    int col = i % 4;
    int x = GAP + col * (BTN_SIZE + GAP);
    int y = start_y + row * (BTN_SIZE + GAP);

    uint32_t color = (col == 3 || i == 12 || i == 14) ? 0xFFC0C0FF : 0xFFFFFFFF;
    p_draw_rect_fill(win, x, y, BTN_SIZE, BTN_SIZE, color);
    p_draw_text(win, x + 15, y + 12, labels[i], 0xFF000000);
  }
}

static void calc_logic(const char *input) {
  if (input[0] >= '0' && input[0] <= '9') {
    if (new_entry) {
      display_buf[0] = input[0];
      display_buf[1] = '\0';
      new_entry = 0;
    } else {
      int len = 0;
      while (display_buf[len])
        len++;
      if (len < 10) {
        display_buf[len] = input[0];
        display_buf[len + 1] = '\0';
      }
    }
    /* Update current val */
    int32_t v = 0;
    for (int i = 0; display_buf[i]; i++)
      v = v * 10 + (display_buf[i] - '0');
    current_val = v;
  } else if (input[0] == 'C') {
    current_val = 0;
    stored_val = 0;
    pending_op = 0;
    display_buf[0] = '0';
    display_buf[1] = '\0';
    new_entry = 1;
  } else if (input[0] == '+' || input[0] == '-' || input[0] == '*' ||
             input[0] == '/') {
    stored_val = current_val;
    pending_op = input[0];
    new_entry = 1;
  } else if (input[0] == '=') {
    if (pending_op) {
      if (pending_op == '+')
        current_val = stored_val + current_val;
      if (pending_op == '-')
        current_val = stored_val - current_val;
      if (pending_op == '*')
        current_val = stored_val * current_val;
      if (pending_op == '/') {
        if (current_val != 0)
          current_val = stored_val / current_val;
        else
          current_val = 0;
      }
      itoa_simple(current_val, display_buf);
      pending_op = 0;
      new_entry = 1;
    }
  }
}

void _start() {
  void *win = p_wm_create_window(CALC_W, CALC_H, 200, 200, "Standalone - Calculator");
  if (!win)
    p_exit(1);

  draw_calc_ui(win);
  p_wm_mark_dirty();

  p_input_event_t ev;
  while (1) {
    if (p_get_event(&ev)) {
      if (ev.type == P_INPUT_MOUSE_CLICK && ev.pressed) {
        int lx = ev.mouse_x - 200; // Hardcoded window X for now
        int ly = ev.mouse_y - 200; // Hardcoded window Y for now

        const char *labels[] = {"7", "8", "9", "/", "4", "5", "6", "*",
                                "1", "2", "3", "-", "C", "0", "=", "+"};

        int start_y = 80;
        for (int i = 0; i < 16; i++) {
          int row = i / 4;
          int col = i % 4;
          int bx = GAP + col * (BTN_SIZE + GAP);
          int by = start_y + row * (BTN_SIZE + GAP);

          if (lx >= bx && lx < bx + BTN_SIZE && ly >= by &&
              ly < by + BTN_SIZE) {
            calc_logic(labels[i]);
            draw_calc_ui(win);
            p_wm_mark_dirty();
            break;
          }
        }
      }
    }
  }
}
