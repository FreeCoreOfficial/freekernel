#include "terminal.h"
#include "drivers/pic.h"
#include "arch/i386/gdt.h"
#include "arch/i386/idt.h"
#include "shell/shell.h"
#include "fs/fs.h"
#include "bootlogo/bootlogo.h"
#include "drivers/pit.h"


extern "C" void kernel_main() {
    gdt_init();
    idt_init();
    pic_remap();

    terminal_init();
    bootlogo_show();
    shell_init();
    fs_init();

    terminal_writestring("Chrysalis OS\n");
    terminal_writestring("Type commands below:\n> ");

    asm volatile("sti");

    pit_init(100); // 100 Hz

    uint64_t last_tick = 0;

    while (1) {
        uint64_t t = pit_get_ticks();

        if (t != last_tick && t % 100 == 0) {
            terminal_writestring(".");
            last_tick = t;
        }

        asm volatile("hlt");
    }
}
