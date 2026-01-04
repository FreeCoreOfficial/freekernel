#include "syscall.h"
#include "idt.h"
#include "../../terminal.h"
#include "../../panic.h"

/* handler ASM */
extern void syscall_handler();

void syscall_init()
{
    /* int 0x80 — syscall gate, DPL=3 */
    idt_set_gate(
        0x80,
        (uint32_t)syscall_handler,
        0x08,      // kernel code selector
        0xEE       // present | ring 3 | 32-bit interrupt gate
    );

    terminal_writestring("[syscall] int 0x80 installed\n");
}
