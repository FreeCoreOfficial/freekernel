#include "calculator_app.h"
#include "../ui/wm/wm.h"
#include "../video/surface.h"
#include "../ui/flyui/draw.h"
#include "../mem/kmalloc.h"
#include "../string.h"
#include "../drivers/serial.h"

extern "C" void serial(const char *fmt, ...);
extern "C" char* itoa_dec(char* out, int32_t v);

static window_t* calc_win = NULL;

#define CALC_W 200
#define CALC_H 260
#define BTN_SIZE 40
#define GAP 5
#define DISP_H 40

static char display_buf[32] = "0";
static int32_t current_val = 0;
static int32_t stored_val = 0;
static char pending_op = 0;
static bool new_entry = true;

static void draw_button(surface_t* s, int x, int y, const char* label, uint32_t color) {
    fly_draw_rect_fill(s, x, y, BTN_SIZE, BTN_SIZE, color);
    fly_draw_rect_outline(s, x, y, BTN_SIZE, BTN_SIZE, 0xFF000000);
    
    int text_len = strlen(label);
    int tx = x + (BTN_SIZE - text_len * 8) / 2;
    int ty = y + (BTN_SIZE - 16) / 2;
    fly_draw_text(s, tx, ty, label, 0xFF000000);
}

static void draw_calc_ui(surface_t* s) {
    /* Background */
    fly_draw_rect_fill(s, 0, 0, CALC_W, CALC_H, 0xFFE0E0E0);
    
    /* Title Bar */
    fly_draw_rect_fill(s, 0, 0, CALC_W, 24, 0xFF000080);
    fly_draw_text(s, 5, 4, "Calculator", 0xFFFFFFFF);
    
    /* Close Button */
    fly_draw_rect_fill(s, CALC_W - 20, 4, 16, 16, 0xFFC0C0C0);
    fly_draw_text(s, CALC_W - 16, 4, "X", 0xFF000000);

    /* Display */
    fly_draw_rect_fill(s, GAP, 30, CALC_W - 2*GAP, DISP_H, 0xFFFFFFFF);
    fly_draw_rect_outline(s, GAP, 30, CALC_W - 2*GAP, DISP_H, 0xFF000000);
    
    int text_len = strlen(display_buf);
    int tx = (CALC_W - GAP) - (text_len * 8) - 5;
    fly_draw_text(s, tx, 30 + (DISP_H - 16)/2, display_buf, 0xFF000000);

    /* Buttons */
    const char* labels[] = {
        "7", "8", "9", "/",
        "4", "5", "6", "*",
        "1", "2", "3", "-",
        "C", "0", "=", "+"
    };

    int start_y = 80;
    for (int i = 0; i < 16; i++) {
        int row = i / 4;
        int col = i % 4;
        int x = GAP + col * (BTN_SIZE + GAP);
        int y = start_y + row * (BTN_SIZE + GAP);
        
        uint32_t color = (col == 3 || i == 12 || i == 14) ? 0xFFC0C0FF : 0xFFFFFFFF;
        draw_button(s, x, y, labels[i], color);
    }
}

static void calc_logic(const char* input) {
    if (input[0] >= '0' && input[0] <= '9') {
        if (new_entry) {
            strcpy(display_buf, input);
            new_entry = false;
        } else {
            if (strlen(display_buf) < 10) {
                strcat(display_buf, input);
            }
        }
        /* Update current val */
        int32_t v = 0;
        char* p = display_buf;
        while(*p) { v = v*10 + (*p - '0'); p++; }
        current_val = v;
    }
    else if (input[0] == 'C') {
        current_val = 0;
        stored_val = 0;
        pending_op = 0;
        strcpy(display_buf, "0");
        new_entry = true;
    }
    else if (input[0] == '+' || input[0] == '-' || input[0] == '*' || input[0] == '/') {
        stored_val = current_val;
        pending_op = input[0];
        new_entry = true;
    }
    else if (input[0] == '=') {
        if (pending_op) {
            if (pending_op == '+') current_val = stored_val + current_val;
            if (pending_op == '-') current_val = stored_val - current_val;
            if (pending_op == '*') current_val = stored_val * current_val;
            if (pending_op == '/') {
                if (current_val != 0) current_val = stored_val / current_val;
                else current_val = 0; /* Div by zero */
            }
            itoa_dec(display_buf, current_val);
            pending_op = 0;
            new_entry = true;
        }
    }
}

void calculator_app_create(void) {
    if (calc_win) return;

    surface_t* s = surface_create(CALC_W, CALC_H);
    if (!s) return;

    calc_win = wm_create_window(s, 200, 200);
    
    /* Reset state */
    strcpy(display_buf, "0");
    current_val = 0;
    stored_val = 0;
    pending_op = 0;
    new_entry = true;

    draw_calc_ui(s);
    serial("[CALC] Started.\n");
}

bool calculator_app_handle_event(input_event_t* ev) {
    if (!calc_win) return false;

    if (ev->type == INPUT_MOUSE_CLICK && ev->pressed) {
        int lx = ev->mouse_x - calc_win->x;
        int ly = ev->mouse_y - calc_win->y;

        /* Close Button */
        if (lx >= CALC_W - 20 && lx <= CALC_W - 4 && ly >= 4 && ly <= 20) {
            wm_destroy_window(calc_win);
            calc_win = NULL;
            wm_mark_dirty();
            return true;
        }

        /* Buttons */
        const char* labels[] = {
            "7", "8", "9", "/",
            "4", "5", "6", "*",
            "1", "2", "3", "-",
            "C", "0", "=", "+"
        };

        int start_y = 80;
        for (int i = 0; i < 16; i++) {
            int row = i / 4;
            int col = i % 4;
            int bx = GAP + col * (BTN_SIZE + GAP);
            int by = start_y + row * (BTN_SIZE + GAP);
            
            if (lx >= bx && lx < bx + BTN_SIZE && ly >= by && ly < by + BTN_SIZE) {
                calc_logic(labels[i]);
                draw_calc_ui(calc_win->surface);
                wm_mark_dirty();
                return true;
            }
        }
    }
    return false;
}

window_t* calculator_app_get_window(void) {
    return calc_win;
}