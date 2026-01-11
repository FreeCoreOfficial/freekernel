#pragma once
#include <stddef.h>
#include <stdint.h>
#include "types.h"

/* File type bits */
#define S_IFMT   0170000
#define S_IFDIR  0040000
#define S_IFREG  0100000

#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)

/* Minimal stat struct */
struct stat {
    uint32_t st_mode;   /* File type */
    uint32_t st_size;   /* File size in bytes */
};

/* Stub implementations */
int stat(const char* path, struct stat* st);
int fstat(int fd, struct stat* st);

int mkdir(const char* path, mode_t mode);
