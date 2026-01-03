#pragma once
#include <stdint.h>

#define KSTACK_SIZE 8192

typedef struct task {
    int pid;
    uint32_t *kstack_ptr;   /* pointer la frame-ul salvat (folosit de context_switch) */
    uint32_t cr3;           /* optional: pagina director (setează CR3 la switch) */
    uint8_t kstack[KSTACK_SIZE] __attribute__((aligned(16)));
    struct task *next;      /* pentru scheduler circular simplu */
} task_t;

/* API minim */
task_t *task_create(void (*entry)(void), int pid);
void task_init_scheduler(void);
void yield(void);         /* forțează context switch */
void schedule(void);
extern task_t *current_task;
