#include "mv.h"
#include "fat.h"
#include "../terminal.h"
#include "../mem/kmalloc.h"
#include "pathutil.h"
#include <stdint.h>

#define MAX_COPY_SIZE (2 * 1024 * 1024)

static int copy_file(const char* src, const char* dst) {
    int32_t size = fat32_get_file_size(src);
    if (size < 0) {
        terminal_printf("mv: %s: not found\n", src);
        return -1;
    }

    if (size > MAX_COPY_SIZE) {
        terminal_printf("mv: %s too large (%d bytes, max %d)\n", src, (int)size, (int)MAX_COPY_SIZE);
        return -1;
    }

    if (size == 0) {
        if (fat32_create_file(dst, 0, 0) == 0) {
            return 0;
        }
        terminal_printf("mv: failed to create %s\n", dst);
        return -1;
    }

    void* buf = kmalloc((uint32_t)size);
    if (!buf) {
        terminal_writestring("mv: out of memory\n");
        return -1;
    }

    int bytes = fat32_read_file(src, buf, (uint32_t)size);
    if (bytes < 0) {
        terminal_printf("mv: failed to read %s\n", src);
        kfree(buf);
        return -1;
    }

    int r = fat32_create_file(dst, buf, (uint32_t)bytes);
    kfree(buf);

    if (r != 0) {
        terminal_printf("mv: failed to write %s\n", dst);
        return -1;
    }

    return 0;
}

extern "C" int cmd_mv(int argc, char** argv) {
    if (argc < 3) {
        terminal_writestring("usage: mv <src> <dst>\n");
        return -1;
    }

    char src[256];
    char dst[256];
    cmd_resolve_path(argv[1], src, sizeof(src));
    cmd_resolve_path(argv[2], dst, sizeof(dst));

    fat_automount();

    int rr = fat32_rename(src, dst);
    if (rr == 0) {
        return 0;
    }
    if (rr == -2) {
        terminal_printf("mv: %s exists\n", dst);
        return -1;
    }

    if (copy_file(src, dst) != 0) {
        return -1;
    }

    if (fat32_delete_file(src) != 0) {
        terminal_printf("mv: warning: copied to %s but failed to delete %s\n", dst, src);
        return -1;
    }

    return 0;
}
