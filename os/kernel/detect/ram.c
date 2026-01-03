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

/* Convert uint32 to decimal string. Returns number of chars written.
   Writes no leading zeros. Assumes out has enough space. */
static int u32_to_dec(uint32_t value, char *out)
{
    char tmp[12]; /* enough for 32-bit decimal */
    int pos = 0;
    if (value == 0) {
        out[0] = '0';
        return 1;
    }
    while (value > 0) {
        tmp[pos++] = '0' + (value % 10);
        value /= 10;
    }
    /* reverse into out */
    int i;
    for (i = 0; i < pos; ++i) {
        out[i] = tmp[pos - 1 - i];
    }
    return pos;
}

/* append src into dst at dst[off], return appended length */
static int append_str_at(char *dst, int off, const char *src)
{
    int n = 0;
    while (src[n]) {
        dst[off + n] = src[n];
        ++n;
    }
    return n;
}

/* append single char */
static int append_char_at(char *dst, int off, char c)
{
    dst[off] = c;
    return 1;
}

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
    uint32_t mem_lower_kb = 0;
    uint32_t mem_upper_kb = 0;
    uint32_t total_kb = 0;
    uint32_t ram_mb = 0;

    if (multiboot_magic == MULTIBOOT_MAGIC && multiboot_addr != 0) {
        struct mb_basic *mb = (struct mb_basic*)(uintptr_t)multiboot_addr;
        if (mb && (mb->flags & 0x1)) {
            mem_lower_kb = mb->mem_lower;
            mem_upper_kb = mb->mem_upper;
            total_kb = mem_lower_kb + mem_upper_kb;
            ram_mb = total_kb / 1024;
        }
    }

    /* Detailed terminal output */
    terminal_printf("[ram] mem_lower = %u KB\n", mem_lower_kb);
    terminal_printf("[ram] mem_upper = %u KB\n", mem_upper_kb);
    terminal_printf("[ram] total     = %u KB (%u MB)\n", total_kb, ram_mb);
    terminal_printf("[ram] required  = %u MB\n", CHRYSALIS_MIN_RAM_MB);

    if (ram_mb < CHRYSALIS_MIN_RAM_MB) {
        /* Build a multi-line panic message in a single buffer */
        char buf[384];
        int off = 0;

        off += append_str_at(buf, off, "INSUFFICIENT RAM\n");
        off += append_str_at(buf, off, "-----------------\n");

        off += append_str_at(buf, off, "Detected: ");
        off += u32_to_dec(ram_mb, buf + off);
        off += append_str_at(buf, off, " MB (");
        off += u32_to_dec(total_kb, buf + off);
        off += append_str_at(buf, off, " KB)\n");

        off += append_str_at(buf, off, "Lower mem: ");
        off += u32_to_dec(mem_lower_kb, buf + off);
        off += append_str_at(buf, off, " KB\n");

        off += append_str_at(buf, off, "Upper mem: ");
        off += u32_to_dec(mem_upper_kb, buf + off);
        off += append_str_at(buf, off, " KB\n");

        off += append_str_at(buf, off, "Required : ");
        off += u32_to_dec(CHRYSALIS_MIN_RAM_MB, buf + off);
        off += append_str_at(buf, off, " MB\n\n");

        off += append_str_at(buf, off, "Suggestion: boot with more RAM (e.g. qemu -m 64)\n");
        off += append_str_at(buf, off, "If you believe this is incorrect, check your bootloader Multiboot info.\n");

        /* terminate */
        if (off < (int)sizeof(buf)) buf[off] = '\0';
        else buf[sizeof(buf)-1] = '\0';

        /* print the same info to terminal (redundant but useful) */
        terminal_writestring("\n");
        terminal_writestring(buf);
        terminal_writestring("\n");

        /* call panic with multi-line message (single const char*) */
        panic(buf);
    }
}
