#pragma once
#include <stdint.h>

void terminal_init();
void terminal_putchar(char c);
void terminal_writestring(const char* str);
void terminal_clear();
