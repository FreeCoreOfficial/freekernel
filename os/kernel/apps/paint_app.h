#pragma once
#include "../ui/wm/window.h"
#include "../input/input.h"

#ifdef __cplusplus
extern "C" {
#endif
void paint_app_create(void);
bool paint_app_handle_event(input_event_t* ev);
window_t* paint_app_get_window(void);
#ifdef __cplusplus
}
#endif