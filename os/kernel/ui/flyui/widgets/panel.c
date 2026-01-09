#include "widgets.h"
#include "../draw.h"
#include "../theme.h"

static void panel_draw(fly_widget_t* w, surface_t* surf, int x, int y) {
    fly_draw_rect_fill(surf, x, y, w->w, w->h, w->bg_color);
    fly_draw_rect_outline(surf, x, y, w->w, w->h, FLY_COLOR_BORDER);
}

fly_widget_t* fly_panel_create(int w, int h) {
    fly_widget_t* widget = fly_widget_create();
    widget->w = w;
    widget->h = h;
    widget->on_draw = panel_draw;
    widget->bg_color = FLY_COLOR_PANEL;
    return widget;
}
