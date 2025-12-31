#include "irq.h"
#include "../../include/ports.h"
#include "../drivers/keyboard.h"
#include "../drivers/pic.h"

extern "C" void irq_handler() {
    uint8_t scancode = inb(0x60);

    keyboard_handle(scancode);

    pic_send_eoi(1);
}
