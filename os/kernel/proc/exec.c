// kernel/proc/exec.c
#include "process.h"
#include "../elf/elf.h"
#include <stdint.h>
#include <stddef.h> /* for NULL */

/* Filesystem helper (assumed) */
extern int fs_readfile(const char* path, void** out_buf, uint32_t* out_len); /* 0 ok */

/* Memory / kernel helpers */
extern void* kmalloc(uint32_t s);
extern void  kfree(void* p);
extern void  terminal_printf(const char* fmt, ...);

/* provide memcpy/memset declarations (use your kernel implementations) */
extern void* memcpy(void* dst, const void* src, uint32_t n);
extern void* memset(void* dst, int c, uint32_t n);

/* VMM / mapping helpers (assumed) */
extern void* vmm_create_page_directory(void);
extern int   vmm_map_pages(void* page_dir, uint32_t vaddr, void* phys, uint32_t size, uint32_t flags);
extern void  vmm_activate_page_directory(void* pd);
/* optional free helper (weak) */
extern void  vmm_free_page_directory(void* pd) __attribute__((weak));

/* PMM helper */
extern void* pmm_alloc_frames(uint32_t pages); /* returns physical pointer or NULL */

/* Process / scheduler helpers (assumed) */
extern int scheduler_add_process(process_t* p); /* returns pid or <0 */
extern void prepare_tss_for_process(process_t* p, void* kernel_stack, uint32_t kstack_size);

/* Constants for user stack */
#define USER_STACK_TOP 0xC0000000UL /* example: top of user space */
#define USER_STACK_SIZE (0x1000 * 8) /* 32KB stack */

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif

