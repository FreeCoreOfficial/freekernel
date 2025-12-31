#include "shell.h"
#include "../terminal.h"

static char input_buffer[128];
static int input_len = 0;

static void shell_prompt() {
    terminal_writestring("> ");
}

extern "C" void shell_init() {
    input_len = 0;
    shell_prompt();
}

extern "C" void shell_handle_char(char c) {
    if (c == '\n') {
        terminal_putchar('\n');

        // momentan doar resetăm linia
        input_len = 0;
        shell_prompt();
        return;
    }

    if (c == '\b') {
        if (input_len > 0) {
            input_len--;
            terminal_putchar('\b');
        }
        return;
    }

    if (input_len < 127) {
        input_buffer[input_len++] = c;
        terminal_putchar(c);
    }
}
