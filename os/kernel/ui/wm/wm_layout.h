#pragma once
#include "window.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wm_layout {
    const char* name;
    void (*apply)(window_t** wins, size_t count);
} wm_layout_t;

extern wm_layout_t wm_layout_floating;

#ifdef __cplusplus
}
#endif