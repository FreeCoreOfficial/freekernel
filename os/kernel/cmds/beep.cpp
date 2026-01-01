#include "../drivers/speaker.h"
#include "../shortcuts/shortcuts.h"
#include "../terminal.h"

extern "C" void cmd_beep(const char*)
{
    terminal_writestring("Beep (Ctrl+C to stop)\n");

    speaker_on(440);

    while (1) {
        asm volatile("hlt");

        if (shortcuts_poll_ctrl_c())
            break;
    }

    speaker_stop();
    terminal_writestring("Stopped.\n");
}
