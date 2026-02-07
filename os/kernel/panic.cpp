// kernel/panic.cpp

#include "panic.h"
#include "panic_sys.h"
#include "arch/i386/tss.h"
#include "video/framebuffer.h"
#include "video/font8x16.h"
#include "video/gpu.h"
#include "arch/i386/io.h"
// === Freestanding headers (fără libc) ===
typedef unsigned int       size_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef signed int         int32_t;

// === Fallback strlen sigur ===
static size_t strlen(const char* str) {
    const char* s = str;
    while (*s) ++s;
    return s - str;
}

// === VGA Text Mode ===
#define VGA_MEM ((volatile uint16_t*)0xB8000)
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

// === Chrysalis Panic Palette (distinct from XP) ===
#define CHR_BG      0   // Black
#define CHR_FG      15  // White
#define CHR_ACC_BG  3   // Dark cyan
#define CHR_ACC_FG  15  // White
#define CHR_ATTR    ((CHR_BG << 4) | (CHR_FG & 0x0F))
#define CHR_ACC_ATTR ((CHR_ACC_BG << 4) | (CHR_ACC_FG & 0x0F))

// === Framebuffer Colors (ARGB) ===
#define FB_BG       0x0012171D // deep slate
#define FB_TEXT     0x00F0F3F7 // near white
#define FB_ACC      0x0028C7B7 // teal accent

// === Serial debug (COM1) ===
static void serial_print(const char* s) {
    while (*s) {
        while (!(inb(0x3F8 + 5) & 0x20));
        outb(0x3F8, *s++);
    }
}

// === Framebuffer Helpers ===
static bool use_fb = false;
static uint32_t fb_w = 0, fb_h = 0;

/* Use GPU subsystem instead of raw framebuffer */
static void check_fb() {
    gpu_device_t* gpu = gpu_get_primary();
    if (gpu && gpu->virt_addr) {
        use_fb = true;
        fb_w = gpu->width;
        fb_h = gpu->height;
    } else {
        use_fb = false;
    }
}

static uint32_t fb_text_color = FB_TEXT;
static uint8_t vga_text_attr = CHR_ATTR;

static void fb_draw_char_color(int x, int y, char c, uint32_t color) {
    gpu_device_t* gpu = gpu_get_primary();
    if (!gpu) return;

    const uint8_t* glyph = &font8x16[(uint8_t)c * 16];
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 8; j++) {
            if (glyph[i] & (1 << (7-j))) {
                gpu->ops->putpixel(gpu, x + j, y + i, color);
            }
        }
    }
}

// === Primitivă de scriere caracter ===
static void put_char(int x, int y, char c) {
    if (use_fb) {
        // Pe FB folosim coordonate pixel (8x16 font)
        fb_draw_char_color(x * 8, y * 16, c, fb_text_color);
    } else {
        // Pe VGA folosim coordonate text
        if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT)
            VGA_MEM[y * VGA_WIDTH + x] = ((uint16_t)vga_text_attr << 8) | (uint8_t)c;
    }
}

// === Scrie string centrat sau la poziție fixă ===
static void draw_string(int x, int y, const char* s) {
    while (*s) {
        put_char(x++, y, *s++);
    }
}

static void set_text_color(uint32_t fb_color, uint8_t vga_attr) {
    fb_text_color = fb_color;
    vga_text_attr = vga_attr;
}

static void fb_fill_rect(int x, int y, int w, int h, uint32_t color) {
    gpu_device_t* gpu = gpu_get_primary();
    if (!gpu) return;
    if (gpu->ops && gpu->ops->fillrect) {
        gpu->ops->fillrect(gpu, x, y, w, h, color);
        return;
    }
    for (int yy = 0; yy < h; yy++) {
        for (int xx = 0; xx < w; xx++) {
            gpu->ops->putpixel(gpu, x + xx, y + yy, color);
        }
    }
}

static void draw_string_center(int y, const char* s) __attribute__((unused));
static void draw_string_center(int y, const char* s) {
    size_t len = strlen(s);
    int x = (VGA_WIDTH - (int)len) / 2;
    draw_string(x, y, s);
}

// === Umple tot ecranul cu fundal albastru + spații (autentic XP) ===
static void clear_screen_chrysalis() {
    if (use_fb) {
        gpu_device_t* gpu = gpu_get_primary();
        if (gpu) gpu->ops->clear(gpu, FB_BG);
    } else {
        for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
            VGA_MEM[i] = ((uint16_t)CHR_ATTR << 8) | ' ';
        }
    }
}

