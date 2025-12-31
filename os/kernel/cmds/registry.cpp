#include "command.h"

void cmd_clear(const char*);
void cmd_reboot(const char*);
void cmd_shutdown(const char*);

Command command_table[] = {
    { "clear",    cmd_clear },
    { "reboot",   cmd_reboot },
    { "shutdown", cmd_shutdown },
};

int command_count = sizeof(command_table) / sizeof(Command);
