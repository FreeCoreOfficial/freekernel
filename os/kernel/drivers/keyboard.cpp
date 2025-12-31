#include "keyboard.h"

static const char keymap[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',
};

extern "C" void terminal_putchar(char c);

void keyboard_handle(uint8_t scancode) {
    if (scancode & 0x80)
        return;

    char c = keymap[scancode];
    if (c)
        terminal_putchar(c);
}
