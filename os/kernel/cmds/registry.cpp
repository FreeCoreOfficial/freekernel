#include "registry.h"
#include "clear.h"
#include "reboot.h"
#include "shutdown.h"
#include "ls.h"
#include "cat.h"
#include "touch.h"

Command command_table[] = {
    { "clear",    cmd_clear },
    { "reboot",   cmd_reboot },
    { "shutdown", cmd_shutdown },
    { "ls",       cmd_ls },
    { "cat",      cmd_cat },
    { "touch",    cmd_touch }
};

int command_count = sizeof(command_table) / sizeof(Command);
