#include "terminal.h"
#include <stdarg.h>
static uint16_t* vga = (uint16_t*)0xB8000;
static int row = 0;
static int col = 0;
static const uint8_t color = 0x0F;

static void scroll() {
    if (row < 25) return;

    for (int y = 1; y < 25; y++)
        for (int x = 0; x < 80; x++)
            vga[(y - 1) * 80 + x] = vga[y * 80 + x];

    for (int x = 0; x < 80; x++)
        vga[24 * 80 + x] = ' ' | (color << 8);

    row = 24;
}

extern "C" void terminal_clear() {
    for (int i = 0; i < 80 * 25; i++)
        vga[i] = ' ' | (color << 8);

    row = 0;
    col = 0;
}

extern "C" void terminal_putchar(char c) {
    if (c == '\n') {
        row++;
        col = 0;
        scroll();
        return;
    }

    if (c == '\b') {
        if (col > 0) col--;
        vga[row * 80 + col] = ' ' | (color << 8);
        return;
    }

    vga[row * 80 + col] = c | (color << 8);
    col++;

    if (col >= 80) {
        col = 0;
        row++;
        scroll();
    }
}

extern "C" void terminal_writestring(const char* s) {
    while (*s)
        terminal_putchar(*s++);
}

extern "C" void terminal_init() {
    terminal_clear();
}

/* helper: hex 32-bit */
static void print_hex(uint32_t v)
{
    const char* hex = "0123456789ABCDEF";
    terminal_writestring("0x");
    for (int i = 7; i >= 0; i--) {
        terminal_putchar(hex[(v >> (i * 4)) & 0xF]);
    }
}

/* helper: decimal */
static void print_dec(int v)
{
    char buf[16];
    int i = 0;
    int neg = 0;

    if (v == 0) {
        terminal_putchar('0');
        return;
    }

    if (v < 0) {
        neg = 1;
        v = -v;
    }

    while (v && i < 15) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }

    if (neg)
        terminal_putchar('-');

    while (i--)
        terminal_putchar(buf[i]);
}

extern "C" void terminal_vprintf(const char* fmt, void* va_ptr)
{
    va_list args;
    va_copy(args, *(va_list*)va_ptr);

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            terminal_putchar(*fmt);
            continue;
        }

        fmt++;
        switch (*fmt) {
            case 'c':
                terminal_putchar((char)va_arg(args, int));
                break;

            case 's': {
                const char* s = va_arg(args, const char*);
                terminal_writestring(s ? s : "(null)");
                break;
            }

            case 'd':
                print_dec(va_arg(args, int));
                break;

            case 'x':
                print_hex(va_arg(args, uint32_t));
                break;

            case '%':
                terminal_putchar('%');
                break;

            default:
                terminal_putchar('?');
                break;
        }
    }

    va_end(args);
}

extern "C" void terminal_printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    terminal_vprintf(fmt, &args);
    va_end(args);
}