#include "keymaps.h"

/* forward declarations of per-map init functions (if any) */
void keymap_ro_init(void);

void keymaps_init(void)
{
    keymap_ro_init();
    /* if more keymaps need runtime init, call them here */
}
