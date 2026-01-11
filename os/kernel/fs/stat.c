#include <sys/stat.h>
#include <errno.h>

int stat(const char* path, struct stat* st)
{
    (void)path;

    if (!st) {
        errno = EINVAL;
        return -1;
    }

    /* Treat everything as a regular file with unknown size */
    st->st_mode = S_IFREG;
    st->st_size = 0;
    return 0;
}

int fstat(int fd, struct stat* st)
{
    (void)fd;
    return stat(NULL, st);
}
