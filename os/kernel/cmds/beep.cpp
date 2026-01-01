#include "../drivers/speaker.h"
#include "../drivers/shortcuts.h"
#include "../terminal.h"

extern "C" void cmd_beep(const char*)
{
    terminal_writestring("Beep (Ctrl+C to stop)\n");

    while (!shortcut_ctrl_c()) {
        speaker_on(440);
    }

    speaker_stop();
}
