#include "command.h"
#include "../terminal.h"

void cmd_clear(const char*) {
    terminal_clear();
}
