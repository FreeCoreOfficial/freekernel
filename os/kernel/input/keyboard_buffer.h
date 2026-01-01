#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void kbd_buffer_init();
void kbd_push(char c);

bool kbd_has_char();
char kbd_get_char();

#ifdef __cplusplus
}
#endif
