#include "sysinfo_app.h"
#include "../ui/wm/wm.h"
#include "../ui/flyui/draw.h"
#include "../memory/pmm.h"
#include "../panic_sys.h"
#include "../string.h"

extern "C" char* itoa_dec(char* out, int32_t v);

static window_t* sys_win = NULL;

void sysinfo_app_create(void) {
    if (sys_win) return;
    surface_t* s = surface_create(300, 200);
    surface_clear(s, 0xFFE0E0E0);

    fly_draw_rect_fill(s, 0, 0, 300, 24, 0xFF000080);
    fly_draw_text(s, 5, 4, "About Chrysalis OS", 0xFFFFFFFF);
    fly_draw_rect_fill(s, 280, 4, 16, 16, 0xFFC0C0C0);
    fly_draw_text(s, 284, 4, "X", 0xFF000000);

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

    sys_win = wm_create_window(s, 150, 150);
}

bool sysinfo_app_handle_event(input_event_t* ev) {
    if (!sys_win) return false;
    if (ev->type == INPUT_MOUSE_CLICK && ev->pressed) {
        int lx = ev->mouse_x - sys_win->x;
        int ly = ev->mouse_y - sys_win->y;
        if (lx >= sys_win->w - 20 && ly < 24) {
            wm_destroy_window(sys_win);
            sys_win = NULL;
            wm_mark_dirty();
            return true;
        }
    }
    return false;
}

window_t* sysinfo_app_get_window(void) { return sys_win; }