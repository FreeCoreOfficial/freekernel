#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void shortcuts_handle_scancode(uint8_t scancode);
int shortcut_ctrl_c();

#ifdef __cplusplus
}
#endif
