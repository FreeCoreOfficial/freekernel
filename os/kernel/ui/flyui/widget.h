#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "event.h"
#include "../../video/surface.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fly_widget fly_widget_t;

typedef void (*fly_draw_func)(fly_widget_t* w, surface_t* surf, int x_off, int y_off);
typedef bool (*fly_event_func)(fly_widget_t* w, fly_event_t* e);

struct fly_widget {
    int x, y, w, h;
    
    fly_widget_t* parent;
    fly_widget_t* first_child;
    fly_widget_t* next_sibling;
    
    fly_draw_func on_draw;
    fly_event_func on_event;
    
    void* userdata;
    void* internal_data;
    
    uint32_t bg_color;
    uint32_t fg_color;
    
    bool needs_redraw;
};

fly_widget_t* fly_widget_create(void);
void fly_widget_destroy(fly_widget_t* w);
void fly_widget_add(fly_widget_t* parent, fly_widget_t* child);

#ifdef __cplusplus
}
#endif