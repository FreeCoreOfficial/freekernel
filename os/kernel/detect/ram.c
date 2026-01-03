// kernel/detect/ram.c
#include "ram.h"

#include "../terminal.h"
#include "../panic.h"

#define MULTIBOOT_MAGIC 0x2BADB002u

/* minim multiboot basic for mem fields */
struct mb_basic {
    uint32_t flags;
    uint32_t mem_lower; /* KB */
    uint32_t mem_upper; /* KB */
};

uint32_t ram_detect_mb(uint32_t multiboot_magic, uint32_t multiboot_addr)
{
    if (multiboot_magic != MULTIBOOT_MAGIC)
        return 0;

    if (multiboot_addr == 0)
        return 0;

    struct mb_basic *mb = (struct mb_basic*)(uintptr_t)multiboot_addr;
    if (!(mb->flags & 0x1))
        return 0;

    uint32_t total_kb = mb->mem_lower + mb->mem_upper;
    return total_kb / 1024;
}

void ram_check_or_panic(uint32_t multiboot_magic, uint32_t multiboot_addr)
{
    uint32_t ram_mb = ram_detect_mb(multiboot_magic, multiboot_addr);

    /* afișăm detaliile (terminal + serial deja sunt folosite în altă parte) */
    terminal_printf("[ram] detected %u MB\n", ram_mb);

    if (ram_mb < CHRYSALIS_MIN_RAM_MB) {
        /* Panic acceptă un singur string; folosim terminal_printf pentru detalii */
        panic("INSUFFICIENT RAM: require >= 20 MB");
    }
}
