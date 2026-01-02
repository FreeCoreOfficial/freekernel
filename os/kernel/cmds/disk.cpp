/* repaired disk.cpp - freestanding-friendly (no <string.h>/<stdio.h>/<stdlib.h>) */
#include "disk.h"
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "../storage/ata.h"
#include "../terminal.h"

/* Minimal freestanding helpers (avoid host libc headers that pull in bits/...) */
static size_t k_strlen(const char* s) {
    const char* p = s; while (*p) ++p; return (size_t)(p - s);
}
static int k_strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { ++a; ++b; }
    return (unsigned char)*a - (unsigned char)*b;
}

/* Tiny memcpy / sprintf replacements for freestanding linking */
static void* k_memcpy(void* dst, const void* src, size_t n)
{
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    for (size_t i = 0; i < n; ++i) d[i] = s[i];
    return dst;
}

static char* u32_to_dec(char* out, uint32_t v)
{
    char tmp[12]; int ti = 0;
    if (v == 0) { out[0] = '0'; return out + 1; }
    while (v > 0) { tmp[ti++] = (char)('0' + (v % 10)); v /= 10; }
    for (int i = ti - 1, j = 0; i >= 0; --i, ++j) out[j] = tmp[i];
    return out + ti;
}

static char* i32_to_dec(char* out, int32_t v)
{
    if (v < 0) { *out++ = '-'; return u32_to_dec(out, (uint32_t)(-v)); }
    return u32_to_dec(out, (uint32_t)v);
}

static int k_sprintf(char* out, const char* fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    char* p = out;
    const char* f = fmt;
    while (*f) {
        if (*f != '%') { *p++ = *f++; continue; }
        f++; // skip '%'
        switch (*f) {
            case 'u': {
                unsigned int v = va_arg(ap, unsigned int);
                p = u32_to_dec(p, (uint32_t)v);
                break;
            }
            case 'd': {
                int v = va_arg(ap, int);
                p = i32_to_dec(p, v);
                break;
            }
            case 'c': {
                int c = va_arg(ap, int);
                *p++ = (char)c;
                break;
            }
            case 's': {
                const char* s = va_arg(ap, const char*);
                if (!s) s = "(null)";
                while (*s) *p++ = *s++;
                break;
            }
            case '%': {
                *p++ = '%'; break;
            }
            default: {
                *p++ = '%'; *p++ = *f; break;
            }
        }
        f++;
    }
    *p = '\0';
    va_end(ap);
    return (int)(p - out);
}

/* map standard names used in this file to our implementations */
#define memcpy k_memcpy
#define sprintf k_sprintf
#define strlen k_strlen
#define strcmp k_strcmp

/* Small helpers ----------------------------------------------------------*/
static void println(const char* s) { terminal_writestring((char*)s); terminal_writestring("\n"); }
static void print(const char* s)  { terminal_writestring((char*)s); }

/* Lightweight decimal parser (positive integers only) */
static uint32_t parse_u32(const char* s)
{
    if (!s) return 0;
    uint32_t v = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (uint32_t)(*s - '0'); s++; }
    return v;
}

static void hexdump_line(const uint8_t* buf, int off, int len)
{
    const char hex[] = "0123456789ABCDEF";
    char out[4];
    for (int i = 0; i < len; ++i) {
        uint8_t b = buf[off + i];
        out[0] = hex[b >> 4]; out[1] = hex[b & 0xF]; out[2] = ' '; out[3] = '\0';
        print(out);
    }
    print("\n");
}

static void hexdump(const uint8_t* buf, int count)
{
    for (int i = 0; i < count; i += 16)
        hexdump_line(buf, i, (count - i) >= 16 ? 16 : (count - i));
}

/* Arg splitter (no heap) */
static int split_args(const char* args_orig, char* argv[], int max_args, char* store, int store_size)
{
    if (!args_orig || args_orig[0] == '\0') return 0;
    int n = strlen(args_orig);
    if (n + 1 > store_size) n = store_size - 1;
    memcpy(store, args_orig, n);
    store[n] = '\0';

    int argc = 0;
    char* p = store;
    while (argc < max_args && p && *p) {
        while (*p == ' ') p++;
        if (!*p) break;
        argv[argc++] = p;
        char* q = p;
        while (*q && *q != ' ') q++;
        if (*q) { *q = '\0'; p = q + 1; } else { p = NULL; }
    }
    return argc;
}

