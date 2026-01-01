#include "vga.h"

#define VGA_WIDTH  320
#define VGA_HEIGHT 200
#define VGA_MEMORY ((uint8_t*)0xA0000)

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void vga_init()
{
    /* set mode 13h */
    asm volatile (
        "mov $0x13, %%ax \n"
        "int $0x10"
        :
        :
        : "ax"
    );
}

void vga_putpixel(int x, int y, uint8_t color)
{
    if (x < 0 || y < 0 || x >= VGA_WIDTH || y >= VGA_HEIGHT)
        return;

    VGA_MEMORY[y * VGA_WIDTH + x] = color;
}

void vga_clear(uint8_t color)
{
    for (int y = 0; y < VGA_HEIGHT; y++)
        for (int x = 0; x < VGA_WIDTH; x++)
            vga_putpixel(x, y, color);
}
