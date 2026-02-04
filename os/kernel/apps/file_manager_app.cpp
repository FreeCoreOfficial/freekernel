#include "file_manager_app.h"
#include "../ui/wm/wm.h"
#include "../ui/flyui/draw.h"
#include "../cmds/fat.h"
#include "../string.h"
#include "../drivers/serial.h"
#include "../mem/kmalloc.h"
#include "app_manager.h"
#include "notepad_app.h"

static window_t* fm_win = NULL;
static char current_path[256] = "/";
static fat_file_info_t files[32];
static int file_count = 0;
static int selected_idx = -1;

static void append_path(char* dst, size_t cap, const char* src) {
    size_t len = strlen(dst);
    size_t i = 0;
    while (src[i] && (len + 1) < cap) {
        dst[len++] = src[i++];
    }
    dst[len] = 0;
}

static void fm_go_up(void) {
    if (strcmp(current_path, "/") == 0) return;
    int len = strlen(current_path);
    while (len > 0 && current_path[len - 1] != '/') len--;
    if (len == 0) strcpy(current_path, "/");
    else {
        current_path[len - 1] = 0;
        if (strlen(current_path) == 0) strcpy(current_path, "/");
    }
}

static void build_breadcrumb(char* out, size_t cap) {
    if (!out || cap == 0) return;
    out[0] = 0;

    if (strcmp(current_path, "/") == 0) {
        strncpy(out, "/", cap);
        out[cap - 1] = 0;
        return;
    }

    append_path(out, cap, "/");
    char tmp[256];
    strncpy(tmp, current_path, sizeof(tmp));
    tmp[sizeof(tmp) - 1] = 0;

    char* p = tmp;
    if (*p == '/') p++;
    while (*p) {
        char* seg = p;
        while (*p && *p != '/') p++;
        char saved = *p;
        *p = 0;

        append_path(out, cap, " > ");
        append_path(out, cap, seg);

        if (saved == 0) break;
        *p = saved;
        p++;
    }
}

static void refresh_files() {
    fat_automount();
    file_count = fat32_read_directory(current_path, files, 32);
}

static void draw_fm(surface_t* s) {
    fly_draw_rect_fill(s, 0, 0, s->width, s->height, 0xFFFFFFFF);
    fly_draw_rect_fill(s, 0, 0, s->width, 24, 0xFF000080);
    fly_draw_text(s, 5, 4, "File Manager", 0xFFFFFFFF);
    fly_draw_rect_fill(s, s->width-20, 4, 16, 16, 0xFFC0C0C0);
    fly_draw_text(s, s->width-16, 4, "X", 0xFF000000);

    /* Path Bar */
    fly_draw_rect_fill(s, 5, 28, 18, 18, 0xFFC0C0C0);
    fly_draw_rect_outline(s, 5, 28, 18, 18, 0xFF000000);
    fly_draw_text(s, 9, 30, "<", 0xFF000000);

    fly_draw_text(s, 28, 30, "Path:", 0xFF000000);
    fly_draw_text(s, 70, 30, current_path, 0xFF000000);

    char breadcrumb[256];
    build_breadcrumb(breadcrumb, sizeof(breadcrumb));
    fly_draw_text(s, 28, 44, breadcrumb, 0xFF404040);
    fly_draw_rect_outline(s, 0, 58, s->width, 1, 0xFF000000);

    /* File List */
    int y = 66;
    for (int i = 0; i < file_count; i++) {
        uint32_t bg = (i == selected_idx) ? 0xFF000080 : 0xFFFFFFFF;
        uint32_t fg = (i == selected_idx) ? 0xFFFFFFFF : 0xFF000000;
        
        fly_draw_rect_fill(s, 5, y, s->width-10, 18, bg);
        
        char label[64];
        if (files[i].is_dir) {
            /* Draw Folder Icon (Yellow Rect) */
            fly_draw_rect_fill(s, 10, y+2, 12, 12, 0xFFFFFF00);
            strcpy(label, "      "); /* Space for icon */
            strcat(label, files[i].name);
        } else {
            strcpy(label, files[i].name);
        }
        fly_draw_text(s, 25, y+2, label, fg);
        y += 20;
    }
}

void file_manager_app_create(void) {
    if (fm_win) return;
    surface_t* s = surface_create(300, 400);
    fm_win = wm_create_window(s, 50, 50);
    refresh_files();
    draw_fm(s);
    app_register("File Manager", fm_win);
}

bool file_manager_app_handle_event(input_event_t* ev) {
    if (!fm_win) return false;
    if (ev->type == INPUT_MOUSE_CLICK && ev->pressed) {
        int lx = ev->mouse_x - fm_win->x;
        int ly = ev->mouse_y - fm_win->y;

        /* Close */
        if (lx >= fm_win->w - 20 && ly < 24) {
            wm_destroy_window(fm_win);
            app_unregister(fm_win);
            fm_win = NULL;
            wm_mark_dirty();
            return true;
        }

        /* Back Button */
        if (lx >= 5 && lx <= 23 && ly >= 28 && ly <= 46) {
            fm_go_up();
            refresh_files();
            selected_idx = -1;
            draw_fm(fm_win->surface);
            wm_mark_dirty();
            return true;
        }

        /* File Click */
        if (ly >= 66) {
            int idx = (ly - 66) / 20;
            if (idx >= 0 && idx < file_count) {
                if (selected_idx == idx) {
                    /* Double Click Simulation (Second click on same item) */
                    /* Open logic */
                    if (files[idx].is_dir) {
                        if (strcmp(files[idx].name, ".") == 0) { /* no-op */ }
                        else if (strcmp(files[idx].name, "..") == 0) {
                            /* Go Up */
                            fm_go_up();
                        } else {
                            /* Go Down */
                            if (strcmp(current_path, "/") != 0) strcat(current_path, "/");
                            strcat(current_path, files[idx].name);
                        }
                        refresh_files();
                        selected_idx = -1;
                    } else {
                        char fullpath[256];
                        if (strcmp(current_path, "/") == 0) {
                            strncpy(fullpath, "/", sizeof(fullpath));
                            fullpath[sizeof(fullpath) - 1] = 0;
                            append_path(fullpath, sizeof(fullpath), files[idx].name);
                        } else {
                            strncpy(fullpath, current_path, sizeof(fullpath));
                            fullpath[sizeof(fullpath) - 1] = 0;
                            if (fullpath[strlen(fullpath) - 1] != '/') {
                                append_path(fullpath, sizeof(fullpath), "/");
                            }
                            append_path(fullpath, sizeof(fullpath), files[idx].name);
                        }
                        notepad_app_open(fullpath);
                    }
                } else {
                    selected_idx = idx;
                }
                draw_fm(fm_win->surface);
                wm_mark_dirty();
            }
        }
        return true;
    }
    return false;
}

window_t* file_manager_app_get_window(void) { return fm_win; }
