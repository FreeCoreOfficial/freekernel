#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define O_RDONLY 00
#define O_WRONLY 01
#define O_RDWR   02
#define O_CREAT  0100
#define O_TRUNC  01000
#define O_APPEND 02000
#define O_BINARY 0

int open(const char *pathname, int flags, ...);

#ifdef __cplusplus
}
#endif