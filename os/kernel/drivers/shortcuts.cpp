#include "shortcuts.h"

static int ctrl_down = 0;
static int ctrl_c_pressed = 0;

void shortcuts_handle_scancode(uint8_t scancode)
{
    // key release
    if (scancode & 0x80) {
        uint8_t key = scancode & 0x7F;
        if (key == 0x1D) // CTRL
            ctrl_down = 0;
        return;
    }

    // key press
    if (scancode == 0x1D) { // CTRL
        ctrl_down = 1;
        return;
    }

    if (ctrl_down && scancode == 0x2E) { // C
        ctrl_c_pressed = 1;
    }
}

int shortcut_ctrl_c()
{
    if (ctrl_c_pressed) {
        ctrl_c_pressed = 0;
        return 1;
    }
    return 0;
}
