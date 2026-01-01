#include "load.h"
#include "../terminal.h"

extern "C" {

static const char* current = nullptr;

void load_begin(const char* name)
{
    current = name;
    terminal_writestring("[....] ");
    terminal_writestring(name);
}

void load_ok()
{
    terminal_writestring("\r[ OK ] ");
    terminal_writestring(current);
    terminal_writestring("\n");
}

void load_fail()
{
    terminal_writestring("\r[FAIL] ");
    terminal_writestring(current);
    terminal_writestring("\n");
}

}