// === Convert u32 → string (fără diviziune libc) ===
static void u32_to_hex(uint32_t value, char* buf) {
    const char* hex = "0123456789ABCDEF";
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 9; i >= 2; i--) {
        buf[i] = hex[value & 0xF];
        value >>= 4;
    }
    buf[10] = 0;
}

static void u32_to_dec(uint32_t value, char* buf) __attribute__((unused));
static void u32_to_dec(uint32_t value, char* buf) {
    char temp[12];
    int i = 0;
    if (value == 0) {
        buf[0] = '0'; buf[1] = 0;
        return;
    }
    while (value) {
        temp[i++] = (value % 10) + '0';
        value /= 10;
    }
    int j = 0;
    while (i > 0) buf[j++] = temp[--i];
    buf[j] = 0;
}

extern "C" void panic_render_pretty(const char* msg) {
    check_fb(); // Detectăm modul video
    clear_screen_chrysalis();

    /* Accent header bar */
    if (use_fb) {
        fb_fill_rect(0, 0, (int)fb_w, 20, FB_ACC);
        set_text_color(FB_TEXT, CHR_ATTR);
    } else {
        set_text_color(FB_TEXT, CHR_ACC_ATTR);
        for (int x = 0; x < VGA_WIDTH; x++) {
            put_char(x, 0, ' ');
        }
        set_text_color(FB_TEXT, CHR_ATTR);
    }

    int line = 2;
    int indent = 2;

    draw_string(indent, line++, "Chrysalis OS - Kernel Panic");
    line++;

    draw_string(indent, line++, "Reason:");
    if (msg && msg[0]) {
        char buffer[76];
        const char* p = msg;
        while (*p) {
            int i = 0;
            while (*p && *p != '\n' && i < 75) {
                buffer[i++] = *p++;
            }
            buffer[i] = 0;
            draw_string(indent + 2, line++, buffer);
            if (*p == '\n') p++;
        }
    } else {
        draw_string(indent + 2, line++, "Unknown fatal error");
    }

    line++;
    draw_string(indent, line++, "System:");
    const char* cpu = panic_sys_cpu_str();
    draw_string(indent + 2, line, "CPU: ");
    draw_string(indent + 7, line++, cpu ? cpu : "unknown");

    char numbuf[12];
    draw_string(indent + 2, line, "RAM: ");
    u32_to_dec(panic_sys_total_ram_kb() / 1024, numbuf);
    draw_string(indent + 7, line, numbuf);
    draw_string(indent + 7 + (int)strlen(numbuf), line, " MB");
    line++;

    draw_string(indent + 2, line, "Uptime: ");
    u32_to_dec(panic_sys_uptime_seconds(), numbuf);
    draw_string(indent + 10, line++, numbuf);

    line++;
    draw_string(indent, line++, "Press M to reboot, or power off to halt.");

    // Protecție contra depășire (șterge jos dacă e cazul)
    if (!use_fb) {
        for (int y = line; y < VGA_HEIGHT; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                put_char(x, y, ' ');
            }
        }
    }
}

// === Reboot forțat ===
extern "C" void try_reboot() {
    // 1. 8042 Keyboard Controller Pulse
    outb(0x64, 0xFE);
    for (volatile int i = 0; i < 100000; i++);

    // 2. Fast A20/Reset via System Control Port A (0x92)
    uint8_t val = inb(0x92);
    outb(0x92, val | 1);
    for (volatile int i = 0; i < 100000; i++);

    // 3. Fallback: Triple Fault
    struct { uint16_t limit; uint32_t base; } __attribute__((packed)) idt = {0, 0};
    asm volatile("lidt %0; int $3" : : "m"(idt));
}

// === Handler panic principal ===
extern "C" void panic_ex(const char *msg, uint32_t eip, uint32_t cs, uint32_t eflags) {
    (void)eip; (void)cs; (void)eflags;
    asm volatile("cli");  // dezactivează interrupțiile

    serial_print("\n!!! KERNEL PANIC !!!\n");
    if (msg) {
        serial_print(msg);
    }
    serial_print("\nEIP: ");
    char hexbuf[12];
    u32_to_hex(eip, hexbuf);
    serial_print(hexbuf);
    serial_print("\n");

    panic_render_pretty(msg);

    // Loop infinit, așteaptă tasta M sau R pentru reboot
    for (;;) {
        if (inb(0x64) & 1) {              // key ready
            uint8_t scancode = inb(0x60);
            if (scancode == 0x32 || scancode == 0x13) {      // M or R
                try_reboot();
            }
        }
    }
}

extern "C" void panic(const char *msg) {
    panic_ex(msg, 0, 0, 0);
}
