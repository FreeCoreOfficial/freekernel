#pragma once
#include <stdint.h>
#include "../elf/elf.h"

typedef enum {
    PROC_NEW = 0,
    PROC_RUNNING,
    PROC_WAITING,
    PROC_ZOMBIE
} proc_state_t;

typedef struct process {
    int pid;
    void* page_dir; /* pointer/handle la page directory (implementation-specific) */
    uint32_t entry;
    uint32_t user_stack_vaddr;
    elf_load_info_t elf_info; /* keep the loaded info for cleanup / debugging */
    proc_state_t state;
    /* TODO: add file descriptors, cwd, name, kernel stack, etc. */
} process_t;

process_t* process_create_empty(void);
void process_destroy(process_t* p);
