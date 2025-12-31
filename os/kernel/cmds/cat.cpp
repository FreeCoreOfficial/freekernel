#include "../fs/fs.h"
#include "../terminal.h"

extern "C" void cmd_cat(const char* args) {
    const FSNode* f = fs_find(args);
    if (!f) {
        terminal_writestring("File not found\n");
        return;
    }

    terminal_writestring(f->data);
}
