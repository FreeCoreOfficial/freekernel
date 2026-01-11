#pragma once
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_OK 0

int close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
off_t lseek(int fd, off_t offset, int whence);
int unlink(const char *pathname);

/* Redenumim sleep pentru a evita conflictul cu sleep(ms) din kernel */
#define sleep posix_sleep
unsigned int posix_sleep(unsigned int seconds);
int usleep(unsigned int usec);
int access(const char *pathname, int mode);
int isatty(int fd);

#ifdef __cplusplus
}
#endif