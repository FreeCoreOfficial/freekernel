#include "clear.h"
#include "../terminal.h"

extern "C" void cmd_clear(const char*) {
    terminal_clear();
}
