#include <stdint.h>
#include "keyboard.h"

#include "../interrupts/irq.h"
#include "../shell/shell.h"
#include "../shortcuts/shortcuts.h"

/* keymap US basic */
static const char keymap[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',
    0,'*',0,' '
};

/* stare modificatori */
static int ctrl_pressed = 0;

extern "C" void keyboard_handler(registers_t* regs)
{
    (void)regs;

    uint8_t scancode;
    asm volatile("inb %1, %0" : "=a"(scancode) : "Nd"(0x60));

    /* ===============================
       CTRL press / release
       =============================== */

    if (scancode == 0x1D) {          // Left Ctrl pressed
        ctrl_pressed = 1;
        return;
    }

    if (scancode == 0x9D) {          // Left Ctrl released
        ctrl_pressed = 0;
        return;
    }

    /* ===============================
       key release → ignorăm
       =============================== */
    if (scancode & 0x80)
        return;

    /* ===============================
       shortcuts (Ctrl + key)
       =============================== */
    if (ctrl_pressed) {
        if (shortcuts_handle_ctrl(scancode)) {
            return; // shortcut consumat
        }
    }

    /* ===============================
       input normal
       =============================== */
    char c = keymap[scancode];
    if (c) {
        shell_handle_char(c);
    }
}

extern "C" void keyboard_init()
{
    irq_install_handler(1, keyboard_handler);
}
