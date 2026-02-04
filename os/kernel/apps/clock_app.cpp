#include "clock_app.h"
#include "../ui/wm/wm.h"
#include "../video/surface.h"
#include "../ui/flyui/draw.h"
#include "../time/clock.h"
#include "../drivers/serial.h"
#include "../mem/kmalloc.h"

extern "C" void serial(const char *fmt, ...);

static window_t* clock_win = NULL;
static int last_sec = -1;

static void draw_clock_face(surface_t* s);
static void draw_hands(surface_t* s, int h, int m, int sec);

static void clock_close(window_t* win) {
    (void)win;
    if (clock_win) {
        wm_destroy_window(clock_win);
        clock_win = NULL;
        wm_mark_dirty();
    }
}

static void clock_on_resize(window_t* win) {
    if (!win || !win->surface) return;
    datetime t;
    time_get_local(&t);
    draw_clock_face(win->surface);
    draw_hands(win->surface, t.hour, t.minute, t.second);
    wm_mark_dirty();
}
/* Dimensiuni */
#define WIN_W 200
#define WIN_H 224 /* 200 content + 24 title */
#define CLOCK_R 80
#define CENTER_X (WIN_W / 2)
#define CENTER_Y (24 + (WIN_H - 24) / 2)

/* Culori */
#define COL_BG      0xFFFFFFFF /* Alb */
#define COL_FACE    0xFFEEEEEE /* Gri deschis */
#define COL_BORDER  0xFF000000 /* Negru */
#define COL_SEC     0xFFFF0000 /* Roșu */
#define COL_MIN     0xFF000000
#define COL_HOUR    0xFF000000
#define COL_TITLE   0xFF000080 /* Navy Blue */
#define COL_BTN     0xFFC0C0C0
#define COL_BTN_X   0xFF000000

/* Math Helpers (Fixed point sin/cos approximation for 0..59 steps) */
/* Scaled by 1024 */
/* Angle 0 = 12 o'clock (270 deg / -90 deg). We map step 0..59 to coordinates. */

static void get_hand_endpoint(int center_x, int center_y, int length, int step, int* out_x, int* out_y) {
    /* Simple lookup is too big, let's use a small approximation or logic.
       x = cx + len * sin(angle)
       y = cy - len * cos(angle)
       Wait, standard math:
       step 0 (12 o'clock) -> sin=0, cos=1 -> x=cx, y=cy-len. Correct.
       step 15 (3 o'clock) -> sin=1, cos=0 -> x=cx+len, y=cy. Correct.
    */
    
    /* We need sin/cos. Since we don't have libm, we implement a minimal lookup for 60 steps? 
       Actually, let's just implement a rough sin/cos table generator or hardcode a few if needed.
       Better: use a small precomputed table for 0..15 (90 degrees) and mirror.
    */
    
    /* 16 values for 0..90 degrees (0..15 steps) */
    static const int sin_val[] = { 0, 107, 212, 316, 416, 512, 601, 685, 761, 828, 887, 935, 972, 998, 1014, 1024 };
    /* cos is reverse of sin */
    
    int s = step % 60;
    int quad = s / 15;
    int offs = s % 15;
    
    int sin_v = 0, cos_v = 0;
    
    if (quad == 0) { // 0..15
        sin_v = sin_val[offs];
        cos_v = sin_val[15 - offs];
    } else if (quad == 1) { // 15..30
        sin_v = sin_val[15 - offs];
        cos_v = -sin_val[offs];
    } else if (quad == 2) { // 30..45
        sin_v = -sin_val[offs];
        cos_v = -sin_val[15 - offs];
    } else { // 45..60
        sin_v = -sin_val[15 - offs];
        cos_v = sin_val[offs];
    }
    
    *out_x = center_x + (length * sin_v) / 1024;
    *out_y = center_y - (length * cos_v) / 1024;
}

/* Graphics Primitives */
static void put_pixel(surface_t* s, int x, int y, uint32_t color) {
    if (x < 0 || y < 0 || x >= (int)s->width || y >= (int)s->height) return;
    s->pixels[y * s->width + x] = color;
}

