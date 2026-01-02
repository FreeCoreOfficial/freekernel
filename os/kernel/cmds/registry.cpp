#include "registry.h"
#include "clear.h"
#include "reboot.h"
#include "shutdown.h"
#include "ls.h"
#include "cat.h"
#include "touch.h"
#include "date.h"
#include "beep.h"
#include "play.h"
#include "uptime.h"
#include "ticks.h"
#include "help.h"
#include "disk.h"


Command command_table[] = {
    { "beep",     cmd_beep },
    { "cat",      cmd_cat },
    { "clear",    cmd_clear },
    { "date",     cmd_date },
    { "disk",     cmd_disk },
    { "help",     cmd_help },
    { "ls",       cmd_ls },
    { "play",     cmd_play },
    { "reboot",   cmd_reboot },
    { "shutdown", cmd_shutdown },
    { "ticks",    cmd_ticks },
    { "touch",    cmd_touch },
    { "uptime",   cmd_uptime },
};

int command_count = sizeof(command_table) / sizeof(Command);
