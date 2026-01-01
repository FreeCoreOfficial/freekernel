#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct keymap {
    const char* name;

    uint16_t normal[128];
    uint16_t shift[128];
    uint16_t altgr[128];
} keymap_t;

extern const keymap_t* current_keymap;

void keymap_set(const keymap_t* map);
uint16_t keymap_translate(uint8_t scancode, int shift, int altgr);

#ifdef __cplusplus
}
#endif
