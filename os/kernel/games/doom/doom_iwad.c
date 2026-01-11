#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Import serial for debug */
extern void serial(const char *fmt, ...);

__attribute__((aligned(4)))
const unsigned char doom_iwad[] = {
#include "freedoom1_wad.inc"
};

const unsigned int doom_iwad_len = sizeof(doom_iwad);

int doom_iwad_open(const char* name, void** data, size_t* size)
{
    const char* filename = name;

    if (filename[0] == '.' && filename[1] == '/')
        filename += 2;

    if (!strcasecmp(filename, "freedoom1.wad") ||
        !strcasecmp(filename, "doom.wad") ||
        !strcasecmp(filename, "doom1.wad"))
    {
        *data = (void*)doom_iwad;
        *size = doom_iwad_len;
        
        /* Debug info to confirm we hooked the right file */
        serial("[IWAD] doom_iwad_open: matched '%s', size=%u bytes\n", filename, doom_iwad_len);
        return 1;
    }

    return 0;
}
