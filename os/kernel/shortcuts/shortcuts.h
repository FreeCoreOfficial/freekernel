#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include <stdint.h>
#include <stdbool.h>

bool shortcuts_handle_ctrl(uint8_t scancode);

/* semnal Ctrl+C */
bool shortcuts_poll_ctrl_c(void);

#endif
