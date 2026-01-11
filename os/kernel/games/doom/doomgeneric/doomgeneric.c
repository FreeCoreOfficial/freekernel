#include <stddef.h>

#include "m_argv.h"

#include "doomgeneric.h"

pixel_t* DG_ScreenBuffer = NULL;

extern void* malloc(size_t size);
extern void serial(const char *fmt, ...);

void M_FindResponseFile(void);
void D_DoomMain (void);


void doomgeneric_Create(int argc, char **argv)
{
    serial("[DOOM] doomgeneric_Create called\n");
	// save arguments
    myargc = argc;
    myargv = argv;

    serial("[DOOM] M_FindResponseFile...\n");
	M_FindResponseFile();

    serial("[DOOM] Allocating screen buffer...\n");
	DG_ScreenBuffer = malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);

    serial("[DOOM] DG_Init...\n");
	DG_Init();

    serial("[DOOM] D_DoomMain...\n");
	D_DoomMain ();
    serial("[DOOM] D_DoomMain returned\n");
}
