#include "wm.h"
#include "../../video/compositor.h"
#include "../../mem/kmalloc.h"
#include <stddef.h>
#include "../../terminal.h"
#include "../../video/gpu.h"
#include "../../drivers/mouse.h"
#include "../flyui/draw.h"
#include "../flyui/theme.h"
#include "../../string.h"

/* Import serial logging */
extern void serial(const char *fmt, ...);

#define MAX_WINDOWS 32

static window_t* windows_list = NULL;
static window_t* focused_window = NULL;
static wm_layout_t* current_layout = &wm_layout_floating;
static uint32_t next_win_id = 1;
static bool wm_dirty = true;
static int wm_reserved_bottom = 0;

/* Array for layout processing */
static window_t* win_array[MAX_WINDOWS];
static size_t win_count = 0;

void wm_init(void) {
    windows_list = NULL;
    focused_window = NULL;
    current_layout = &wm_layout_floating;
    win_count = 0;
    wm_dirty = true;
    serial("[WM] Initialized\n");
}

void wm_mark_dirty(void) {
    wm_dirty = true;
}

bool wm_is_dirty(void) {
    return wm_dirty;
}

void wm_set_reserved_bottom(int pixels) {
    if (pixels < 0) pixels = 0;
    wm_reserved_bottom = pixels;
}

int wm_get_reserved_bottom(void) {
    return wm_reserved_bottom;
}

bool wm_window_is_decorated(window_t* win) {
    if (!win) return false;
    return (win->flags & WIN_FLAG_NO_DECOR) == 0;
}

void wm_set_window_flags(window_t* win, uint32_t flags) {
    if (!win) return;
    win->flags = flags;
}

void wm_set_title(window_t* win, const char* title) {
    if (!win || !title) return;
    strncpy(win->title, title, sizeof(win->title) - 1);
    win->title[sizeof(win->title) - 1] = 0;
}

void wm_set_on_close(window_t* win, void (*on_close)(window_t*)) {
    if (!win) return;
    win->on_close = on_close;
}

void wm_set_on_resize(window_t* win, void (*on_resize)(window_t*)) {
    if (!win) return;
    win->on_resize = on_resize;
}

void wm_minimize_window(window_t* win) {
    if (!win || !win->surface) return;
    if (win->state == WIN_STATE_MINIMIZED) return;

    win->restore_x = win->x;
    win->restore_y = win->y;
    win->restore_w = win->w;
    win->restore_h = win->h;
    win->state = WIN_STATE_MINIMIZED;
    win->surface->visible = false;
    if (focused_window == win) focused_window = NULL;
    wm_dirty = true;
}

void wm_restore_window(window_t* win) {
    if (!win || !win->surface) return;
    if (win->state == WIN_STATE_MINIMIZED) {
        win->surface->visible = true;
        win->state = WIN_STATE_NORMAL;
        wm_focus_window(win);
        wm_dirty = true;
    }
}

static void wm_maximize_window(window_t* win) {
    if (!win || !win->surface) return;
    gpu_device_t* gpu = gpu_get_primary();
    if (!gpu) return;

    win->restore_x = win->x;
    win->restore_y = win->y;
    win->restore_w = win->w;
    win->restore_h = win->h;

    int new_w = (int)gpu->width;
    int new_h = (int)gpu->height - wm_reserved_bottom;
    if (new_w < 100) new_w = 100;
    if (new_h < 80) new_h = 80;

    win->x = 0;
    win->y = 0;
    if (surface_resize(win->surface, (uint32_t)new_w, (uint32_t)new_h, theme_get()->win_bg)) {
        win->w = new_w;
        win->h = new_h;
        if (win->on_resize) win->on_resize(win);
    }
    win->state = WIN_STATE_MAXIMIZED;
    wm_dirty = true;
}

static void wm_restore_from_max(window_t* win) {
    if (!win || !win->surface) return;
    win->x = win->restore_x;
    win->y = win->restore_y;
    if (surface_resize(win->surface, (uint32_t)win->restore_w, (uint32_t)win->restore_h, theme_get()->win_bg)) {
        win->w = win->restore_w;
        win->h = win->restore_h;
        if (win->on_resize) win->on_resize(win);
    }
    win->state = WIN_STATE_NORMAL;
    wm_dirty = true;
}

