#include "../mm/paging.h"
#include "../terminal.h"

struct user_proc {
    void* pagedir;
    void* entry; /* virtual address */
    void* stack; /* virtual user stack top */
};

void spawn_user_process(uint8_t* elf_image, size_t len) {
    void* pd = create_user_pagetable();
    /* mapăm text la 0x08048000, stack la 0xC0000000 - 8KB (alege layout) */
    void* user_base = (void*)0x08048000;
    /* copy elf_image în pagini fizice, map la user_base cu USER bit */
    /* pentru exemplu: mapează 1 pagină și copiază un mic program binar */
}

void enter_user_mode(void* entry, void* user_stack) {
    /* setează esp0 în TSS pentru intrări kernel */
    /* pregătește frame pentru iret: */
    asm volatile (
        "movl %0, %%esp\n"   /* opțional: folosește un kernel stack pentru push-uri */
        "pushl $0x23\n"      /* user SS (DPL3 data selector) */
        "pushl %1\n"         /* user ESP */
        "pushfl\n"
        "popl %%eax\n"
        "orl $0x200, %%eax\n" /* setează IF */
        "pushl %%eax\n"
        "pushl $0x1B\n"      /* user CS (DPL3 code selector) */
        "pushl %2\n"         /* entry eip */
        "iret\n"
        : : "r"(/*dummy kernel esp*/0), "r"(user_stack), "r"(entry) : "eax"
    );
}
