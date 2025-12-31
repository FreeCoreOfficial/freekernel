#include "../fs/fs.h"

extern "C" void cmd_ls(const char*) {
    fs_list();
}
