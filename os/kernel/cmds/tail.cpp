#include "tail.h"
#include "fat.h"
#include "../terminal.h"
#include "../string.h"
#include "../mem/kmalloc.h"
#include "pathutil.h"
#include <stdint.h>

#define MAX_TAIL_BYTES (16 * 1024)

static int parse_lines_arg(int argc, char** argv, int* out_lines, const char** out_path) {
    int lines = 10;
    const char* path = 0;

    if (argc >= 3 && strcmp(argv[1], "-n") == 0) {
        lines = atoi(argv[2]);
        if (argc >= 4) path = argv[3];
    } else if (argc >= 2) {
        path = argv[1];
    }

    if (lines <= 0) lines = 10;

    *out_lines = lines;
    *out_path = path;
    return 0;
}

static void print_tail_buffer(const char* buf, int len, int lines) {
    if (len <= 0) return;

    int found = 0;
    int start = 0;

    for (int i = len - 1; i >= 0; --i) {
        if (buf[i] == '\n') {
            found++;
            if (found > lines) {
                start = i + 1;
                break;
            }
        }
    }

    for (int i = start; i < len; ++i) {
        terminal_putchar(buf[i]);
    }

    if (len > 0 && buf[len - 1] != '\n') {
        terminal_putchar('\n');
    }
}

static void tail_from_stdin(int lines) {
    char* buf = (char*)kmalloc(MAX_TAIL_BYTES);
    if (!buf) {
        terminal_writestring("tail: out of memory\n");
        return;
    }

    int len = 0;
    int c;
    while ((c = terminal_read_char()) >= 0) {
        if (len < MAX_TAIL_BYTES) {
            buf[len++] = (char)c;
        } else {
            memmove(buf, buf + 1, MAX_TAIL_BYTES - 1);
            buf[MAX_TAIL_BYTES - 1] = (char)c;
        }
    }

    print_tail_buffer(buf, len, lines);
    kfree(buf);
}

static void tail_from_file(const char* path, int lines) {
    char resolved[256];
    cmd_resolve_path(path, resolved, sizeof(resolved));
    fat_automount();

    int32_t size = fat32_get_file_size(resolved);
    if (size < 0) {
        terminal_printf("tail: %s: not found\n", resolved);
        return;
    }

    uint32_t read_size = (uint32_t)size;
    if (read_size > MAX_TAIL_BYTES) {
        read_size = MAX_TAIL_BYTES;
        terminal_printf("tail: showing last %d bytes (file is %d bytes)\n", (int)read_size, (int)size);
    }

    uint32_t offset = (uint32_t)size - read_size;
    char* buf = (char*)kmalloc(read_size);
    if (!buf) {
        terminal_writestring("tail: out of memory\n");
        return;
    }

    int bytes = fat32_read_file_offset(resolved, buf, read_size, offset);
    if (bytes <= 0) {
        terminal_printf("tail: failed to read %s\n", resolved);
        kfree(buf);
        return;
    }

    print_tail_buffer(buf, bytes, lines);
    kfree(buf);
}

extern "C" int cmd_tail(int argc, char** argv) {
    int lines = 10;
    const char* path = 0;
    if (argc > 1) {
        parse_lines_arg(argc, argv, &lines, &path);
    }

    if (path) {
        tail_from_file(path, lines);
    } else {
        tail_from_stdin(lines);
    }

    return 0;
}
