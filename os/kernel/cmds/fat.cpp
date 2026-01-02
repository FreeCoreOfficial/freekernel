// kernel/cmds/fat.cpp
#include "fat.h"

#include "../fs/fat/fat.h"
#include "../terminal.h"
#include <stdint.h>

/*
 * Comandă: fat
 *
 * Utilizare:
 *   fat mount <lba>
 *   fat info
 */

/* mic strcmp local, freestanding */
static int strcmp_local(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int cmd_fat(int argc, char **argv)
{
    if (argc < 2) {
        terminal_writestring("Usage:\n");
        terminal_writestring("  fat mount <lba>\n");
        terminal_writestring("  fat info\n");
        return 0;
    }

    if (!strcmp_local(argv[1], "mount")) {
        if (argc < 3) {
            terminal_writestring("fat mount: lipseste LBA\n");
            return 0;
        }

        /* conversie string -> uint32_t (doar cifre) */
        uint32_t lba = 0;
        const char *p = argv[2];
        if (*p == '\0') {
            terminal_writestring("fat mount: LBA invalid\n");
            return 0;
        }
        while (*p) {
            char c = *p++;
            if (c < '0' || c > '9') {
                terminal_writestring("fat mount: LBA invalid\n");
                return 0;
            }
            lba = lba * 10 + (uint32_t)(c - '0');
        }

        if (!fat_mount(lba)) {
            terminal_writestring("FAT mount failed\n");
        }
        return 0;
    }

    if (!strcmp_local(argv[1], "info")) {
        fat_info();
        return 0;
    }

    terminal_writestring("Comanda necunoscuta: ");
    terminal_writestring(argv[1]);
    terminal_writestring("\n");

    return 0;
}
