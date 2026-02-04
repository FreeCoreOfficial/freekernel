#include "pwd.h"
#include "cd.h"
#include "../terminal.h"

extern "C" int cmd_pwd(int argc, char** argv) {
    (void)argc;
    (void)argv;

    char cwd[256];
    cd_get_cwd(cwd, sizeof(cwd));
    terminal_writestring(cwd);
    terminal_writestring("\n");
    return 0;
}
