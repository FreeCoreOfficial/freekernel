#include "../fs/fs.h"
#include "../terminal.h"
#include "../fs/chrysfs/chrysfs.h"
#include "../string.h"
#include "../mem/kmalloc.h"
#include "fat.h"
#include "pathutil.h"

/* FAT32 Driver API */
extern "C" int fat32_read_file(const char* path, void* buf, uint32_t max_size);

extern "C" void cmd_cat(int argc, char** argv) {
    if (argc < 2) {
        terminal_writestring("usage: cat <file>\n");
        return;
    }
    char path[256];
    cmd_resolve_path(argv[1], path, sizeof(path));

    /* Try Disk (FAT32) first */
    fat_automount();

    char *buf = (char*)kmalloc(4096);
    if (buf) {
        int bytes = fat32_read_file(path, buf, 4095);
        if (bytes >= 0) {
            buf[bytes] = 0;
            terminal_writestring(buf);
            terminal_writestring("\n");
            kfree(buf);
            return;
        }
        kfree(buf);
    }

    /* Fallback to RAMFS */
    const FSNode* node = fs_find(path);
    if (!node) {
        terminal_writestring("file not found\n");
        return;
    }

    /* node->data este stocat ca buffer text în RAMFS */
    terminal_writestring((const char*)node->data);
}
