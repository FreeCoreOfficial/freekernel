#include "timer.h"
#include "../drivers/pit.h"
#include "../interrupts/irq.h"
#include "../interrupts/isr.h"

static volatile uint64_t ticks = 0;

static void timer_irq_handler(registers_t* regs)
{
    (void)regs;
    ticks++;
}

void timer_init(uint32_t freq)
{
    ticks = 0;

    pit_init(freq);
    irq_install_handler(0, timer_irq_handler);
}

uint64_t timer_ticks(void)
{
    return ticks;
}

void sleep(uint32_t ms)
{
    uint64_t target = ticks + ms;
    while (ticks < target) {
        asm volatile("hlt");
    }
}
