#include "head.h"
#include "fat.h"
#include "../terminal.h"
#include "../string.h"
#include "pathutil.h"
#include <stdint.h>

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

static void head_from_stdin(int lines) {
    int c;
    int seen = 0;
    while ((c = terminal_read_char()) >= 0) {
        terminal_putchar((char)c);
        if (c == '\n') {
            seen++;
            if (seen >= lines) break;
        }
    }
}

static void head_from_file(const char* path, int lines) {
    char resolved[256];
    cmd_resolve_path(path, resolved, sizeof(resolved));
    fat_automount();

    int32_t size = fat32_get_file_size(resolved);
    if (size < 0) {
        terminal_printf("head: %s: not found\n", resolved);
        return;
    }

    uint32_t offset = 0;
    uint8_t buf[256];
    int seen = 0;

    while (offset < (uint32_t)size) {
        uint32_t chunk = sizeof(buf);
        if (offset + chunk > (uint32_t)size) {
            chunk = (uint32_t)size - offset;
        }
        int bytes = fat32_read_file_offset(resolved, buf, chunk, offset);
        if (bytes <= 0) break;

        for (int i = 0; i < bytes; ++i) {
            terminal_putchar((char)buf[i]);
            if (buf[i] == '\n') {
                seen++;
                if (seen >= lines) return;
            }
        }
        offset += (uint32_t)bytes;
    }
}

extern "C" int cmd_head(int argc, char** argv) {
    int lines = 10;
    const char* path = 0;
    if (argc > 1) {
        parse_lines_arg(argc, argv, &lines, &path);
    }

    if (path) {
        head_from_file(path, lines);
    } else {
        head_from_stdin(lines);
    }

    return 0;
}
