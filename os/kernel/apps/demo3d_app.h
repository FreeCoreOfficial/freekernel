#pragma once
#include "../ui/wm/window.h"
#include "../input/input.h"

#ifdef __cplusplus
extern "C" {
#endif
void demo3d_app_create(void);
bool demo3d_app_handle_event(input_event_t* ev);
void demo3d_app_update(void);
window_t* demo3d_app_get_window(void);
#ifdef __cplusplus
}
#endif