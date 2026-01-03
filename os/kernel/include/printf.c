#include <stdarg.h>
#include "terminal.h"

int printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    terminal_vprintf(fmt, args);  // dacă există
    va_end(args);
    return 0;
}
