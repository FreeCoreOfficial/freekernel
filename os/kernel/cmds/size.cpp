#include "size.h"
#include "fat.h"
#include "../terminal.h"
#include "pathutil.h"
#include <stdint.h>

extern "C" int cmd_size(int argc, char** argv) {
    if (argc < 2) {
        terminal_writestring("usage: size <file>\n");
        return -1;
    }

    char path[256];
    cmd_resolve_path(argv[1], path, sizeof(path));
    fat_automount();

    int32_t sz = fat32_get_file_size(path);
    if (sz < 0) {
        terminal_printf("size: %s: not found\n", path);
        return -1;
    }

    terminal_printf("%s: %d bytes\n", path, (int)sz);
    return 0;
}
