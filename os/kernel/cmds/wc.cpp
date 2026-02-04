#include "wc.h"
#include "fat.h"
#include "../terminal.h"
#include "../string.h"
#include "pathutil.h"
#include <stdint.h>

static void wc_count_buffer(const uint8_t* buf, int len, uint32_t* lines, uint32_t* words, uint32_t* bytes, int* in_word) {
    for (int i = 0; i < len; ++i) {
        char c = (char)buf[i];
        (*bytes)++;
        if (c == '\n') (*lines)++;

        bool is_space = (c == ' ' || c == '\t' || c == '\n' || c == '\r');
        if (is_space) {
            *in_word = 0;
        } else if (!*in_word) {
            (*words)++;
            *in_word = 1;
        }
    }
}

static void wc_from_stdin(uint32_t* lines, uint32_t* words, uint32_t* bytes) {
    int in_word = 0;
    int c;
    while ((c = terminal_read_char()) >= 0) {
        uint8_t b = (uint8_t)c;
        wc_count_buffer(&b, 1, lines, words, bytes, &in_word);
    }
}

static int wc_from_file(const char* path, uint32_t* lines, uint32_t* words, uint32_t* bytes) {
    char resolved[256];
    cmd_resolve_path(path, resolved, sizeof(resolved));
    fat_automount();

    int32_t size = fat32_get_file_size(resolved);
    if (size < 0) {
        terminal_printf("wc: %s: not found\n", resolved);
        return -1;
    }

    uint32_t offset = 0;
    uint8_t buf[256];
    int in_word = 0;

    while (offset < (uint32_t)size) {
        uint32_t chunk = sizeof(buf);
        if (offset + chunk > (uint32_t)size) {
            chunk = (uint32_t)size - offset;
        }
        int bytes_read = fat32_read_file_offset(resolved, buf, chunk, offset);
        if (bytes_read <= 0) break;
        wc_count_buffer(buf, bytes_read, lines, words, bytes, &in_word);
        offset += (uint32_t)bytes_read;
    }

    return 0;
}

extern "C" int cmd_wc(int argc, char** argv) {
    uint32_t lines = 0;
    uint32_t words = 0;
    uint32_t bytes = 0;

    if (argc >= 2) {
        if (wc_from_file(argv[1], &lines, &words, &bytes) != 0) {
            return -1;
        }
        terminal_printf("%u %u %u %s\n", lines, words, bytes, argv[1]);
        return 0;
    }

    wc_from_stdin(&lines, &words, &bytes);
    terminal_printf("%u %u %u\n", lines, words, bytes);
    return 0;
}
