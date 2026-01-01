#include "keymap.h"

const keymap_t* current_keymap = 0;

void keymap_set(const keymap_t* map) {
    current_keymap = map;
}

uint16_t keymap_translate(uint8_t scancode, int shift, int altgr)
{
    if (!current_keymap)
        return 0;

    if (scancode >= 128)
        return 0;

    /* AltGr has priority, but may fall back */
    if (altgr) {
        uint16_t c = current_keymap->altgr[scancode];
        if (c)
            return c;
    }

    /* Shift fallback */
    if (shift) {
        uint16_t c = current_keymap->shift[scancode];
        if (c)
            return c;
    }

    /* Normal */
    return current_keymap->normal[scancode];
}
