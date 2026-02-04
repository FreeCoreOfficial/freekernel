#include "tee.h"
#include "fat.h"
#include "../terminal.h"
#include "../mem/kmalloc.h"
#include "pathutil.h"
#include <stdint.h>

#define MAX_TEE_BYTES (64 * 1024)

extern "C" int cmd_tee(int argc, char** argv) {
    if (argc < 2) {
        terminal_writestring("usage: tee <file>\n");
        return -1;
    }

    char path[256];
    cmd_resolve_path(argv[1], path, sizeof(path));
    fat_automount();

    uint8_t* buf = (uint8_t*)kmalloc(MAX_TEE_BYTES);
    if (!buf) {
        terminal_writestring("tee: out of memory\n");
        return -1;
    }

    uint32_t len = 0;
    int c;
    while ((c = terminal_read_char()) >= 0) {
        terminal_putchar((char)c);
        if (len < MAX_TEE_BYTES) {
            buf[len++] = (uint8_t)c;
        }
    }

    int r = fat32_create_file(path, buf, len);
    kfree(buf);

    if (r != 0) {
        terminal_printf("tee: failed to write %s\n", path);
        return -1;
    }

    return 0;
}
