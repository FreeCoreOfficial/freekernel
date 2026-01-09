#include "widgets.h"
#include "../draw.h"
#include "../theme.h"
#include "../../../mem/kmalloc.h"
#include "../../../string.h"

extern void serial(const char *fmt, ...);

typedef struct {
    char* text;
} button_data_t;

static void button_draw(fly_widget_t* w, surface_t* surf, int x, int y) {
    /* Windows 1.0 Style: White background, Black border */
    fly_draw_rect_fill(surf, x, y, w->w, w->h, w->bg_color);
    fly_draw_rect_outline(surf, x, y, w->w, w->h, FLY_COLOR_BORDER);
    /* Double border for "3D" effect (flat in Win1.0 but distinct) */
    fly_draw_rect_outline(surf, x + 2, y + 2, w->w - 4, w->h - 4, FLY_COLOR_BORDER);
    
    /* Draw text centered */
    button_data_t* d = (button_data_t*)w->internal_data;
    if (d && d->text) {
        int text_w = strlen(d->text) * 8;
        int tx = x + (w->w - text_w) / 2;
        int ty = y + (w->h - 16) / 2;
        fly_draw_text(surf, tx, ty, d->text, w->fg_color);
    }
}

static bool button_event(fly_widget_t* w, fly_event_t* e) {
    if (e->type == FLY_EVENT_MOUSE_DOWN) {
        button_data_t* d = (button_data_t*)w->internal_data;
        serial("[FLYUI] button clicked: %s\n", d ? d->text : "???");
        return true;
    }
    return false;
}

fly_widget_t* fly_button_create(const char* text) {
    fly_widget_t* w = fly_widget_create();
    button_data_t* d = (button_data_t*)kmalloc(sizeof(button_data_t));
    if (d) {
        d->text = (char*)kmalloc(strlen(text) + 1);
        strcpy(d->text, text);
        w->internal_data = d;
    }
    w->on_draw = button_draw;
    w->on_event = button_event;
    w->bg_color = FLY_COLOR_BUTTON;
    w->fg_color = FLY_COLOR_TEXT;
    return w;
}
