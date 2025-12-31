#include "../drivers/pic.h"
#include "../terminal.h"

extern "C" void keyboard_handler();

/* handler apelat din ASM */
extern "C" void irq_handler(int irq)
{
    if (irq == 1) {
        keyboard_handler();
    }

    pic_send_eoi(irq);
}