/* Partition assign table (volatile, only in-memory) */
struct part_assign { int used; char letter; uint32_t lba; uint32_t count; };
static struct part_assign g_assigns[26];

static void cmd_usage()
{
    println("disk --list              : show device info");
    println("disk --usage / --help    : this help");
    println("disk --init              : write simple MBR (one partition, LBA start 2048)");
    println("disk --assign <letter> <lba> <count> : assign letter 'a' to an LBA range");
    println("disk --format <letter>   : quick-zero first sectors of the partition");
    println("disk --test --write <lba>: write test pattern to lba");
    println("disk --test --output <lba>: read & dump LBA");
    println("disk --test --see-raw <lba> <count> : dump raw sectors");
}

static void cmd_list()
{
    uint16_t idbuf[256];
    int r = ata_identify(idbuf);
    if (r != 0) { println("[disk] no device / identify failed"); return; }

    char model[48];
    uint32_t sectors = 0;
    ata_decode_identify(idbuf, model, sizeof(model), &sectors);

    print("[disk] model: "); print(model); print("\n");
    char tmp[64];
    sprintf(tmp, "[disk] LBA28 sectors: %u\n", sectors);
    print(tmp);

    println("[disk] assignments:");
    for (int i = 0; i < 26; ++i) {
        if (g_assigns[i].used) {
            sprintf(tmp, "  %c => LBA %u count %u\n", g_assigns[i].letter, g_assigns[i].lba, g_assigns[i].count);
            print(tmp);
        }
    }
}

/* Create a minimal MBR */
static int create_minimal_mbr()
{
    uint16_t idbuf[256];
    uint32_t sectors = 0;
    if (ata_identify(idbuf) != 0) {
        println("[disk] identify failed - cannot init MBR");
        return -1;
    }
    ata_decode_identify(idbuf, NULL, 0, &sectors);
    if (sectors == 0) {
        println("[disk] identify returned 0 sectors");
        return -2;
    }

    uint32_t start = 2048;
    if (start >= sectors) start = 1;
    uint32_t part_size = sectors - start;

    uint8_t mbr[512];
    for (int i = 0; i < 512; ++i) mbr[i] = 0;

    int off = 446;
    mbr[off + 0] = 0x00;
    mbr[off + 1] = 0x00;
    mbr[off + 2] = 0x00;
    mbr[off + 3] = 0x00;
    mbr[off + 4] = 0x0B;
    mbr[off + 5] = 0x00;
    mbr[off + 6] = 0x00;
    mbr[off + 7] = 0x00;
    mbr[off + 8] = (uint8_t)(start & 0xFF);
    mbr[off + 9] = (uint8_t)((start >> 8) & 0xFF);
    mbr[off +10] = (uint8_t)((start >> 16) & 0xFF);
    mbr[off +11] = (uint8_t)((start >> 24) & 0xFF);
    mbr[off +12] = (uint8_t)(part_size & 0xFF);
    mbr[off +13] = (uint8_t)((part_size >> 8) & 0xFF);
    mbr[off +14] = (uint8_t)((part_size >> 16) & 0xFF);
    mbr[off +15] = (uint8_t)((part_size >> 24) & 0xFF);

    mbr[510] = 0x55;
    mbr[511] = 0xAA;

    ata_set_allow_mbr_write(1);
    int w = ata_write_sector(0, mbr);
    ata_set_allow_mbr_write(0);

    if (w == 0) println("[disk] MBR written (partition created)");
    else { println("[disk] MBR write FAILED"); return -3; }
    return 0;
}

