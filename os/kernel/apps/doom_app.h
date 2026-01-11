#pragma once
#include "../ui/wm/window.h"
#include "../input/input.h"

#ifdef __cplusplus
extern "C" {
#endif

void doom_app_create(void);
void doom_app_update(void);
bool doom_app_handle_event(input_event_t* ev);
window_t* doom_app_get_window(void);

#ifdef __cplusplus
}
#endif