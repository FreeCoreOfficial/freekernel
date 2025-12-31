#include "drivers/pic.h"
#include "arch/i386/gdt.h"
#include "arch/i386/idt.h"

extern "C" void kernel_main()
{
    gdt_init();
    idt_init();
    pic_remap();

    asm volatile("sti");

    while (1);
}
