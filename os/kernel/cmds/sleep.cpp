#include "sleep.h"
#include "../terminal.h"
#include "../string.h"
#include "../time/timer.h"
#include <stdint.h>

static uint32_t parse_ms(const char* arg) {
    int len = (int)strlen(arg);
    if (len <= 0) return 0;

    if (len >= 2 && arg[len - 1] == 's' && arg[len - 2] == 'm') {
        char tmp[16];
        int copy = len - 2;
        if (copy > (int)sizeof(tmp) - 1) copy = (int)sizeof(tmp) - 1;
        memcpy(tmp, arg, copy);
        tmp[copy] = 0;
        int v = atoi(tmp);
        if (v < 0) v = 0;
        return (uint32_t)v;
    }

    if (arg[len - 1] == 's') {
        char tmp[16];
        int copy = len - 1;
        if (copy > (int)sizeof(tmp) - 1) copy = (int)sizeof(tmp) - 1;
        memcpy(tmp, arg, copy);
        tmp[copy] = 0;
        int v = atoi(tmp);
        if (v < 0) v = 0;
        return (uint32_t)v * 1000u;
    }

    int v = atoi(arg);
    if (v < 0) v = 0;
    return (uint32_t)v;
}

extern "C" int cmd_sleep(int argc, char** argv) {
    if (argc < 2) {
        terminal_writestring("usage: sleep <ms|Ns|Nms>\n");
        return -1;
    }

    uint32_t ms = parse_ms(argv[1]);
    sleep(ms);
    return 0;
}
