#include "which.h"
#include "registry.h"
#include "../terminal.h"
#include "../string.h"

extern "C" int cmd_which(int argc, char** argv) {
    if (argc < 2) {
        terminal_writestring("usage: which <command>\n");
        return -1;
    }

    const char* name = argv[1];
    for (int i = 0; i < command_count; ++i) {
        if (strcmp(command_table[i].name, name) == 0) {
            terminal_writestring(name);
            terminal_writestring("\n");
            return 0;
        }
    }

    terminal_printf("which: %s: not found\n", name);
    return -1;
}
