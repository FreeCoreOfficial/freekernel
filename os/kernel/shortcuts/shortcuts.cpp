#include "shortcuts.h"

#include "../terminal.h"
#include "../shell/shell.h"

#define SC_C 0x2E
#define SC_L 0x26
#define SC_D 0x20
#define SC_Z 0x2C

/* latch pentru Ctrl+C */
static volatile bool ctrl_c_pending = false;
/* latch pentru tasta 'l' mică */
static volatile bool key_l_pending = false;

void shortcuts_init(void)
{
    ctrl_c_pending = false;
    key_l_pending = false;
}

bool shortcuts_handle_ctrl(uint8_t scancode)
{
    switch (scancode) {

        case SC_C:
            terminal_writestring("^C\n");
            ctrl_c_pending = true;
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

        case SC_Z:
            terminal_writestring("^Z\n");
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

void shortcuts_clear_ctrl_c(void)
{
    ctrl_c_pending = false;
}

/* ---------------------
   Character notification API (for plain keys like lowercase 'l')
   --------------------- */

void shortcuts_notify_char(char c)
{
    /* setăm latch doar când primește litera mică 'l' */
    if (c == 'l') {
        key_l_pending = true;
    }
}

bool shortcuts_poll_key_l(void)
{
    if (key_l_pending) {
        key_l_pending = false;
        return true;
    }
    return false;
}

void shortcuts_clear_key_l(void)
{
    key_l_pending = false;
}
