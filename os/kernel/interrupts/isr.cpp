// kernel/interrupts/isr.cpp
#include "isr.h"
#include "../terminal.h"
#include "../panic.h"
#include <stdint.h>

extern "C" void isr_handler(registers_t* r)
{
    // Print basic info
    terminal_writestring("\n*** KERNEL PANIC: Unhandled CPU exception ***\n");
    terminal_writestring("Interrupt: ");
    // print decimal int_no
    {
        uint32_t v = r->int_no;
        // simple itoa decimal
        char buf[12];
        int idx = 0;
        if (v == 0) { terminal_putchar('0'); } else {
            while (v) { buf[idx++] = '0' + (v % 10); v /= 10; }
            while (idx--) terminal_putchar(buf[idx]);
        }
        terminal_writestring("\n");
    }

    terminal_writestring("Error code: ");
    // hex print 32-bit
    {
        const char* hex = "0123456789ABCDEF";
        for (int i = 7; i >= 0; --i) {
            terminal_putchar(hex[(r->err_code >> (i*4)) & 0xF]);
        }
        terminal_writestring("\n");
    }

    // Optionally dump some registers (EIP/CS/EFLAGS)
    terminal_writestring("EIP: ");
    {
        const char* hex = "0123456789ABCDEF";
        uint32_t v = r->eip;
        for (int i = 7; i >= 0; --i) terminal_putchar(hex[(v >> (i*4)) & 0xF]);
    }
    terminal_writestring("\n");

    // Call panic (this should never return)
    panic("Unhandled CPU exception");
}
