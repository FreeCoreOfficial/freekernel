#include "irq.h"
#include "../drivers/keyboard.h"
#include "../../include/ports.h"

void irq_handler() {
    uint8_t scancode = inb(0x60);

    keyboard_handle(scancode);

    outb(0x20, 0x20); // EOI
}
