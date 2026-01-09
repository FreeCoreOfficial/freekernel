#pragma once
#include "../../video/surface.h"
#include "widget.h"
#include "event.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct flyui_context {
    surface_t* surface;
    fly_widget_t* root;
} flyui_context_t;

flyui_context_t* flyui_init(surface_t* surface);
void flyui_set_root(flyui_context_t* ctx, fly_widget_t* root);
void flyui_render(flyui_context_t* ctx);
void flyui_dispatch_event(flyui_context_t* ctx, fly_event_t* event);

#ifdef __cplusplus
}
#endif