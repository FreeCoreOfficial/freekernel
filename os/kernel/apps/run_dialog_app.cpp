#include "run_dialog_app.h"
#include "apps.h"
#include "../ui/wm/wm.h"
#include "../ui/flyui/draw.h"
#include "../string.h"
#include "../shell/shell.h"

static window_t* run_win = NULL;
static char cmd_buf[64] = "";

static void draw_run(surface_t* s);

static void run_close(window_t* win) {
    (void)win;
    if (run_win) {
        wm_destroy_window(run_win);
        run_win = NULL;
        wm_mark_dirty();
    }
}

static void run_on_resize(window_t* win) {
    if (!win || !win->surface) return;
    draw_run(win->surface);
    wm_mark_dirty();
}

static void draw_run(surface_t* s) {
    fly_draw_rect_fill(s, 0, 0, 300, 120, 0xFFE0E0E0);
    fly_draw_rect_fill(s, 0, 0, 300, 24, 0xFF000080);
    fly_draw_text(s, 5, 4, "Run", 0xFFFFFFFF);
    fly_draw_rect_fill(s, 280, 4, 16, 16, 0xFFC0C0C0);
    fly_draw_text(s, 284, 4, "X", 0xFF000000);

    fly_draw_text(s, 10, 35, "Type the name of a program to open:", 0xFF000000);
    
    /* Input Box */
    fly_draw_rect_fill(s, 10, 55, 280, 20, 0xFFFFFFFF);
    fly_draw_rect_outline(s, 10, 55, 280, 20, 0xFF000000);
    fly_draw_text(s, 15, 57, cmd_buf, 0xFF000000);

    /* OK Button */
    fly_draw_rect_fill(s, 200, 85, 80, 25, 0xFFC0C0C0);
    fly_draw_rect_outline(s, 200, 85, 80, 25, 0xFF000000);
    fly_draw_text(s, 225, 90, "Run", 0xFF000000);
}

static void execute_run() {
    if (strcmp(cmd_buf, "calc") == 0) calculator_app_create();
    else if (strcmp(cmd_buf, "notepad") == 0) notepad_app_create();
    else if (strcmp(cmd_buf, "clock") == 0) clock_app_create();
    else if (strcmp(cmd_buf, "calendar") == 0) calendar_app_create();
    else if (strcmp(cmd_buf, "files") == 0) file_manager_app_create();
    else if (strcmp(cmd_buf, "info") == 0) sysinfo_app_create();
    else if (strcmp(cmd_buf, "paint") == 0) paint_app_create();
    else if (strcmp(cmd_buf, "demo") == 0) demo3d_app_create();
    else if (strcmp(cmd_buf, "terminal") == 0) shell_create_window();
    else if (strncmp(cmd_buf, "view ", 5) == 0) image_viewer_app_create(cmd_buf + 5);
    
    wm_destroy_window(run_win);
    run_win = NULL;
    wm_mark_dirty();
}

void run_dialog_app_create(void) {
    if (run_win) {
        wm_restore_window(run_win);
        wm_focus_window(run_win);
        return;
    }
    surface_t* s = surface_create(300, 120);
    run_win = wm_create_window(s, 100, 300);
    if (run_win) {
        wm_set_title(run_win, "Run");
        wm_set_on_close(run_win, run_close);
        wm_set_on_resize(run_win, run_on_resize);
    }
    cmd_buf[0] = 0;
    draw_run(s);
}

bool run_dialog_app_handle_event(input_event_t* ev) {
    if (!run_win) return false;
    if (ev->type == INPUT_MOUSE_CLICK && ev->pressed) {
        int lx = ev->mouse_x - run_win->x;
        int ly = ev->mouse_y - run_win->y;
        
        /* Close */
        if (lx >= 280 && ly < 24) {
            run_close(run_win);
            return true;
        }
        /* Run Button */
        if (lx >= 200 && lx <= 280 && ly >= 85 && ly <= 110) {
            execute_run();
            return true;
        }
    }
    return false;
}

void run_dialog_app_handle_key(char c) {
    if (!run_win) return;
    int len = strlen(cmd_buf);
    if (c == '\b') {
        if (len > 0) cmd_buf[len-1] = 0;
    } else if (c == '\n') {
        execute_run();
        return;
    } else if (c >= 32 && c <= 126) {
        if (len < 60) {
            cmd_buf[len] = c;
            cmd_buf[len+1] = 0;
        }
    }
    draw_run(run_win->surface);
    wm_mark_dirty();
}

window_t* run_dialog_app_get_window(void) { return run_win; }
