#include <stdint.h>
#include "keyboard.h"

#include "../interrupts/irq.h"
#include "../shell/shell.h"
#include "../shortcuts/shortcuts.h"

static int ctrl_pressed = 0;

static const char keymap[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',
    0,'*',0,' '
};

/* trimite EOI la PIC */
static inline void pic_send_eoi(void) {
    uint8_t val = 0x20;
    uint16_t port = 0x20;
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

extern "C" void keyboard_handler(registers_t* regs)
{
    (void)regs;

    uint8_t scancode;
    asm volatile("inb %1, %0" : "=a"(scancode) : "Nd"(0x60));

    bool handled = false;

    /* CTRL press / release */
    if (scancode == 0x1D) {          // Ctrl down
        ctrl_pressed = 1;
        handled = true;
    } else if (scancode == 0x9D) {   // Ctrl up
        ctrl_pressed = 0;
        handled = true;
    } else if (scancode & 0x80) {    // ignore releases for other keys
        handled = true;
    } else {
        /* notify by character when a printable char results from this scancode */
        char c = keymap[scancode];
        if (c) {
            /* notify shortcuts about the produced character (ex: 'l') */
            shortcuts_notify_char(c);
        }

        /* Ctrl + key shortcuts (if Ctrl is held) */
        if (ctrl_pressed) {
            if (shortcuts_handle_ctrl(scancode)) {
                handled = true;
            }
        }

        /* normal input */
        if (!handled) {
            if (c) {
                shell_handle_char(c);
                handled = true;
            }
        }
    }

    /* manual EOI pentru PIC (dacă wrapper-ul tău deja trimite EOI, poți elimina) */
    pic_send_eoi();

    (void)handled;
    return;
}

extern "C" void keyboard_init()
{
    irq_install_handler(1, keyboard_handler);
}