static int cmd_format_letter(char letter)
{
    int idx = letter - 'a';
    if (idx < 0 || idx >= 26) { println("[disk] invalid letter"); return -1; }
    if (!g_assigns[idx].used) { println("[disk] letter not assigned"); return -2; }

    uint32_t start = g_assigns[idx].lba;
    uint32_t count = g_assigns[idx].count;

    uint32_t to_zero = count < 128 ? count : 128;
    uint8_t buf[512];
    for (int i = 0; i < 512; ++i) buf[i] = 0;

    char tmp[64];
    for (uint32_t s = 0; s < to_zero; ++s) {
        int r = ata_write_sector(start + s, buf);
        if (r != 0) {
            sprintf(tmp, "[disk] write failed at LBA %u (r=%d)\n", start + s, r);
            print(tmp);
            return -3;
        }
    }

    sprintf(tmp, "[disk] quick-format done (%u sectors zeroed)\n", to_zero);
    print(tmp);
    return 0;
}

/* cmd_disk: main entrypoint called by shell */
void cmd_disk(const char* args)
{
    char store[256];
    char* argv[16];
    int argc = split_args(args, argv, 16, store, sizeof(store));

    if (argc == 0) { cmd_usage(); return; }

    if (strcmp(argv[0], "--usage") == 0) { cmd_usage(); return; }
    if (strcmp(argv[0], "--help") == 0) { cmd_usage(); return; }
    if (strcmp(argv[0], "--list") == 0) { cmd_list(); return; }

    if (strcmp(argv[0], "--init") == 0) {
        println("[disk] initializing (write MBR) ...");
        create_minimal_mbr();
        return;
    }

    if (strcmp(argv[0], "--assign") == 0) {
        if (argc < 4) { println("usage: disk --assign <letter> <lba> <count>"); return; }
        char letter = argv[1][0];
        uint32_t lba = parse_u32(argv[2]);
        uint32_t count = parse_u32(argv[3]);
        int idx = letter - 'a';
        if (idx < 0 || idx >= 26) { println("invalid letter"); return; }
        g_assigns[idx].used = 1;
        g_assigns[idx].letter = letter;
        g_assigns[idx].lba = lba;
        g_assigns[idx].count = count;
        println("[disk] assigned");
        return;
    }

    if (strcmp(argv[0], "--format") == 0) {
        if (argc < 2) { println("usage: disk --format <letter>"); return; }
        char letter = argv[1][0];
        cmd_format_letter(letter);
        return;
    }

    if (strcmp(argv[0], "--test") == 0) {
        if (argc < 2) { println("usage: disk --test --write <lba> | --output <lba> | --see-raw <lba> <count>"); return; }
        if (strcmp(argv[1], "--write") == 0) {
            if (argc < 3) { println("usage: disk --test --write <lba>"); return; }
            uint32_t lba = parse_u32(argv[2]);
            uint8_t buf[512];
            for (int i = 0; i < 512; ++i) buf[i] = (uint8_t)(i & 0xFF);
            int r = ata_write_sector(lba, buf);
            if (r == 0) println("[disk] test write OK"); else println("[disk] test write FAILED");
            return;
        }
        if (strcmp(argv[1], "--output") == 0) {
            if (argc < 3) { println("usage: disk --test --output <lba>"); return; }
            uint32_t lba = parse_u32(argv[2]);
            uint8_t buf[512];
            int r = ata_read_sector(lba, buf);
            if (r != 0) { println("[disk] read failed"); return; }
            hexdump(buf, 512);
            return;
        }
        if (strcmp(argv[1], "--see-raw") == 0) {
            if (argc < 4) { println("usage: disk --test --see-raw <lba> <count>"); return; }
            uint32_t lba = parse_u32(argv[2]);
            int count = (int)parse_u32(argv[3]);
            uint8_t buf[512];
            for (int i = 0; i < count; ++i) {
                int r = ata_read_sector(lba + i, buf);
                if (r != 0) { print("[disk] read failed\n"); break; }
                char tmp[64]; sprintf(tmp, "--- LBA %u ---\n", lba + i); print(tmp);
                hexdump(buf, 256);
            }
            return;
        }
    }

    println("[disk] unknown command");
    cmd_usage();
}
