#pragma once
#include "../interrupts/isr.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void mouse_init(void);
void mouse_handler(registers_t *regs);

/* Compositor Synchronization */
void mouse_blit_start(void);
void mouse_blit_end(void);
void mouse_get_coords(int *x, int *y);
const uint8_t *mouse_get_cursor_bitmap(void);

#ifdef __cplusplus
}
#endif
