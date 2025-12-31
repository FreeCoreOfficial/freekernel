#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void terminal_putchar(char c);
void terminal_writestring(const char* s);
void terminal_clear();
void terminal_init();

#ifdef __cplusplus
}
#endif
