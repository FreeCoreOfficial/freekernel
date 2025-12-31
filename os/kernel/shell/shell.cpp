#include "shell.h"
#include "../terminal.h"
#include "../cmds/command.h"

static char buffer[128];
static int index = 0;

static void execute_command(const char* input) {
    for (int i = 0; i < command_count; i++) {
        const char* name = command_table[i].name;

        int j = 0;
        while (name[j] && input[j] && name[j] == input[j])
            j++;

        if (name[j] == 0 && (input[j] == 0 || input[j] == ' ')) {
            const char* args = (input[j] == ' ') ? input + j + 1 : "";
            command_table[i].func(args);
            return;
        }
    }

    terminal_writestring("Unknown command\n");
}

void shell_init() {
    index = 0;
}

void shell_handle_char(char c) {
    if (c == '\n') {
        buffer[index] = 0;
        terminal_putchar('\n');

        execute_command(buffer);

        terminal_writestring("> ");
        index = 0;
        return;
    }

    if (c == '\b') {
        if (index > 0) {
            index--;
            terminal_putchar('\b');
        }
        return;
    }

    if (index < 127) {
        buffer[index++] = c;
        terminal_putchar(c);
    }
}
