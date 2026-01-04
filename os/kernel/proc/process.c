// kernel/proc/process.c
#include "process.h"
#include <stdint.h>
#include <stddef.h> /* for NULL */

/* Kernel extern helpers (adaptează dacă ai alte nume) */
extern void* vmm_create_page_directory(void);
extern void vmm_switch_page_directory(void* pd);
extern void vmm_map_region(void* pd, uint32_t vaddr, void* phys, uint32_t size, uint32_t flags);
extern void* pmm_alloc_frames(uint32_t pages); /* returns physical pointer or NULL */

extern void* kmalloc(uint32_t s);
extern void  kfree(void* p);
extern void* memset(void* dst, int c, uint32_t n);
extern void  terminal_printf(const char* fmt, ...);

/* optional free function for PD (marked weak so linking won't fail if not present) */
extern void vmm_free_page_directory(void* pd) __attribute__((weak));

process_t* process_create_empty(void) {
    process_t* p = (process_t*)kmalloc(sizeof(process_t));
    if (!p) {
        terminal_printf("[process] kmalloc failed in process_create_empty\n");
        return NULL;
    }

    /* zero everything to avoid uninitialized fields */
    memset(p, 0, sizeof(process_t));

    p->pid = -1;
    p->page_dir = NULL;
    p->entry = 0;
    p->user_stack_vaddr = 0;
    p->state = PROC_NEW;

    /* ensure elf_info is in a clean state */
    p->elf_info.seg_count = 0;
    for (int i = 0; i < ELF_MAX_LOAD_SEGMENTS; ++i) {
        p->elf_info.segments[i].kernel_buf = NULL;
        p->elf_info.segments[i].vaddr = 0;
        p->elf_info.segments[i].filesz = 0;
        p->elf_info.segments[i].memsz = 0;
        p->elf_info.segments[i].flags = 0;
    }

    return p;
}

void process_destroy(process_t* p) {
    if (!p) return;

    /* unload ELF-backed kernel buffers if any */
    if (p->elf_info.seg_count > 0) {
        elf_unload_kernel_space(&p->elf_info);
        /* elf_unload_kernel_space should null out kernel_bufs and reset seg_count,
           but we defensively set it too. */
        p->elf_info.seg_count = 0;
    }

    /* free page directory if present and helper available */
    if (p->page_dir) {
        if (vmm_free_page_directory) {
            vmm_free_page_directory(p->page_dir);
        } else {
            /* best-effort: log and continue — kernel may not provide a free helper yet */
            terminal_printf("[process] vmm_free_page_directory not available, leaking PD handle\n");
        }
        p->page_dir = NULL;
    }

    /* TODO: unmap pages + free physical frames if your VMM requires explicit cleanup
       (if vmm_free_page_directory doesn't do that). */

    /* free process struct */
    kfree(p);
}
