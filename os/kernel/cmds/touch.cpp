#include "../fs/fs.h"

extern "C" void cmd_touch(const char* name) {
    if (!name || !*name) return;
    fs_create(name, "");
}
