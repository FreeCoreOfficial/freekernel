#include "calendar_app.h"
#include "../ui/wm/wm.h"
#include "../video/surface.h"
#include "../ui/flyui/draw.h"
#include "../time/clock.h"
#include "../mem/kmalloc.h"
#include "../string.h"
#include "../drivers/serial.h"

extern "C" void serial(const char *fmt, ...);
extern "C" char* itoa_dec(char* out, int32_t v);

static window_t* cal_win = NULL;

#define CAL_W 240
#define CAL_H 240
#define CELL_W 30
#define CELL_H 20
#define HEADER_H 40

static int view_month = 1;
static int view_year = 2026;
static int today_day = 0;
static int today_month = 0;
static int today_year = 0;

/* Helpers */
static bool is_leap(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

static int days_in_month(int m, int y) {
    if (m == 2) return is_leap(y) ? 29 : 28;
    if (m == 4 || m == 6 || m == 9 || m == 11) return 30;
    return 31;
}

/* Zeller's congruence: 0=Sunday, 1=Monday, ... 6=Saturday */
static int get_day_of_week(int d, int m, int y) {
    if (m < 3) { m += 12; y -= 1; }
    int K = y % 100;
    int J = y / 100;
    int h = (d + 13*(m+1)/5 + K + K/4 + J/4 + 5*J) % 7;
    return (h + 6) % 7; /* Adjust to 0=Sun */
}

static const char* month_names[] = {
    "", "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

static void draw_calendar_ui(surface_t* s) {
    /* Background */
    fly_draw_rect_fill(s, 0, 0, CAL_W, CAL_H, 0xFFFFFFFF);
    
    /* Title Bar */
    fly_draw_rect_fill(s, 0, 0, CAL_W, 24, 0xFF800000); /* Dark Red */
    fly_draw_text(s, 5, 4, "Calendar", 0xFFFFFFFF);
    
    /* Close Button */
    fly_draw_rect_fill(s, CAL_W - 20, 4, 16, 16, 0xFFC0C0C0);
    fly_draw_text(s, CAL_W - 16, 4, "X", 0xFF000000);

    /* Header (Month Year + Nav) */
    int hy = 30;
    /* Prev Button (<) */
    fly_draw_rect_fill(s, 10, hy, 20, 20, 0xFFE0E0E0);
    fly_draw_rect_outline(s, 10, hy, 20, 20, 0xFF000000);
    fly_draw_text(s, 16, hy+2, "<", 0xFF000000);

    /* Next Button (>) */
    fly_draw_rect_fill(s, CAL_W - 30, hy, 20, 20, 0xFFE0E0E0);
    fly_draw_rect_outline(s, CAL_W - 30, hy, 20, 20, 0xFF000000);
    fly_draw_text(s, CAL_W - 24, hy+2, ">", 0xFF000000);

    /* Label */
    char header[32];
    char year_str[8];
    itoa_dec(year_str, view_year);
    strcpy(header, month_names[view_month]);
    strcat(header, " ");
    strcat(header, year_str);
    
    int text_w = strlen(header) * 8;
    fly_draw_text(s, (CAL_W - text_w)/2, hy+2, header, 0xFF000000);

    /* Days Header */
    const char* days[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    int grid_y = 60;
    int margin_x = 15;
    
    for (int i = 0; i < 7; i++) {
        fly_draw_text(s, margin_x + i * CELL_W + 6, grid_y, days[i], 0xFF000080);
    }

    /* Days Grid */
    int start_day = get_day_of_week(1, view_month, view_year); /* 0=Sun */
    int total_days = days_in_month(view_month, view_year);
    
    int row = 0;
    int col = start_day;
    int y_off = grid_y + 20;

    for (int d = 1; d <= total_days; d++) {
        int x = margin_x + col * CELL_W;
        int y = y_off + row * CELL_H;
        
        /* Highlight Today */
        bool is_today = (d == today_day && view_month == today_month && view_year == today_year);
        if (is_today) {
            fly_draw_rect_fill(s, x, y, CELL_W, CELL_H, 0xFFFFFF00); /* Yellow */
        }

        char day_str[4];
        itoa_dec(day_str, d);
        int dw = strlen(day_str) * 8;
        fly_draw_text(s, x + (CELL_W - dw)/2, y + 2, day_str, is_today ? 0xFF000000 : 0xFF000000);

        col++;
        if (col > 6) {
            col = 0;
            row++;
        }
    }
}

void calendar_app_create(void) {
    if (cal_win) return;

    surface_t* s = surface_create(CAL_W, CAL_H);
    if (!s) return;

    cal_win = wm_create_window(s, 250, 150);
    
    /* Init with current date */
    datetime t;
    time_get_local(&t);
    today_day = t.day;
    today_month = t.month;
    today_year = t.year;
    
    view_month = today_month;
    view_year = today_year;

    draw_calendar_ui(s);
    serial("[CAL] Started.\n");
}

bool calendar_app_handle_event(input_event_t* ev) {
    if (!cal_win) return false;

    if (ev->type == INPUT_MOUSE_CLICK && ev->pressed) {
        int lx = ev->mouse_x - cal_win->x;
        int ly = ev->mouse_y - cal_win->y;

        /* Close Button */
        if (lx >= CAL_W - 20 && lx <= CAL_W - 4 && ly >= 4 && ly <= 20) {
            wm_destroy_window(cal_win);
            cal_win = NULL;
            wm_mark_dirty();
            return true;
        }

        /* Navigation */
        int hy = 30;
        /* Prev (<) */
        if (lx >= 10 && lx <= 30 && ly >= hy && ly <= hy+20) {
            view_month--;
            if (view_month < 1) {
                view_month = 12;
                view_year--;
            }
            draw_calendar_ui(cal_win->surface);
            wm_mark_dirty();
            return true;
        }
        /* Next (>) */
        if (lx >= CAL_W - 30 && lx <= CAL_W - 10 && ly >= hy && ly <= hy+20) {
            view_month++;
            if (view_month > 12) {
                view_month = 1;
                view_year++;
            }
            draw_calendar_ui(cal_win->surface);
            wm_mark_dirty();
            return true;
        }
    }
    return false;
}

window_t* calendar_app_get_window(void) {
    return cal_win;
}