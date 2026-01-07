// kernel/terminal/terminal.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void terminal_putchar(char c);
void terminal_writestring(const char* s);
void terminal_clear();
void terminal_init();

/* Switch backend to Framebuffer Console */
void terminal_set_backend_fb(bool active);

/* ===== NEW: printf support ===== */
void terminal_printf(const char* fmt, ...);
void terminal_vprintf(const char* fmt, void* va); /* intern */

/* print value as hex (no 0x prefix), trimmed leading zeros */
void terminal_writehex(uint32_t v);

#ifdef __cplusplus
}
#endif
