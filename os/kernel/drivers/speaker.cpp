#include "speaker.h"
#include "../arch/i386/io.h"
#include "pit.h"

/*
 PC speaker control:
 bit 0 = gate
 bit 1 = speaker enable
 port 0x61
*/

static void speaker_enable()
{
    uint8_t val = inb(0x61);
    if ((val & 3) != 3)
        outb(0x61, val | 3);
}

static void speaker_disable()
{
    uint8_t val = inb(0x61);
    outb(0x61, val & ~3);
}

void speaker_on(uint32_t freq)
{
    if (!freq) return;

    uint32_t divisor = 1193180 / freq;

    // PIT channel 2
    outb(0x43, 0xB6);
    outb(0x42, divisor & 0xFF);
    outb(0x42, (divisor >> 8) & 0xFF);

    speaker_enable();
}

void speaker_off()
{
    speaker_disable();
}

void speaker_beep(uint32_t freq, uint32_t ms)
{
    speaker_on(freq);
    pit_sleep(ms);
    speaker_off();
}
