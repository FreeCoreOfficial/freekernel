#include "interrupts.h"
#include "../../drivers/pic.h"
#include "../../terminal.h"

void irq_stub_handler(struct regs* r)
{
    terminal_writestring("[irq] unhandled irq ");
    terminal_writehex(r->int_no);
    terminal_writestring("\n");

    /* trimite EOI la PIC */
    pic_send_eoi(r->int_no - 32);
}
