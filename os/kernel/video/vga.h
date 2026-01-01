#pragma once
#include <stdint.h>

void vga_init();
void vga_putpixel(int x, int y, uint8_t color);
void vga_clear(uint8_t color);