void wm_toggle_maximize(window_t* win) {
    if (!win) return;
    if (win->state == WIN_STATE_MINIMIZED) {
        wm_restore_window(win);
    }
    if (win->state == WIN_STATE_MAXIMIZED) {
        wm_restore_from_max(win);
    } else {
        wm_maximize_window(win);
    }
}

bool wm_resize_window(window_t* win, int w, int h) {
    if (!win || !win->surface) return false;
    if (w < 120) w = 120;
    if (h < 80) h = 80;
    if (!surface_resize(win->surface, (uint32_t)w, (uint32_t)h, theme_get()->win_bg)) return false;
    win->w = w;
    win->h = h;
    if (win->on_resize) win->on_resize(win);
    wm_dirty = true;
    return true;
}

static void rebuild_win_array(void) {
    win_count = 0;
    window_t* cur = windows_list;
    while (cur && win_count < MAX_WINDOWS) {
        win_array[win_count++] = cur;
        cur = cur->next;
    }
}

window_t* wm_create_window(surface_t* surface, int x, int y) {
    if (win_count >= MAX_WINDOWS) {
        serial("[WM] Error: Max windows reached\n");
        return NULL;
    }

    window_t* win = window_create(surface, x, y);
    if (!win) return NULL;

    win->id = next_win_id++;
    
    /* Add to list */
    win->next = windows_list;
    windows_list = win;
    
    rebuild_win_array();

    serial("[WM] Window created id=%u\n", win->id);

    wm_hooks_t* hooks = wm_get_hooks();
    if (hooks && hooks->on_window_create) {
        hooks->on_window_create(win);
    }

    wm_focus_window(win);
    wm_dirty = true;
    return win;
}

void wm_destroy_window(window_t* win) {
    if (!win) return;

    /* Remove from list */
    if (windows_list == win) {
        windows_list = win->next;
    } else {
        window_t* cur = windows_list;
        while (cur && cur->next != win) {
            cur = cur->next;
        }
        if (cur) {
            cur->next = win->next;
        }
    }

    rebuild_win_array();

    if (focused_window == win) {
        focused_window = windows_list; /* Focus first available or NULL */
        serial("[WM] Focus changed (auto)\n");
    }

    wm_hooks_t* hooks = wm_get_hooks();
    if (hooks && hooks->on_window_destroy) {
        hooks->on_window_destroy(win);
    }

    serial("[WM] Window destroyed id=%u\n", win->id);
    window_destroy(win);
    wm_dirty = true;
}

void wm_focus_window(window_t* win) {
    if (focused_window == win) return;

    window_t* old = focused_window;
    focused_window = win;

    /* Move focused window to front (head of list) */
    if (win && windows_list != win) {
        window_t* cur = windows_list;
        window_t* prev = NULL;
        while (cur && cur != win) {
            prev = cur;
            cur = cur->next;
        }
        if (cur) {
            if (prev) prev->next = cur->next;
            cur->next = windows_list;
            windows_list = cur;
            rebuild_win_array();
        }
    }

    serial("[WM] Focus changed\n");

    wm_hooks_t* hooks = wm_get_hooks();
    if (hooks && hooks->on_focus_change) {
        hooks->on_focus_change(old, win);
    }
    wm_dirty = true;
}

window_t* wm_get_focused(void) {
    return focused_window;
}

void wm_set_layout(wm_layout_t* layout) {
    if (layout) {
        current_layout = layout;
        serial("[WM] Layout set to %s\n", layout->name);
    }
    wm_dirty = true;
}

window_t* wm_find_window_at(int x, int y) {
    /* Iterate from Newest (Top) to Oldest (Bottom) */
    for (size_t i = 0; i < win_count; i++) {
        window_t* w = win_array[i];
        if (!w || !w->surface) continue;
        if (w->state == WIN_STATE_MINIMIZED) continue;
        if (!w->surface->visible) continue;
        
        if (x >= w->x && x < w->x + (int)w->surface->width &&
            y >= w->y && y < w->y + (int)w->surface->height) {
            return w;
        }
    }
    return NULL;
}

