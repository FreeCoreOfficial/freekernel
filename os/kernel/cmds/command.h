#pragma once

typedef void (*command_func)(const char* args);

struct Command {
    const char* name;
    command_func func;
};

extern Command command_table[];
extern int command_count;
