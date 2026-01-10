#pragma once
#include "../ui/wm/window.h"
#include "../input/input.h"

#ifdef __cplusplus
extern "C" {
#endif
void run_dialog_app_create(void);
bool run_dialog_app_handle_event(input_event_t* ev);
void run_dialog_app_handle_key(char c);
window_t* run_dialog_app_get_window(void);
#ifdef __cplusplus
}
#endif