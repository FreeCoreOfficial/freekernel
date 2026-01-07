#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the framebuffer console.
 * Requires framebuffer to be initialized first.
 */
void fb_cons_init(void);

/* Write a single character to the console */
void fb_cons_putc(char c);

/* Write a string to the console */
void fb_cons_puts(const char* s);

/* Clear the console screen */
void fb_cons_clear(void);

#ifdef __cplusplus
}
#endif