#include "widget.h"
#include "../../mem/kmalloc.h"
#include <stddef.h>

fly_widget_t* fly_widget_create(void) {
    fly_widget_t* w = (fly_widget_t*)kmalloc(sizeof(fly_widget_t));
    if (!w) return NULL;
    
    w->x = 0; w->y = 0; w->w = 10; w->h = 10;
    w->parent = NULL;
    w->first_child = NULL;
    w->next_sibling = NULL;
    w->on_draw = NULL;
    w->on_event = NULL;
    w->userdata = NULL;
    w->internal_data = NULL;
    w->bg_color = 0xFF000000;
    w->fg_color = 0xFFFFFFFF;
    w->needs_redraw = true;
    
    return w;
}

void fly_widget_destroy(fly_widget_t* w) {
    if (!w) return;
    
    /* Destroy children */
    fly_widget_t* c = w->first_child;
    while (c) {
        fly_widget_t* next = c->next_sibling;
        fly_widget_destroy(c);
        c = next;
    }
    
    if (w->internal_data) kfree(w->internal_data);
    kfree(w);
}

void fly_widget_add(fly_widget_t* parent, fly_widget_t* child) {
    if (!parent || !child) return;
    
    child->parent = parent;
    /* Prepend to list (simplest) */
    child->next_sibling = parent->first_child;
    parent->first_child = child;
}