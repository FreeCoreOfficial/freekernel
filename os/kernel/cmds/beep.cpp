#include "../drivers/speaker.h"
#include "../terminal.h"

extern "C" void cmd_beep(const char* args)
{
    (void)args;
    terminal_writestring("Beep!\n");
    speaker_beep(440, 200);
}
