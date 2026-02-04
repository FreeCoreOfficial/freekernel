#include "../terminal.h"
#include "../string.h"
#include "fat.h"
#include "cd.h"

extern "C" void cmd_touch(const char* args) {
    if (!args || !*args) {
        terminal_writestring("usage: touch <file>\n");
        return;
    }

    /* Extract first token as path */
    char name[256];
    int i = 0;
    while (*args == ' ') args++;
    while (*args && *args != ' ' && i < (int)sizeof(name) - 1) {
        name[i++] = *args++;
    }
    name[i] = 0;

    if (!name[0]) {
        terminal_writestring("usage: touch <file>\n");
        return;
    }

    char path[256];
    if (name[0] == '/') {
        strncpy(path, name, sizeof(path));
        path[sizeof(path) - 1] = 0;
    } else {
        char cwd[256];
        cd_get_cwd(cwd, sizeof(cwd));
        strncpy(path, cwd, sizeof(path));
        path[sizeof(path) - 1] = 0;
        size_t len = strlen(path);
        if (len > 0 && path[len - 1] != '/') {
            strcat(path, "/");
        }
        strcat(path, name);
    }

    fat_automount();
    int r = fat32_create_file(path, "", 0);
    if (r != 0) {
        terminal_printf("touch: failed to create %s\n", path);
    }
}
