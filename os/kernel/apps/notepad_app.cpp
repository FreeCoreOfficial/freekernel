#include "notepad_app.h"
#include "../ui/wm/wm.h"
#include "../video/surface.h"
#include "../ui/flyui/draw.h"
#include "../mem/kmalloc.h"
#include "../string.h"
#include "../drivers/serial.h"
#include "../cmds/fat.h"

extern "C" void serial(const char *fmt, ...);

static window_t* note_win = NULL;

#define NOTE_W 400
#define NOTE_H 300

static char filename[128] = "/new.txt";
static char content[4096] = "";
static int content_len = 0;
static bool focus_filename = false;

static void draw_notepad_ui(surface_t* s);

static void note_close(window_t* win) {
    (void)win;
    if (note_win) {
        wm_destroy_window(note_win);
        note_win = NULL;
        wm_mark_dirty();
    }
}

static void note_on_resize(window_t* win) {
    if (!win || !win->surface) return;
    draw_notepad_ui(win->surface);
    wm_mark_dirty();
}

static void draw_notepad_ui(surface_t* s) {
    int w = (int)s->width;
    int h = (int)s->height;
    /* Background */
    fly_draw_rect_fill(s, 0, 0, w, h, 0xFFE0E0E0);
    
    /* Toolbar */
    fly_draw_text(s, 10, 35, "File:", 0xFF000000);
    
    /* Filename Box */
    uint32_t fn_bg = focus_filename ? 0xFFFFFFFF : 0xFFF0F0F0;
    fly_draw_rect_fill(s, 50, 30, 150, 20, fn_bg);
    fly_draw_rect_outline(s, 50, 30, 150, 20, 0xFF000000);
    fly_draw_text(s, 55, 32, filename, 0xFF000000);

    /* Load Button */
    fly_draw_rect_fill(s, 210, 30, 50, 20, 0xFFC0C0C0);
    fly_draw_rect_outline(s, 210, 30, 50, 20, 0xFF000000);
    fly_draw_text(s, 220, 32, "Load", 0xFF000000);

    /* Save Button */
    fly_draw_rect_fill(s, 270, 30, 50, 20, 0xFFC0C0C0);
    fly_draw_rect_outline(s, 270, 30, 50, 20, 0xFF000000);
    fly_draw_text(s, 280, 32, "Save", 0xFF000000);

    /* Content Area */
    uint32_t ct_bg = (!focus_filename) ? 0xFFFFFFFF : 0xFFF0F0F0;
    fly_draw_rect_fill(s, 10, 60, w - 20, h - 70, ct_bg);
    fly_draw_rect_outline(s, 10, 60, w - 20, h - 70, 0xFF000000);

    /* Draw Content (Simple multiline) */
    int cx = 15;
    int cy = 65;
    for (int i = 0; i < content_len; i++) {
        char c = content[i];
        if (c == '\n') {
            cx = 15;
            cy += 16;
            if (cy > h - 20) break; /* Clip */
            continue;
        }
        char str[2] = {c, 0};
        fly_draw_text(s, cx, cy, str, 0xFF000000);
        cx += 8;
        if (cx > w - 20) {
            cx = 15;
            cy += 16;
        }
    }
    
    /* Cursor */
    if (!focus_filename) {
        fly_draw_rect_fill(s, cx, cy, 2, 14, 0xFF000000);
    }
}

static void save_file() {
    fat_automount();
    if (fat32_create_file(filename, content, content_len) == 0) {
        serial("[NOTE] Saved %s (%d bytes)\n", filename, content_len);
    } else {
        serial("[NOTE] Save failed for %s\n", filename);
    }
}

static void load_file() {
    fat_automount();
    int bytes = fat32_read_file(filename, content, 4095);
    if (bytes >= 0) {
        content_len = bytes;
        content[content_len] = 0;
        serial("[NOTE] Loaded %s (%d bytes)\n", filename, content_len);
    } else {
        serial("[NOTE] Load failed for %s\n", filename);
        /* Clear content on fail? No, keep current. */
    }
}

void notepad_app_create(void) {
    if (note_win) {
        wm_restore_window(note_win);
        wm_focus_window(note_win);
        return;
    }

    surface_t* s = surface_create(NOTE_W, NOTE_H);
    if (!s) return;

    note_win = wm_create_window(s, 100, 100);
    if (note_win) {
        wm_set_title(note_win, "Notepad");
        wm_set_on_close(note_win, note_close);
        wm_set_on_resize(note_win, note_on_resize);
    }
    
    /* Reset */
    strcpy(filename, "/new.txt");
    content[0] = 0;
    content_len = 0;
    focus_filename = false;

    draw_notepad_ui(s);
    serial("[NOTE] Started.\n");
}

void notepad_app_open(const char* path) {
    if (!path || !*path) return;

    if (!note_win) {
        notepad_app_create();
        if (!note_win) return;
    }

    strncpy(filename, path, sizeof(filename));
    filename[sizeof(filename) - 1] = 0;
    focus_filename = false;
    load_file();
    draw_notepad_ui(note_win->surface);
    wm_mark_dirty();
}

bool notepad_app_handle_event(input_event_t* ev) {
    if (!note_win) return false;

    if (ev->type == INPUT_MOUSE_CLICK && ev->pressed) {
        int lx = ev->mouse_x - note_win->x;
        int ly = ev->mouse_y - note_win->y;

        /* Close */
        if (lx >= NOTE_W - 20 && lx <= NOTE_W - 4 && ly >= 4 && ly <= 20) {
            wm_destroy_window(note_win);
            note_win = NULL;
            wm_mark_dirty();
            return true;
        }

        /* Filename Box Click */
        if (lx >= 50 && lx <= 200 && ly >= 30 && ly <= 50) {
            focus_filename = true;
            draw_notepad_ui(note_win->surface);
            wm_mark_dirty();
            return true;
        }

        /* Content Box Click */
        if (lx >= 10 && lx <= NOTE_W - 10 && ly >= 60 && ly <= NOTE_H - 10) {
            focus_filename = false;
            draw_notepad_ui(note_win->surface);
            wm_mark_dirty();
            return true;
        }

        /* Load Button */
        if (lx >= 210 && lx <= 260 && ly >= 30 && ly <= 50) {
            load_file();
            draw_notepad_ui(note_win->surface);
            wm_mark_dirty();
            return true;
        }

        /* Save Button */
        if (lx >= 270 && lx <= 320 && ly >= 30 && ly <= 50) {
            save_file();
            return true;
        }
    }
    return false;
}

void notepad_app_handle_key(char c) {
    if (!note_win) return;

    if (focus_filename) {
        int len = strlen(filename);
        if (c == '\b') {
            if (len > 0) filename[len-1] = 0;
        } else if (c >= 32 && c <= 126) {
            if (len < 63) {
                filename[len] = c;
                filename[len+1] = 0;
            }
        }
    } else {
        /* Content editing */
        if (c == '\b') {
            if (content_len > 0) {
                content_len--;
                content[content_len] = 0;
            }
        } else if (c == '\n' || (c >= 32 && c <= 126)) {
            if (content_len < 4095) {
                content[content_len++] = c;
                content[content_len] = 0;
            }
        }
    }
    draw_notepad_ui(note_win->surface);
    wm_mark_dirty();
}

window_t* notepad_app_get_window(void) {
    return note_win;
}
