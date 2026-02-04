#include "hexdump.h"
#include "fat.h"
#include "../terminal.h"
#include "pathutil.h"
#include <stdint.h>

static void print_hex_byte(uint8_t b) {
    const char* hex = "0123456789ABCDEF";
    terminal_putchar(hex[(b >> 4) & 0xF]);
    terminal_putchar(hex[b & 0xF]);
}

static void print_offset(uint32_t off) {
    const char* hex = "0123456789ABCDEF";
    for (int i = 7; i >= 0; --i) {
        uint8_t nib = (off >> (i * 4)) & 0xF;
        terminal_putchar(hex[nib]);
    }
}

static void dump_buffer(const uint8_t* buf, int len, uint32_t base_off) {
    for (int i = 0; i < len; i += 16) {
        int line_len = (len - i) > 16 ? 16 : (len - i);
        print_offset(base_off + (uint32_t)i);
        terminal_writestring("  ");

        for (int j = 0; j < 16; ++j) {
            if (j < line_len) {
                print_hex_byte(buf[i + j]);
            } else {
                terminal_writestring("  ");
            }
            terminal_putchar(' ');
        }

        terminal_writestring(" ");
        for (int j = 0; j < line_len; ++j) {
            char c = (char)buf[i + j];
            if (c < 32 || c > 126) c = '.';
            terminal_putchar(c);
        }
        terminal_putchar('\n');
    }
}

static void hexdump_stdin(void) {
    uint8_t buf[256];
    int len = 0;
    uint32_t off = 0;
    int c;

    while ((c = terminal_read_char()) >= 0) {
        buf[len++] = (uint8_t)c;
        if (len == (int)sizeof(buf)) {
            dump_buffer(buf, len, off);
            off += (uint32_t)len;
            len = 0;
        }
    }

    if (len > 0) {
        dump_buffer(buf, len, off);
    }
}

static void hexdump_file(const char* path) {
    char resolved[256];
    cmd_resolve_path(path, resolved, sizeof(resolved));
    fat_automount();

    int32_t size = fat32_get_file_size(resolved);
    if (size < 0) {
        terminal_printf("hexdump: %s: not found\n", resolved);
        return;
    }

    uint8_t buf[256];
    uint32_t offset = 0;

    while (offset < (uint32_t)size) {
        uint32_t chunk = sizeof(buf);
        if (offset + chunk > (uint32_t)size) {
            chunk = (uint32_t)size - offset;
        }
        int bytes = fat32_read_file_offset(resolved, buf, chunk, offset);
        if (bytes <= 0) break;
        dump_buffer(buf, bytes, offset);
        offset += (uint32_t)bytes;
    }
}

extern "C" int cmd_hexdump(int argc, char** argv) {
    if (argc >= 2) {
        hexdump_file(argv[1]);
    } else {
        hexdump_stdin();
    }
    return 0;
}
