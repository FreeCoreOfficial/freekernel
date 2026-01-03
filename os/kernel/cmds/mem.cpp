// kernel/cmds/mem.cpp
#include "../terminal.h"
#include "../mem/kmalloc.h"
#include "../mem/slab.h"
#include "../mem/buddy.h"

#include <stdint.h>
#include <stddef.h>
#include "../string.h"


/* linker symbols from linker.ld */
extern uint8_t __heap_start;
extern uint8_t __heap_end;

/* small helper to parse an unsigned integer from args (returns 0 if fail) */
static unsigned parse_uint(const char* s) {
    if (!s) return 0;
    unsigned v = 0;
    while (*s == ' ') ++s;
    if (!*s) return 0;
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (unsigned)(*s - '0');
        ++s;
    }
    return v;
}

static void print_help() {
    terminal_writestring(
        "mem - memory tests and info\n"
        "usage:\n"
        "  mem help              : show this help\n"
        "  mem status            : show heap range\n"
        "  mem slab              : quick slab allocator smoke test\n"
        "  mem buddy             : quick buddy allocator smoke test\n"
        "  mem kmalloc <bytes>   : allocate and free <bytes> using kmalloc/kfree\n"
    );
}

extern "C" void cmd_mem(const char* args)
{
    if (!args || !*args) {
        print_help();
        return;
    }

    /* extract first token */
    char cmd[32];
    int i = 0;
    const char* p = args;
    while (*p == ' ') ++p;
    while (*p && *p != ' ' && i < (int)(sizeof(cmd)-1)) {
        cmd[i++] = *p++;
    }
    cmd[i] = '\0';
    while (*p == ' ') ++p; /* p now points to rest (maybe empty) */

    if (strcmp(cmd, "help") == 0) {
        print_help();
        return;
    }

    if (strcmp(cmd, "status") == 0) {
        uintptr_t start = (uintptr_t)&__heap_start;
        uintptr_t end   = (uintptr_t)&__heap_end;
        unsigned size = (unsigned)(end - start);
        terminal_printf("heap: %p - %p  (%u bytes)\n", (void*)start, (void*)end, size);
        return;
    }

    if (strcmp(cmd, "slab") == 0) {
        terminal_writestring("slab test: creating cache and allocating objects...\n");

        slab_cache_t* c = slab_create("test-16", 16);
        if (!c) {
            terminal_writestring("  slab_create failed\n");
            return;
        }

        void* a = slab_alloc(c);
        void* b = slab_alloc(c);
        void* cobj = slab_alloc(c);

        terminal_printf("  alloc(16) -> %p\n", a);
        terminal_printf("  alloc(16) -> %p\n", b);
        terminal_printf("  alloc(16) -> %p\n", cobj);

        slab_free(c, b);
        terminal_printf("  freed %p\n", b);

        void* d = slab_alloc(c);
        terminal_printf("  alloc(16) -> %p (should reuse freed)\n", d);

        /* free remaining */
        slab_free(c, a);
        slab_free(c, cobj);
        slab_free(c, d);

        terminal_writestring("slab test done\n");
        return;
    }

    if (strcmp(cmd, "buddy") == 0) {
        terminal_writestring("buddy test: allocating pages (order 0 = 4KB, order 1 = 8KB)...\n");
        void* p4k = buddy_alloc_page(0);
        void* p8k = buddy_alloc_page(1);

        terminal_printf("  buddy_alloc_page(order=0) -> %p\n", p4k);
        terminal_printf("  buddy_alloc_page(order=1) -> %p\n", p8k);

        if (p4k) {
            buddy_free_page(p4k, 0);
            terminal_printf("  freed %p (order 0)\n", p4k);
        }
        if (p8k) {
            buddy_free_page(p8k, 1);
            terminal_printf("  freed %p (order 1)\n", p8k);
        }

        terminal_writestring("buddy test done\n");
        return;
    }

    if (strcmp(cmd, "kmalloc") == 0) {
        unsigned n = parse_uint(p);
        if (n == 0) {
            terminal_writestring("usage: mem kmalloc <bytes>\n");
            return;
        }
        terminal_printf("kmalloc test: allocating %u bytes...\n", n);
        void* q = kmalloc((size_t)n);
        if (!q) {
            terminal_writestring("  kmalloc failed (NULL)\n");
            return;
        }
        terminal_printf("  kmalloc -> %p\n", q);

        /* touch first and last byte to ensure pages are present (optional) */
        volatile uint8_t* z = (volatile uint8_t*)q;
        z[0] = 0xAA;
        z[n-1] = 0x55;

        kfree(q);
        terminal_printf("  freed %p\n", q);
        return;
    }

    /* unknown subcommand */
    terminal_printf("mem: unknown command '%s'\n", cmd);
    terminal_writestring("Type 'mem help' for usage\n");
}
