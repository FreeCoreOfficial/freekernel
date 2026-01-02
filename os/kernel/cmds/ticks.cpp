#include "../terminal.h"
#include "../time/timer.h"

static void u32_to_str(uint32_t val, char* buf)
{
    char tmp[16];
    int i = 0;

    if (val == 0) {
        buf[0] = '0';
        buf[1] = 0;
        return;
    }

    while (val > 0) {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }

    int j = 0;
    while (i > 0)
        buf[j++] = tmp[--i];

    buf[j] = 0;
}

extern "C" void cmd_ticks(const char* args)
{
    (void)args;

    char buf[32];

    terminal_writestring("ticks: ");
    u32_to_str((uint32_t)timer_ticks(), buf);
    terminal_writestring(buf);
    terminal_writestring("\n");
}
