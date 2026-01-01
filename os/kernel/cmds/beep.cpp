#include "../terminal.h"
#include "../drivers/speaker.h"

extern "C" void cmd_beep(const char* args)
{
    (void)args;

    terminal_writestring("Beep!\n");
    speaker_beep(1000, 200);
}
