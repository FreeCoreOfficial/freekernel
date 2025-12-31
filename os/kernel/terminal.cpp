#include "terminal.h"

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
