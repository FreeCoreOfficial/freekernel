// kernel/cmds/mem.cpp
#include "../terminal.h"
#include "../mem/kmalloc.h"
#include "../mem/slab.h"
#include "../mem/buddy.h"

#include <stdint.h>
#include <stddef.h>

/* Minimal local string helpers (avoid system libc includes) */
static size_t k_strlen(const char* s) {
    const char* p = s;
    while (p && *p) ++p;
    return (size_t)(p - s);
}

static int k_strcmp(const char* a, const char* b) {
    if (!a || !b) {
        if (a == b) return 0;
        return a ? 1 : -1;
    }
    while (*a && (*a == *b)) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static const char* k_strstr(const char* haystack, const char* needle) {
    if (!haystack || !needle) return NULL;
    if (!*needle) return haystack;
    for (; *haystack; ++haystack) {
        const char* h = haystack;
        const char* n = needle;
        while (*n && *h == *n) { ++h; ++n; }
        if (!*n) return haystack;
    }
    return NULL;
}

static void* k_memcpy(void* dest, const void* src, size_t n) {
    if (!dest || !src) return dest;
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    for (size_t i = 0; i < n; ++i) d[i] = s[i];
    return dest;
}

/* map names used below */
#define strlen  k_strlen
#define strcmp  k_strcmp
#define strstr  k_strstr
#define memcpy  k_memcpy

/* linker symbols (heap bounds) */
extern uint8_t __heap_start;
extern uint8_t __heap_end;

/* helpers */
static const char* skip_spaces(const char* s) {
    if (!s) return s;
    while (*s == ' ' || *s == '\t') ++s;
    return s;
}

static unsigned parse_uint(const char* s) {
    if (!s) return 0;
    s = skip_spaces(s);
    if (!*s) return 0;
    unsigned v = 0;
    int got = 0;
    while (*s >= '0' && *s <= '9') {
        got = 1;
        v = v * 10 + (unsigned)(*s - '0');
        ++s;
    }
    return got ? v : 0;
}

/* help text */
static void print_help(void) {
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

/* --- subcommand implementations --- */
static void cmd_mem_status(void) {
    uintptr_t start = (uintptr_t)&__heap_start;
    uintptr_t end   = (uintptr_t)&__heap_end;
    unsigned size = (unsigned)(end - start);
    terminal_printf("heap: %p - %p  (%u bytes)\n", (void*)start, (void*)end, size);
}

static void cmd_mem_slab(const char* /*rest*/) {
    terminal_writestring("slab test: creating cache and allocating objects...\n");
    slab_cache_t* cache = slab_create("test-16", 16);
    if (!cache) {
        terminal_writestring("  slab_create failed (out of memory?)\n");
        return;
    }
    void* a = slab_alloc(cache);
    void* b = slab_alloc(cache);
    void* cobj = slab_alloc(cache);
    terminal_printf("  alloc(16) -> %p\n", a ? a : (void*)0);
    terminal_printf("  alloc(16) -> %p\n", b ? b : (void*)0);
    terminal_printf("  alloc(16) -> %p\n", cobj ? cobj : (void*)0);
    if (b) { slab_free(cache, b); terminal_printf("  freed %p\n", b); }
    else terminal_writestring("  note: second alloc returned NULL, skipping free\n");
    void* d = slab_alloc(cache);
    terminal_printf("  alloc(16) -> %p (should reuse freed)\n", d ? d : (void*)0);
    if (a) slab_free(cache, a);
    if (cobj) slab_free(cache, cobj);
    if (d) slab_free(cache, d);
    terminal_writestring("slab test done\n");
}

static void cmd_mem_buddy(const char* /*rest*/) {
    terminal_writestring("buddy test: allocating pages (order 0 = 4KB, order 1 = 8KB)...\n");
    void* p4k = buddy_alloc_page(0);
    void* p8k = buddy_alloc_page(1);
    terminal_printf("  buddy_alloc_page(order=0) -> %p\n", p4k ? p4k : (void*)0);
    terminal_printf("  buddy_alloc_page(order=1) -> %p\n", p8k ? p8k : (void*)0);
    if (p4k) { buddy_free_page(p4k, 0); terminal_printf("  freed %p (order 0)\n", p4k); }
    else terminal_writestring("  note: buddy_alloc_page(order=0) returned NULL\n");
    if (p8k) { buddy_free_page(p8k, 1); terminal_printf("  freed %p (order 1)\n", p8k); }
    else terminal_writestring("  note: buddy_alloc_page(order=1) returned NULL\n");
    terminal_writestring("buddy test done\n");
}

static void cmd_mem_kmalloc(const char* rest) {
    unsigned n = parse_uint(rest);
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
    if (n >= 1) {
        volatile uint8_t* z = (volatile uint8_t*)q;
        z[0] = 0xAA;
        z[n-1] = 0x55;
    }
    kfree(q);
    terminal_printf("  freed %p\n", q);
}

/* Build a single args string from argv[1..argc-1] into dest (dest_size).
   Returns pointer to dest (empty string if no args). */
static char* build_args_from_argv(int argc, char* argv[], char* dest, size_t dest_size) {
    if (!dest || dest_size == 0) return dest;
    dest[0] = '\0';
    if (argc <= 1 || !argv) return dest;
    size_t pos = 0;
    for (int i = 1; i < argc; ++i) {
        const char* tok = argv[i] ? argv[i] : "";
        for (size_t j = 0; tok[j] && pos + 1 < dest_size; ++j) {
            dest[pos++] = tok[j];
        }
        if (i + 1 < argc && pos + 1 < dest_size) {
            dest[pos++] = ' ';
        }
    }
    dest[pos < dest_size ? pos : (dest_size-1)] = '\0';
    return dest;
}

/* Main entry expected by shell: signature (int argc, char* argv[]) */
extern "C" void cmd_mem(int argc, char* argv[])
{
    /* debug: show what shell passed in */
    terminal_printf("[mem dbg] argc=%d argv0='%s' argv1='%s'\n",
        argc,
        (argc >= 1 && argv[0]) ? argv[0] : "(null)",
        (argc >= 2 && argv[1]) ? argv[1] : "(null)"
    );

    /* Build args string */
    char argsbuf[128];
    char* args = build_args_from_argv(argc, argv, argsbuf, sizeof(argsbuf));

    /* If empty -> help */
    if (!args || !*args) {
        print_help();
        return;
    }

    /* Try tokenizing the args string: first token = subcommand */
    const char* p = args;
    char token[32];
    /* skip leading spaces */
    while (*p == ' ' || *p == '\t') ++p;
    int ti = 0;
    while (*p && *p != ' ' && *p != '\t' && ti + 1 < (int)sizeof(token)) {
        token[ti++] = *p++;
    }
    token[ti] = '\0';
    while (*p == ' ' || *p == '\t') ++p; /* rest */

    /* dispatch */
    if (strcmp(token, "help") == 0) { print_help(); return; }
    if (strcmp(token, "status") == 0) { cmd_mem_status(); return; }
    if (strcmp(token, "slab") == 0) { cmd_mem_slab(p); return; }
    if (strcmp(token, "buddy") == 0) { cmd_mem_buddy(p); return; }
    if (strcmp(token, "kmalloc") == 0) { cmd_mem_kmalloc(p); return; }

    /* fallback scanning the whole args string */
    if (strstr(args, "help") != NULL) { print_help(); return; }
    if (strstr(args, "status") != NULL) { cmd_mem_status(); return; }
    if (strstr(args, "slab") != NULL) { cmd_mem_slab(args); return; }
    if (strstr(args, "buddy") != NULL) { cmd_mem_buddy(args); return; }
    const char* kmpos = strstr(args, "kmalloc");
    if (kmpos) { kmpos += 7; cmd_mem_kmalloc(kmpos); return; }

    terminal_printf("mem: unknown command '%s'\n", token);
    terminal_writestring("Type 'mem help' for usage\n");
}
