#include "wm_hooks.h"
#include <stddef.h>

static wm_hooks_t* g_hooks = NULL;

void wm_register_hooks(wm_hooks_t* hooks) {
    g_hooks = hooks;
}

wm_hooks_t* wm_get_hooks(void) {
    return g_hooks;
}