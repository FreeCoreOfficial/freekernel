#include "../drivers/speaker.h"
#include "../terminal.h"

/* delay foarte simplu (aprox, depinde de CPU/QEMU) */
static void delay_loop(unsigned int loops)
{
    for (unsigned int i = 0; i < loops; i++) {
        asm volatile("pause");
    }
}

extern "C" void cmd_beep(const char*)
{
    terminal_writestring("Beep (5 seconds)...\n");

    speaker_on(440);

    /* ~5 secunde (ajustezi dacă vrei mai lung/scurt) */
    for (int i = 0; i < 5; i++) {
        delay_loop(80 * 1000 * 100);  // aproximativ 1s în QEMU
    }

    speaker_stop();
    terminal_writestring("Stopped.\n");
}
