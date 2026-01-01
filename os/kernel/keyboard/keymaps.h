#pragma once

/* existing externs */
extern const keymap_t* keymap_us_ptr;
extern const keymap_t* keymap_ro_ptr;

/* call once at boot to initialise runtime-filled keymaps */
void keymaps_init(void);
