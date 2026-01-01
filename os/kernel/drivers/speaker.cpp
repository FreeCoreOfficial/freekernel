#include "speaker.h"
#include "../arch/i386/io.h"
#include "pit.h"

static void speaker_enable()
{
    uint8_t tmp = inb(0x61);
    if ((tmp & 3) != 3)
        outb(0x61, tmp | 3);
}

static void speaker_disable()
{
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp & ~3);
}

void speaker_on(uint32_t freq)
{
    if (freq == 0)
        return;

    uint32_t div = 1193180 / freq;

    outb(0x43, 0xB6);          // PIT channel 2
    outb(0x42, div & 0xFF);
    outb(0x42, (div >> 8) & 0xFF);

    speaker_enable();
}

void speaker_stop()
{
    speaker_disable();
}

void speaker_beep(uint32_t freq, uint32_t ms)
{
    speaker_on(freq);
    pit_sleep(ms);
    speaker_stop();
}
