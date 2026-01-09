#pragma once
#include "window.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wm_hooks {
    void (*on_window_create)(window_t*);
    void (*on_window_destroy)(window_t*);
    void (*on_focus_change)(window_t* old_focus, window_t* new_focus);
    void (*on_frame)(void);
} wm_hooks_t;

void wm_register_hooks(wm_hooks_t* hooks);
wm_hooks_t* wm_get_hooks(void);

#ifdef __cplusplus
}
#endif