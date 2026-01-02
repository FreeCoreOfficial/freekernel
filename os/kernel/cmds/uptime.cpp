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

extern "C" void cmd_uptime(const char* args)
{
    (void)args;

    uint32_t sec = timer_uptime_seconds();
    uint32_t ms  = timer_uptime_ms() % 1000;

    char buf[32];

    terminal_writestring("uptime: ");

    u32_to_str(sec, buf);
    terminal_writestring(buf);

    terminal_writestring(".");

    /* zero padding ms */
    if (ms < 100) terminal_writestring("0");
    if (ms < 10)  terminal_writestring("0");

    u32_to_str(ms, buf);
    terminal_writestring(buf);

    terminal_writestring(" sec\n");
}