/* Helper: round-up to pages */
static inline uint32_t roundup_page(uint32_t x) {
    return (x + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

/* Exec function (returns pid >=0 on success, <0 on error) */
int exec_from_path(const char* path, char* const argv[]) {
    void* filebuf = NULL;
    uint32_t len = 0;
    if (!path) {
        terminal_printf("[exec] NULL path\n");
        return -1;
    }

    if (fs_readfile(path, &filebuf, &len) != 0) {
        terminal_printf("[exec] cannot read: %s\n", path);
        return -2;
    }
    if (!filebuf || len == 0) {
        terminal_printf("[exec] empty file: %s\n", path);
        if (filebuf) kfree(filebuf);
        return -3;
    }

    /* parse & create kernel copies of segments */
    elf_load_info_t info;
    int r = elf_load_from_buffer(filebuf, len, &info);
    if (r != 0) {
        terminal_printf("[exec] elf load failed: %d\n", r);
        kfree(filebuf);
        return -4;
    }

    /* allocate process struct via factory if available */
    process_t* proc = NULL;
    extern process_t* process_create_empty(void); /* if present */
    if (process_create_empty) {
        proc = process_create_empty();
    } else {
        /* fallback: plain kmalloc + memset */
        proc = (process_t*)kmalloc(sizeof(process_t));
        if (proc) memset(proc, 0, sizeof(process_t));
    }

    if (!proc) {
        terminal_printf("[exec] cannot allocate process struct\n");
        elf_unload_kernel_space(&info);
        kfree(filebuf);
        return -5;
    }

    /* minimal init */
    proc->page_dir = NULL;
    proc->entry = info.entry_point;
    proc->state = PROC_NEW;
    proc->user_stack_vaddr = USER_STACK_TOP - PAGE_SIZE; /* default, real mapping below */
    /* steal elf info (shallow) into process for later cleanup */
    proc->elf_info = info;

    /* create page directory for process */
    proc->page_dir = vmm_create_page_directory();
    if (!proc->page_dir) {
        terminal_printf("[exec] cannot create page directory\n");
        process_destroy(proc);
        kfree(filebuf);
        return -6;
    }

    /* TODO: map kernel into new PD as your OS expects.
       If you have a helper like vmm_clone_kernel_mappings(pd) call it here.
    */

    /* Map each ELF segment into process address space */
    for (int i = 0; i < proc->elf_info.seg_count; ++i) {
        elf_segment_t* s = &proc->elf_info.segments[i];
        uint32_t seg_vaddr = s->vaddr;
        uint32_t memsz = s->memsz;
        uint32_t filesz = s->filesz;

        if (memsz == 0) {
            /* nothing to map (weird) */
            continue;
        }

        uint32_t memsz_pages = roundup_page(memsz) / PAGE_SIZE;
        uint32_t map_size = memsz_pages * PAGE_SIZE;

        /* allocate physical frames */
        void* phys = pmm_alloc_frames(memsz_pages);
        if (!phys) {
            terminal_printf("[exec] pmm_alloc_frames failed for segment %d\n", i);
            process_destroy(proc);
            kfree(filebuf);
            return -7;
        }

        /* map pages into proc->page_dir at seg_vaddr
           vmm flags: adapt your flags constants; here we use placeholders
           - ensure USER bit + RW/X per segment flags
        */
        uint32_t vmm_flags = 0;
        /* Placeholder mapping flags: adapt these constants to your VMM API */
        const uint32_t VMM_FLAG_USER = 0x10;
        const uint32_t VMM_FLAG_RW   = 0x02;
        const uint32_t VMM_FLAG_RX   = 0x04; /* interpret as executable allowed */

        vmm_flags |= VMM_FLAG_USER;
        if (s->flags & PF_W) vmm_flags |= VMM_FLAG_RW;
        if (s->flags & PF_X) vmm_flags |= VMM_FLAG_RX;

        if (vmm_map_pages(proc->page_dir, seg_vaddr, phys, map_size, vmm_flags) != 0) {
            terminal_printf("[exec] vmm_map_pages failed for seg %d\n", i);
            /* NOTE: you may need to free phys frames here depending on pmm API */
            process_destroy(proc);
            kfree(filebuf);
            return -8;
        }

        /* copy file contents into physical memory (assume phys is kernel-accessible pointer)
           If your pmm returns a physical address only, you must temporarily map it into kernel
           or use an identity mapping helper. */
        if (filesz > 0 && s->kernel_buf) {
            memcpy(phys, s->kernel_buf, filesz);
        }
        if (memsz > filesz) {
            /* zero BSS tail */
            void* bss_ptr = (void*)((uintptr_t)phys + filesz);
            memset(bss_ptr, 0, memsz - filesz);
        }
    }

    /* allocate and map user stack */
    {
        uint32_t stack_pages = (USER_STACK_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;
        void* stack_phys = pmm_alloc_frames(stack_pages);
        if (!stack_phys) {
            terminal_printf("[exec] cannot allocate user stack\n");
            process_destroy(proc);
            kfree(filebuf);
            return -9;
        }
        uint32_t stack_base = (uint32_t)(USER_STACK_TOP - USER_STACK_SIZE);
        uint32_t stack_map_size = stack_pages * PAGE_SIZE;

        /* map stack with USER + RW flags */
        uint32_t stack_vmm_flags = 0;
        const uint32_t VMM_FLAG_USER = 0x10;
        const uint32_t VMM_FLAG_RW   = 0x02;
        stack_vmm_flags = VMM_FLAG_USER | VMM_FLAG_RW;

        if (vmm_map_pages(proc->page_dir, stack_base, stack_phys, stack_map_size, stack_vmm_flags) != 0) {
            terminal_printf("[exec] cannot map user stack\n");
            process_destroy(proc);
            kfree(filebuf);
            return -10;
        }
        proc->user_stack_vaddr = USER_STACK_TOP;
        /* TODO: push argv/env/aux onto user stack if desired */
    }

    /* finalize: add to scheduler */
    proc->pid = scheduler_add_process(proc);
    if (proc->pid < 0) {
        terminal_printf("[exec] scheduler failed to add process\n");
        process_destroy(proc);
        kfree(filebuf);
        return -11;
    }
    proc->state = PROC_RUNNING;

    /* cleanup: filebuf can be freed (we have kernel copies for segments) */
    kfree(filebuf);

    /* TODO: prepare TSS and kernel stack for this process so that on context switch
       the kernel can return to usermode with iret. Use prepare_tss_for_process if available. */

    terminal_printf("[exec] launched pid=%d entry=0x%x\n", proc->pid, proc->entry);
    return proc->pid;
}
