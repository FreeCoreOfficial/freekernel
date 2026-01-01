#include "terminal.h"
#include "drivers/pic.h"
#include "arch/i386/gdt.h"
#include "arch/i386/idt.h"
#include "drivers/keyboard.h"
#include "drivers/pit.h"
#include "shell/shell.h"
#include "fs/fs.h"
#include "bootlogo/bootlogo.h"
// #include "debug/load.h"   // dezactivat temporar
#include "drivers/serial.h"
#include "drivers/rtc.h"
#include "time/clock.h"
#include "drivers/audio/audio.h"
#include "drivers/mouse.h"
#include "input/keyboard_buffer.h" // pentru kbd_buffer_init()

/* Dacă shell.h nu declară shell_poll_input(), avem o declarație locală ca fallback */
#ifdef __cplusplus
extern "C" {
#endif
void shell_poll_input(void);
#ifdef __cplusplus
}
#endif

extern "C" void kernel_main() {

    // load_begin("GDT");
    gdt_init();
    // load_ok();

    // load_begin("IDT");
    idt_init();
    // load_ok();

    // load_begin("PIC");
    pic_remap();
    // load_ok();

    // load_begin("Terminal");
    terminal_init();
    // load_ok();

    // load_begin("Boot logo");
    bootlogo_show();
    // load_ok();

    // load_begin("Filesystem");
    fs_init();
    // load_ok();

    // load_begin("Shell");
    shell_init();
    // load_ok();

    // init keyboard buffer BEFORE keyboard init
    kbd_buffer_init();

    // load_begin("Keyboard IRQ");
    keyboard_init();
    // load_ok();

    // load_begin("PIT");
    pit_init(100);
    // load_ok();

    audio_init();
    mouse_init();

    // serial debug (optional)
    serial_init();
    serial_write_string("=== Chrysalis OS serial online ===\r\n");

    asm volatile("sti");

    terminal_writestring("\nSystem ready.\n> ");

    rtc_print();

    time_init();
    time_set_timezone(2);

    datetime t;
    time_get_local(&t);

    /* main loop: procesăm input din buffer, apoi idle */
    while (1) {
        shell_poll_input();
        asm volatile("hlt");
    }
}
