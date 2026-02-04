#include "sysinfo_app.h"
#include "../ui/wm/wm.h"
#include "../ui/flyui/draw.h"
#include "../memory/pmm.h"
#include "../panic_sys.h"
#include "../string.h"

extern "C" char* itoa_dec(char* out, int32_t v);

static window_t* sys_win = NULL;

static void draw_sysinfo(surface_t* s) {
    surface_clear(s, 0xFFE0E0E0);

    fly_draw_text(s, 20, 40, "Chrysalis OS v0.3", 0xFF000000);
    fly_draw_text(s, 20, 60, "Build: 2026", 0xFF000000);

    char buf[64];
    strcpy(buf, "CPU: ");
    strcat(buf, panic_sys_cpu_str());
    fly_draw_text(s, 20, 90, buf, 0xFF000000);

    uint32_t total = pmm_total_frames() * 4 / 1024;
    uint32_t used = pmm_used_frames() * 4 / 1024;
    char num[16];

    strcpy(buf, "RAM: ");
    itoa_dec(num, used); strcat(buf, num);
    strcat(buf, "MB / ");
    itoa_dec(num, total); strcat(buf, num);
    strcat(buf, "MB");
    fly_draw_text(s, 20, 110, buf, 0xFF000000);
}

static void sys_close(window_t* win) {
    (void)win;
    if (sys_win) {
        wm_destroy_window(sys_win);
        sys_win = NULL;
        wm_mark_dirty();
    }
}

static void sys_on_resize(window_t* win) {
    if (!win || !win->surface) return;
    draw_sysinfo(win->surface);
    wm_mark_dirty();
}

void sysinfo_app_create(void) {
    if (sys_win) {
        wm_restore_window(sys_win);
        wm_focus_window(sys_win);
        return;
    }
    surface_t* s = surface_create(300, 200);
    draw_sysinfo(s);

    sys_win = wm_create_window(s, 150, 150);
    if (sys_win) {
        wm_set_title(sys_win, "System Info");
        wm_set_on_close(sys_win, sys_close);
        wm_set_on_resize(sys_win, sys_on_resize);
    }
}

bool sysinfo_app_handle_event(input_event_t* ev) {
    if (!sys_win) return false;
    if (ev->type == INPUT_MOUSE_CLICK && ev->pressed) {
        int lx = ev->mouse_x - sys_win->x;
        int ly = ev->mouse_y - sys_win->y;
        if (lx >= sys_win->w - 20 && ly < 24) {
            sys_close(sys_win);
            return true;
        }
    }
    return false;
}

window_t* sysinfo_app_get_window(void) { return sys_win; }
