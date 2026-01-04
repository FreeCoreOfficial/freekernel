/* kernel/stubs/exec_fallbacks.c
 * Fallback weak stubs to satisfy linker while real subsystems are not yet wired.
 * These print a message and return error/NULL so exec() fails cleanly.
 */

#include <stdint.h>
#include <stddef.h>

/* Terminal logging - replace with your kernel's print if named differently */
extern void terminal_printf(const char* fmt, ...);

/* Filesystem stub */
int fs_readfile(const char* path, void** out_buf, uint32_t* out_len) __attribute__((weak));
int fs_readfile(const char* path, void** out_buf, uint32_t* out_len) {
    (void)path;
    if (out_buf) *out_buf = NULL;
    if (out_len) *out_len = 0;
    terminal_printf("[stub fs] fs_readfile('%s') not implemented\n", path ? path : "(null)");
    return -1; /* failure */
}

/* VMM / page directory stub */
void* vmm_create_page_directory(void) __attribute__((weak));
void* vmm_create_page_directory(void) {
    terminal_printf("[stub vmm] vmm_create_page_directory() not implemented\n");
    return NULL;
}

int vmm_map_pages(void* page_dir, uint32_t vaddr, void* phys, uint32_t size, uint32_t flags) __attribute__((weak));
int vmm_map_pages(void* page_dir, uint32_t vaddr, void* phys, uint32_t size, uint32_t flags) {
    (void)page_dir; (void)vaddr; (void)phys; (void)size; (void)flags;
    terminal_printf("[stub vmm] vmm_map_pages(...) not implemented\n");
    return -1;
}

/* PMM stub - allocation fails */
void* pmm_alloc_frames(uint32_t pages) __attribute__((weak));
void* pmm_alloc_frames(uint32_t pages) {
    (void)pages;
    terminal_printf("[stub pmm] pmm_alloc_frames(%u) not implemented\n", pages);
    return NULL;
}

/* Scheduler stub */
int scheduler_add_process(void* p) __attribute__((weak));
int scheduler_add_process(void* p) {
    (void)p;
    terminal_printf("[stub sched] scheduler_add_process() not implemented\n");
    return -1;
}
