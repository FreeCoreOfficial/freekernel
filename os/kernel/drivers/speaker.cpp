#include "speaker.h"
#include "../arch/i386/io.h"
#include "../drivers/pit.h"

#define PIT_CHANNEL2  0x42
#define PIT_CMD       0x43
#define SPEAKER_PORT  0x61

void speaker_init()
{
}

void speaker_on(uint32_t freq)
{
    if (freq == 0)
        return;

    uint32_t divisor = 1193180 / freq;

    outb(PIT_CMD, 0xB6);
    outb(PIT_CHANNEL2, divisor & 0xFF);
    outb(PIT_CHANNEL2, (divisor >> 8) & 0xFF);

    uint8_t tmp = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, tmp | 3);
}

void speaker_off()
{
    uint8_t tmp = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, tmp & ~3);
}

void speaker_beep(uint32_t freq, uint32_t ms)
{
    speaker_on(freq);
    pit_sleep(ms);
    speaker_off();
}
