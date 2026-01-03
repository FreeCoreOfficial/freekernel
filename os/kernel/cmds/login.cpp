/* kernel/cmds/login.cpp */
#include "../terminal.h"
#include "../user/user.h"

#ifdef __cplusplus
extern "C" {
#endif

/* usage: login <username> [password] */
int cmd_login_main(int argc, char** argv) {
    if (argc < 2) {
        terminal_printf("Usage: login <username> [password]\n");
        return -1;
    }
    const char* user = argv[1];
    const char* pass = (argc >= 3) ? argv[2] : "";
    int r = user_switch(user, pass);
    if (r == 0) return 0;
    terminal_printf("login: failed for '%s'\n", user);
    return -1;
}

#ifdef __cplusplus
}
#endif
