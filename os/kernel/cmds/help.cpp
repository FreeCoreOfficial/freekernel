#include "../terminal.h"
#include "registry.h"

#define COMMANDS_PER_PAGE 5

/* mic helper local */
static void u32_to_str(unsigned int val, char* buf)
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

/* parse " --page N" */
static int parse_page_arg(const char* args)
{
    if (!args || !*args)
        return 1;

    // cauta "--page"
    const char* p = args;
    while (*p) {
        if (p[0] == '-' && p[1] == '-' &&
            p[2] == 'p' && p[3] == 'a' &&
            p[4] == 'g' && p[5] == 'e')
        {
            p += 6;
            while (*p == ' ') p++;

            int val = 0;
            while (*p >= '0' && *p <= '9') {
                val = val * 10 + (*p - '0');
                p++;
            }

            if (val <= 0)
                return 1;

            return val;
        }
        p++;
    }

    return 1;
}

extern "C" void cmd_help(const char* args)
{
    int total = command_count;
    int per_page = COMMANDS_PER_PAGE;

    int max_pages = (total + per_page - 1) / per_page;

    /* --max-pages */
    if (args && args[0]) {
        if (args[0] == '-' && args[1] == '-' &&
            args[2] == 'm' && args[3] == 'a' &&
            args[4] == 'x')
        {
            char buf[16];
            terminal_writestring("max pages: ");
            u32_to_str(max_pages, buf);
            terminal_writestring(buf);
            terminal_writestring("\n");
            return;
        }
    }

    int page = parse_page_arg(args);

    if (page < 1)
        page = 1;
    if (page > max_pages)
        page = max_pages;

    int start = (page - 1) * per_page;
    int end = start + per_page;
    if (end > total)
        end = total;

    terminal_writestring("Commands (page ");
    {
        char buf[16];
        u32_to_str(page, buf);
        terminal_writestring(buf);
        terminal_writestring("/");
        u32_to_str(max_pages, buf);
        terminal_writestring(buf);
    }
    terminal_writestring("):\n");

    for (int i = start; i < end; i++) {
        terminal_writestring(" - ");
        terminal_writestring(command_table[i].name);
        terminal_writestring("\n");
    }
}
