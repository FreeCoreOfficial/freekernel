#include "shell.h"
#include "../terminal.h"

static char buffer[128];
static int index = 0;

void shell_init() {
    index = 0;
}

void shell_handle_char(char c) {
    if (c == '\n') {
        buffer[index] = 0;

        terminal_putchar('\n');
        terminal_putchar('>');
        terminal_putchar(' ');

        index = 0;
        return;
    }

    if (c == '\b') {
        if (index > 0) index--;
        terminal_putchar('\b');
        return;
    }

    if (index < 127) {
        buffer[index++] = c;
        terminal_putchar(c);
    }
}
