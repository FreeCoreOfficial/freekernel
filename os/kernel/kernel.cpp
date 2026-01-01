#include <stdint.h>
#include <stddef.h>

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
#include "video/vga.h"

/* Dacă shell.h nu declară shell_poll_input(), avem o declarație locală ca fallback */
#ifdef __cplusplus
extern "C" {
#endif
void shell_poll_input(void);
#ifdef __cplusplus
}
#endif

/* Multiboot magic constant (classic Multiboot 1) */
#define MULTIBOOT_MAGIC 0x2BADB002u

/* Debug: activează testul VGA pe care l-ai avut anterior */
//#define VGA_TEST 0
#define VGA_TEST 1

/* Minimal Multiboot info struct with fields we need.
   This is intentionally minimal — it only declares the fields we will read.
   It mirrors the layout used by many Multiboot-compatible bootloaders/GRUB.
   If you use a different multiboot variant, update this struct accordingly. */
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    /* Framebuffer fields - common layout when GRUB provides them */
    uint64_t framebuffer_addr;   // 64-bit address for safety
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    /* padding to keep alignment (not used) */
} multiboot_info_t;

/* Helper: initialize vga framebuffer from multiboot info if present */
static void try_init_framebuffer_from_multiboot(uint32_t magic, uint32_t addr)
{
    if (magic != MULTIBOOT_MAGIC) {
        /* Not multiboot or unknown magic — nothing to do */
        return;
    }

    if (addr == 0) return;

    multiboot_info_t* mbi = (multiboot_info_t*)(uintptr_t)addr;

    /* bit 12 indicates framebuffer info present (commonly used) */
    if (mbi->flags & (1 << 12)) {
        void* fb_addr = (void*)(uintptr_t)mbi->framebuffer_addr;
        uint32_t fb_width = mbi->framebuffer_width;
        uint32_t fb_height = mbi->framebuffer_height;
        uint32_t fb_pitch = mbi->framebuffer_pitch;
        uint8_t  fb_bpp = mbi->framebuffer_bpp;

        /* Only call vga_set_framebuffer if values look sane */
        if (fb_addr && fb_width > 0 && fb_height > 0 && fb_pitch > 0 && fb_bpp > 0) {
            vga_set_framebuffer(fb_addr, fb_width, fb_height, fb_pitch, fb_bpp);
        }
    }
}

extern "C" void kernel_main(uint32_t magic, uint32_t addr) {

#if VGA_TEST
    /* If GRUB passed a framebuffer, try to use it (vga_set_framebuffer called above). */
    try_init_framebuffer_from_multiboot(magic, addr);

    /* Initialize our VGA driver (does NOT call BIOS). 
       If no framebuffer was provided by bootloader, vga_init() falls back to 0xA0000. */
    vga_init();
    vga_clear(0); // culoare 0 = negru

    /* desenăm o diagonală de test (culoare 15 = alb / 32bpp grayscale) */
    for (int i = 0; i < 200 && i < 320; i++) {
        vga_putpixel(i, i, 15);
    }

    /* desenăm și un chenar simplu pentru a confirma desenul pe ecran */
    for (int x = 10; x < 310; x++) {
        vga_putpixel(x, 10, 14);
        vga_putpixel(x, 190, 14);
    }
    for (int y = 10; y < 190; y++) {
        vga_putpixel(10, y, 14);
        vga_putpixel(310, y, 14);
    }

    /* rămânem blocați aici pentru a putea verifica vizual rezultatul în QEMU */
    for (;;)
        asm volatile("hlt");

#else
    /* Revenire la inițializările normale (originale) */

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
#endif
}
