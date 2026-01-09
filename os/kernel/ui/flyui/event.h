#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FLY_EVENT_MOUSE_MOVE,
    FLY_EVENT_MOUSE_DOWN,
    FLY_EVENT_MOUSE_UP,
    FLY_EVENT_KEY_DOWN,
    FLY_EVENT_KEY_UP
} fly_event_type_t;

typedef struct {
    fly_event_type_t type;
    int mx, my;
    int button;
    uint32_t keycode;
} fly_event_t;

#ifdef __cplusplus
}
#endif