#include "rm.h"
#include "pathutil.h"
#include "fat.h"
#include "../terminal.h"

extern "C" int cmd_rm(int argc, char** argv) {
    if (argc < 2) {
        terminal_writestring("Usage: rm <filename>\n");
        return -1;
    }

    char path[256];
    cmd_resolve_path(argv[1], path, sizeof(path));
    
    fat_automount();
    
    int res = fat32_delete_file(path);
    if (res == 0) {
        terminal_writestring("File deleted.\n");
        return 0;
    } else {
        terminal_writestring("Error: Could not delete file (not found or error).\n");
        return -1;
    }
}
