#include "grep.h"
#include "fat.h"
#include "../terminal.h"
#include "../string.h"
#include "pathutil.h"
#include <stdint.h>

static int parse_args(int argc, char** argv, const char** pattern, const char** path, int* show_line) {
    *show_line = 0;
    *pattern = 0;
    *path = 0;

    int idx = 1;
    if (argc > 1 && strcmp(argv[1], "-n") == 0) {
        *show_line = 1;
        idx++;
    }

    if (idx >= argc) return -1;
    *pattern = argv[idx++];
    if (idx < argc) *path = argv[idx];
    return 0;
}

static void grep_line(const char* line, int len, const char* pattern, int show_line, int line_no) {
    char tmp[512];
    int copy_len = len;
    if (copy_len > (int)(sizeof(tmp) - 1)) copy_len = (int)(sizeof(tmp) - 1);
    memcpy(tmp, line, copy_len);
    tmp[copy_len] = 0;

    if (strstr(tmp, pattern)) {
        if (show_line) {
            terminal_printf("%d:", line_no);
        }
        terminal_writestring(tmp);
        if (copy_len == 0 || tmp[copy_len - 1] != '\n') terminal_putchar('\n');
    }
}

static void grep_from_stdin(const char* pattern, int show_line) {
    char line[512];
    int len = 0;
    int line_no = 1;
    int c;

    while ((c = terminal_read_char()) >= 0) {
        if (len < (int)(sizeof(line) - 1)) {
            line[len++] = (char)c;
        }
        if (c == '\n') {
            grep_line(line, len, pattern, show_line, line_no);
            len = 0;
            line_no++;
        }
    }

    if (len > 0) {
        grep_line(line, len, pattern, show_line, line_no);
    }
}

static void grep_from_file(const char* path, const char* pattern, int show_line) {
    char resolved[256];
    cmd_resolve_path(path, resolved, sizeof(resolved));
    fat_automount();

    int32_t size = fat32_get_file_size(resolved);
    if (size < 0) {
        terminal_printf("grep: %s: not found\n", resolved);
        return;
    }

    char line[512];
    int len = 0;
    int line_no = 1;
    uint8_t buf[256];
    uint32_t offset = 0;

    while (offset < (uint32_t)size) {
        uint32_t chunk = sizeof(buf);
        if (offset + chunk > (uint32_t)size) {
            chunk = (uint32_t)size - offset;
        }
        int bytes = fat32_read_file_offset(resolved, buf, chunk, offset);
        if (bytes <= 0) break;

        for (int i = 0; i < bytes; ++i) {
            char c = (char)buf[i];
            if (len < (int)(sizeof(line) - 1)) {
                line[len++] = c;
            }
            if (c == '\n') {
                grep_line(line, len, pattern, show_line, line_no);
                len = 0;
                line_no++;
            }
        }

        offset += (uint32_t)bytes;
    }

    if (len > 0) {
        grep_line(line, len, pattern, show_line, line_no);
    }
}

extern "C" int cmd_grep(int argc, char** argv) {
    const char* pattern = 0;
    const char* path = 0;
    int show_line = 0;

    if (parse_args(argc, argv, &pattern, &path, &show_line) != 0 || !pattern) {
        terminal_writestring("usage: grep [-n] <pattern> [file]\n");
        return -1;
    }

    if (path) {
        grep_from_file(path, pattern, show_line);
    } else {
        grep_from_stdin(pattern, show_line);
    }

    return 0;
}
