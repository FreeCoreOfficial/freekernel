#pragma once
#include "../ui/wm/window.h"
#include "../input/input.h"

#ifdef __cplusplus
extern "C" {
#endif
void image_viewer_app_create(const char* path);
bool image_viewer_app_handle_event(input_event_t* ev);
window_t* image_viewer_app_get_window(void);
#ifdef __cplusplus
}
#endif