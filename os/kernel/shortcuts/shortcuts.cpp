#include "shortcuts.h"

#include "../terminal.h"
#include "../shell/shell.h"

/* scancodes */
#define SC_C 0x2E
#define SC_L 0x26
#define SC_D 0x20

static volatile bool ctrl_c_pending = false;

bool shortcuts_handle_ctrl(uint8_t scancode)
{
    switch (scancode) {

        case SC_C:
            ctrl_c_pending = true;
            terminal_writestring("^C\n");
            shell_reset_input();
            shell_prompt();
            return true;

        case SC_L:
            terminal_clear();
            shell_prompt();
            return true;

        case SC_D:
            terminal_writestring("^D\n");
            return true;

        default:
            return false;
    }
}

bool shortcuts_poll_ctrl_c(void)
{
    if (ctrl_c_pending) {
        ctrl_c_pending = false;
        return true;
    }
    return false;
}
