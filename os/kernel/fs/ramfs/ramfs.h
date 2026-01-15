#pragma once

#include <stddef.h>
#include "../vfs/vnode.h"

#ifdef __cplusplus
extern "C" {
#endif

/* returns the ramfs root vnode (statically allocated) */
struct vnode* ramfs_root(void);

void ramfs_create_file(const char* name, const void* data, size_t len);

#ifdef __cplusplus
}
#endif
