#include "idt.h"

extern "C" void idt_load(uint32_t);
extern "C" void isr_stub();

static IDTEntry idt[256];
static IDTPointer idtp;

static void set_gate(int n, uint32_t handler)
{
    idt[n].base_low  = handler & 0xFFFF;
    idt[n].base_high = (handler >> 16) & 0xFFFF;
    idt[n].sel = 0x08;
    idt[n].always0 = 0;
    idt[n].flags = 0x8E;
}

extern "C" void idt_init()
{
    idtp.limit = sizeof(IDTEntry) * 256 - 1;
    idtp.base  = (uint32_t)&idt;

    for (int i = 0; i < 256; i++)
        set_gate(i, (uint32_t)isr_stub);

    idt_load((uint32_t)&idtp);
}
