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
#include "fat.h"

/* Folosim cast explicit la command_fn pentru a împăca
   funcțiile existente (posibil vechi) cu tipul nou. */
Command command_table[] = {
    { "beep",     (command_fn)cmd_beep },
    { "cat",      (command_fn)cmd_cat },
    { "clear",    (command_fn)cmd_clear },
    { "date",     (command_fn)cmd_date },
    { "disk",     (command_fn)cmd_disk },
    { "fat",      (command_fn)cmd_fat },
    { "help",     (command_fn)cmd_help },
    { "ls",       (command_fn)cmd_ls },
    { "play",     (command_fn)cmd_play },
    { "reboot",   (command_fn)cmd_reboot },
    { "shutdown", (command_fn)cmd_shutdown },
    { "ticks",    (command_fn)cmd_ticks },
    { "touch",    (command_fn)cmd_touch },
    { "uptime",   (command_fn)cmd_uptime },
};

int command_count = sizeof(command_table) / sizeof(Command);
