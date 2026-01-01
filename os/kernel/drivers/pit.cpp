#include "pit.h"
#include "../interrupts/irq.h"

static volatile uint64_t pit_ticks = 0;

static void pit_callback(registers_t*) {
    pit_ticks++;
}

void pit_init(uint32_t frequency) {
    // frecvența de bază PIT
    uint32_t divisor = 1193180 / frequency;

    // comandă PIT
    outb(0x43, 0x36);

    // trimite divisor (low + high)
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);

    irq_install_handler(0, pit_callback);
}

uint64_t pit_get_ticks() {
    return pit_ticks;
}