static void draw_line(surface_t* s, int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;
    int e2;

    while (1) {
        put_pixel(s, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }
}

static void draw_circle(surface_t* s, int xc, int yc, int r, uint32_t color) {
    int x = 0, y = r;
    int d = 3 - 2 * r;
    while (y >= x) {
        put_pixel(s, xc+x, yc+y, color);
        put_pixel(s, xc-x, yc+y, color);
        put_pixel(s, xc+x, yc-y, color);
        put_pixel(s, xc-x, yc-y, color);
        put_pixel(s, xc+y, yc+x, color);
        put_pixel(s, xc-y, yc+x, color);
        put_pixel(s, xc+y, yc-x, color);
        put_pixel(s, xc-y, yc-x, color);
        x++;
        if (d > 0) { y--; d = d + 4 * (x - y) + 10; }
        else d = d + 4 * x + 6;
    }
}

static void draw_clock_face(surface_t* s) {
    /* Background */
    fly_draw_rect_fill(s, 0, 24, WIN_W, WIN_H-24, COL_BG);
    
    /* Title Bar */
    fly_draw_rect_fill(s, 0, 0, WIN_W, 24, COL_TITLE);
    fly_draw_text(s, 6, 4, "Clock", 0xFFFFFFFF);
    
    /* Close Button [X] */
    int bx = WIN_W - 20;
    int by = 4;
    fly_draw_rect_fill(s, bx, by, 16, 16, COL_BTN);
    fly_draw_rect_outline(s, bx, by, 16, 16, 0xFF000000);
    fly_draw_text(s, bx + 4, by, "X", COL_BTN_X);

    /* Clock Circle */
    draw_circle(s, CENTER_X, CENTER_Y, CLOCK_R, COL_BORDER);
    
    /* Ticks */
    for (int i = 0; i < 12; i++) {
        int x1, y1, x2, y2;
        get_hand_endpoint(CENTER_X, CENTER_Y, CLOCK_R, i * 5, &x1, &y1);
        get_hand_endpoint(CENTER_X, CENTER_Y, CLOCK_R - 5, i * 5, &x2, &y2);
        draw_line(s, x1, y1, x2, y2, COL_BORDER);
    }
}

static void draw_hands(surface_t* s, int h, int m, int sec) {
    int x, y;
    
    /* Hour Hand */
    /* 12 hours = 60 steps. h*5 + m/12 */
    int h_step = (h % 12) * 5 + (m / 12);
    get_hand_endpoint(CENTER_X, CENTER_Y, CLOCK_R - 30, h_step, &x, &y);
    draw_line(s, CENTER_X, CENTER_Y, x, y, COL_HOUR);
    /* Thicker hour hand (draw adjacent line) */
    draw_line(s, CENTER_X+1, CENTER_Y, x+1, y, COL_HOUR);

    /* Minute Hand */
    get_hand_endpoint(CENTER_X, CENTER_Y, CLOCK_R - 10, m, &x, &y);
    draw_line(s, CENTER_X, CENTER_Y, x, y, COL_MIN);

    /* Second Hand */
    get_hand_endpoint(CENTER_X, CENTER_Y, CLOCK_R - 5, sec, &x, &y);
    draw_line(s, CENTER_X, CENTER_Y, x, y, COL_SEC);
}

void clock_app_create(void) {
    if (clock_win) {
        wm_restore_window(clock_win);
        wm_focus_window(clock_win);
        return;
    }

    surface_t* s = surface_create(WIN_W, WIN_H);
    if (!s) return;

    clock_win = wm_create_window(s, 150, 150);
    if (clock_win) {
        wm_set_title(clock_win, "Clock");
        wm_set_on_close(clock_win, clock_close);
        wm_set_on_resize(clock_win, clock_on_resize);
    }
    last_sec = -1;
    
    /* Initial Draw */
    clock_app_update();
    
    serial("[CLOCK] App started.\n");
}

void clock_app_update(void) {
    if (!clock_win) return;

    datetime t;
    time_get_local(&t);

    if (t.second != last_sec) {
        last_sec = t.second;
        
        surface_t* s = clock_win->surface;
        
        /* Redraw everything (simple double buffering simulation) */
        draw_clock_face(s);
        draw_hands(s, t.hour, t.minute, t.second);
        
        wm_mark_dirty();
    }
}

bool clock_app_handle_event(input_event_t* ev) {
    if (!clock_win) return false;

    if (ev->type == INPUT_MOUSE_CLICK && ev->pressed) {
        /* Check Close Button */
        /* Mouse coords are global, convert to window local */
        int lx = ev->mouse_x - clock_win->x;
        int ly = ev->mouse_y - clock_win->y;
        
        /* Close Button Rect: x=W-20, y=4, w=16, h=16 */
        if (lx >= WIN_W - 20 && lx <= WIN_W - 4 && ly >= 4 && ly <= 20) {
            serial("[CLOCK] Close button clicked.\n");
            clock_close(clock_win);
            return true;
        }
        
        /* Title Bar Drag (handled by win.cpp generic logic usually, but we can return false to let win.cpp handle drag start if it checks title bar) */
        /* win.cpp handles drag if we focus the window. */
    }
    
    return false;
}

window_t* clock_app_get_window(void) {
    return clock_win;
}
