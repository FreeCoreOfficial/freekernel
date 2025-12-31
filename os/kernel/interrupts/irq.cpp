#include "../drivers/pic.h"
#include "../terminal.h"
#include "../drivers/pic.h"

extern "C" void irq_handler(int irq)
extern "C" void keyboard_handler();

{
    if (irq == 1) {
        terminal_writestring("KEY\n");
    }

    pic_send_eoi(irq);
     keyboard_handler();
}
