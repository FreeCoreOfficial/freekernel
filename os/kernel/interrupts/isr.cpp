#include <stdint.h>
#include "../terminal.h"

extern "C" void isr_handler()
{
    terminal_writestring("INTERRUPT\n");
    while (1) {
        asm volatile("hlt");
    }
}
