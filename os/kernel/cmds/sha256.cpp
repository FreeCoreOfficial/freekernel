#include "sha256.h"
#include "fat.h"
#include "../terminal.h"
#include "../crypto/sha256.h"
#include "pathutil.h"
#include <stdint.h>

static void print_hash(uint8_t hash[32]) {
    const char* hex = "0123456789abcdef";
    for (int i = 0; i < 32; ++i) {
        terminal_putchar(hex[(hash[i] >> 4) & 0xF]);
        terminal_putchar(hex[hash[i] & 0xF]);
    }
}

static int hash_stdin(uint8_t out[32]) {
    sha256_ctx_t ctx;
    sha256_init(&ctx);
    int c;
    while ((c = terminal_read_char()) >= 0) {
        uint8_t b = (uint8_t)c;
        sha256_update(&ctx, &b, 1);
    }
    sha256_final(&ctx, out);
    return 0;
}

static int hash_file(const char* path, uint8_t out[32]) {
    char resolved[256];
    cmd_resolve_path(path, resolved, sizeof(resolved));
    fat_automount();

    int32_t size = fat32_get_file_size(resolved);
    if (size < 0) {
        terminal_printf("sha256: %s: not found\n", resolved);
        return -1;
    }

    sha256_ctx_t ctx;
    sha256_init(&ctx);

    uint8_t buf[256];
    uint32_t offset = 0;

    while (offset < (uint32_t)size) {
        uint32_t chunk = sizeof(buf);
        if (offset + chunk > (uint32_t)size) {
            chunk = (uint32_t)size - offset;
        }
        int bytes = fat32_read_file_offset(resolved, buf, chunk, offset);
        if (bytes <= 0) break;
        sha256_update(&ctx, buf, (size_t)bytes);
        offset += (uint32_t)bytes;
    }

    sha256_final(&ctx, out);
    return 0;
}

extern "C" int cmd_sha256(int argc, char** argv) {
    uint8_t hash[32];

    if (argc >= 2) {
        if (hash_file(argv[1], hash) != 0) {
            return -1;
        }
        print_hash(hash);
        terminal_writestring("  ");
        terminal_writestring(argv[1]);
        terminal_writestring("\n");
        return 0;
    }

    hash_stdin(hash);
    print_hash(hash);
    terminal_writestring("\n");
    return 0;
}
