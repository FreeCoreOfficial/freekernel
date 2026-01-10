#include "file_manager_app.h"
#include "../ui/wm/wm.h"
#include "../ui/flyui/draw.h"
#include "../cmds/fat.h"
#include "../string.h"
#include "../drivers/serial.h"
#include "../mem/kmalloc.h"
#include "app_manager.h"

static window_t* fm_win = NULL;
static char current_path[256] = "/";
static fat_file_info_t files[32];
static int file_count = 0;
static int selected_idx = -1;

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
    fly_draw_text(s, 5, 30, current_path, 0xFF000000);
    fly_draw_rect_outline(s, 0, 50, s->width, 1, 0xFF000000);

    /* File List */
    int y = 60;
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

        /* File Click */
        if (ly >= 60) {
            int idx = (ly - 60) / 20;
            if (idx >= 0 && idx < file_count) {
                if (selected_idx == idx) {
                    /* Double Click Simulation (Second click on same item) */
                    /* Open logic */
                    if (files[idx].is_dir) {
                        if (strcmp(files[idx].name, ".") == 0) { /* no-op */ }
                        else if (strcmp(files[idx].name, "..") == 0) {
                            /* Go Up */
                            int len = strlen(current_path);
                            while(len > 0 && current_path[len-1] != '/') len--;
                            if (len == 0) strcpy(current_path, "/");
                            else {
                                current_path[len-1] = 0;
                                if (strlen(current_path) == 0) strcpy(current_path, "/");
                            }
                        } else {
                            /* Go Down */
                            if (strcmp(current_path, "/") != 0) strcat(current_path, "/");
                            strcat(current_path, files[idx].name);
                        }
                        refresh_files();
                        selected_idx = -1;
                    } else {
                        /* File Open - TODO: Launch Image Viewer for .bmp */
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