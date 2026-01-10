#pragma once
#include "../ui/wm/window.h"
#include "../input/input.h"

#ifdef __cplusplus
extern "C" {
#endif

void notepad_app_create(void);
bool notepad_app_handle_event(input_event_t* ev);
void notepad_app_handle_key(char c);
window_t* notepad_app_get_window(void);

#ifdef __cplusplus
}
#endif