#include "mkdir.h"
#include "fat.h"
#include "../terminal.h"
#include "pathutil.h"

extern "C" int cmd_mkdir(int argc, char** argv) {
    if (argc < 2) {
        terminal_writestring("Usage: mkdir <dirname>\n");
        return -1;
    }

    char path[256];
    cmd_resolve_path(argv[1], path, sizeof(path));
    
    fat_automount();
    
    int res = fat32_create_directory(path);
    if (res == 0) {
        terminal_printf("Directory created: %s\n", path);
        return 0;
    } else {
        terminal_writestring("Error: Could not create directory.\n");
        return -1;
    }
}
