#include "wm_layout.h"

static void layout_floating_apply(window_t** wins, size_t count) {
    /* Floating layout does nothing, windows stay where they are */
    (void)wins;
    (void)count;
}

wm_layout_t wm_layout_floating = {
    .name = "floating",
    .apply = layout_floating_apply
};