static void wm_draw_decorations(window_t* w, bool focused) {
    if (!w || !w->surface) return;
    if (!wm_window_is_decorated(w)) return;

    fly_theme_t* th = theme_get();
    int ww = (int)w->surface->width;
    int wh = (int)w->surface->height;
    int title_h = 24;

    uint32_t title_bg = focused ? th->win_title_active_bg : th->win_title_inactive_bg;
    uint32_t title_fg = focused ? th->win_title_active_fg : th->win_title_inactive_fg;

    fly_draw_rect_fill(w->surface, 0, 0, ww, title_h, title_bg);
    fly_draw_rect_fill(w->surface, 0, title_h, ww, 1, th->color_lo_1);

    const char* title = (w->title[0] != 0) ? w->title : "Chrysalis";
    fly_draw_text(w->surface, 6, 4, title, title_fg);

    int btn = 16;
    int pad = 4;
    int bx_close = ww - pad - btn;
    int bx_max = bx_close - pad - btn;
    int bx_min = bx_max - pad - btn;
    int by = 4;

    fly_draw_rect_fill(w->surface, bx_min, by, btn, btn, th->win_bg);
    fly_draw_rect_outline(w->surface, bx_min, by, btn, btn, th->color_lo_2);
    fly_draw_text(w->surface, bx_min + 5, by, "_", th->color_text);

    fly_draw_rect_fill(w->surface, bx_max, by, btn, btn, th->win_bg);
    fly_draw_rect_outline(w->surface, bx_max, by, btn, btn, th->color_lo_2);
    fly_draw_text(w->surface, bx_max + 4, by, "[]", th->color_text);

    fly_draw_rect_fill(w->surface, bx_close, by, btn, btn, th->win_bg);
    fly_draw_rect_outline(w->surface, bx_close, by, btn, btn, th->color_lo_2);
    fly_draw_text(w->surface, bx_close + 4, by, "X", th->color_text);

    /* Resize grip */
    if ((w->flags & WIN_FLAG_NO_RESIZE) == 0) {
        for (int i = 0; i < 8; i++) {
            fly_draw_rect_fill(w->surface, ww - 1 - i, wh - 1, 1, 1, th->color_hi_1);
            fly_draw_rect_fill(w->surface, ww - 1, wh - 1 - i, 1, 1, th->color_hi_1);
        }
    }
}

void wm_render(void) {
    if (!wm_dirty && !terminal_is_dirty()) return;

    /* 1. Apply Layout */
    if (current_layout && current_layout->apply) {
        current_layout->apply(win_array, win_count);
    }

    /* 2. Draw decorations before render */
    for (size_t i = 0; i < win_count; i++) {
        window_t* w = win_array[i];
        if (!w || !w->surface) continue;
        if (w->state == WIN_STATE_MINIMIZED || !w->surface->visible) continue;
        wm_draw_decorations(w, w == focused_window);
    }

    /* 3. Prepare Surface List for Compositor (Bottom to Top) */
    surface_t* render_list[MAX_WINDOWS];
    int render_count = 0;

    /* Iterate backwards (Oldest/Bottom -> Newest/Top) for Painter's Algorithm */
    for (int i = (int)win_count - 1; i >= 0; i--) {
        window_t* w = win_array[i];
        if (w->surface) {
            w->surface->x = w->x;
            w->surface->y = w->y;
            render_list[render_count++] = w->surface;
        }
    }

    /* 4. Hooks */
    wm_hooks_t* hooks = wm_get_hooks();
    if (hooks && hooks->on_frame) {
        hooks->on_frame();
    }

    /* 5. Render */
    serial("[WM] Rendering\n");
    mouse_blit_start();
    compositor_render_surfaces(render_list, render_count);
    mouse_blit_end();
    
    wm_dirty = false;
    terminal_clear_dirty();
}
