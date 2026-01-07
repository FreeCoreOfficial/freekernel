#include "clear.h"
#include "../terminal.h"
#include "../drivers/serial.h"

extern "C" void serial(const char *fmt, ...);

extern "C" void cmd_clear(const char*) {
    serial("[CMD] clear: starting...\n");
    terminal_clear();
    serial("[CMD] clear: done.\n");
}
