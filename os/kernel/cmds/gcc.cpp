#include "gcc.h"
#include "../fs/vfs/fs_ops.h"
#include "../fs/vfs/vfs.h"
#include "../fs/vfs/vnode.h"
#include "../mem/kmalloc.h"
#include "../string.h"
#include "../terminal.h"
#include "../toolchain/cc.h"

int cmd_gcc(int argc, char **argv) {
  if (argc < 2) {
    terminal_writestring("Usage: gcc <source_code_string> OR gcc <file.c>\n");
    return -1;
  }

  // 1. Check if it's a file first (only if 1 argument is provided)
  if (argc == 2) {
    vnode_t *node = vfs_resolve(argv[1]);
    if (node) {
      char *buf = (char *)kmalloc(4096);
      int bytes = node->ops->read(node, 0, (uint8_t *)buf, 4095);
      if (bytes > 0) {
        buf[bytes] = '\0';
        toolchain_compile_and_run(buf);
        terminal_putchar('\n'); // Ensure prompt starts on new line
      }
      kfree(buf);
      return 0;
    }
  }

  // 2. It's a string (potentially split into multiple argv parts if not quoted)
  char *full_code = (char *)kmalloc(4096);
  full_code[0] = '\0';
  for (int i = 1; i < argc; i++) {
    strcat(full_code, argv[i]);
    if (i < argc - 1)
      strcat(full_code, " ");
  }

  toolchain_compile_and_run(full_code);
  terminal_putchar('\n'); // Ensure prompt starts on new line

  kfree(full_code);
  return 0;
}
