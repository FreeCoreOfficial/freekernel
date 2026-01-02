// kernel/time/timer.c
#include "timer.h"
#include "../drivers/pit.h"
#include "../interrupts/irq.h"
#include "../interrupts/isr.h"
#include <stdint.h>

/* PIC EOI helper (master PIC only) */
static inline void pic_send_eoi(void)
{
    uint8_t val = 0x20;
    uint16_t port = 0x20;
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* tick counter (updated from IRQ) */
static volatile uint64_t ticks = 0;
static uint32_t tick_hz = 100;

/* IRQ0 handler */
static void timer_irq_handler(registers_t* regs)
{
    (void)regs;
    ticks++;
    pic_send_eoi();
}

void timer_init(uint32_t frequency)
{
    if (frequency == 0)
        return;

    tick_hz = frequency;
    ticks = 0;

    pit_init(frequency);
    irq_install_handler(0, timer_irq_handler);
}

uint64_t timer_ticks(void)
{
    return ticks;
}

/* ===== uptime helpers (NO 64-bit division) ===== */

uint32_t timer_uptime_seconds(void)
{
    if (tick_hz == 0)
        return 0;

    uint32_t t = (uint32_t)ticks;
    return t / tick_hz;
}

uint32_t timer_uptime_ms(void)
{
    if (tick_hz == 0)
        return 0;

    uint32_t t = (uint32_t)ticks;
    return (t * 1000) / tick_hz;
}

/* sleep using PIT + HLT */
void sleep(uint32_t ms)
{
    if (tick_hz == 0)
        return;

    uint32_t wait_ticks = (ms * tick_hz) / 1000;
    if (wait_ticks == 0)
        wait_ticks = 1;

    uint64_t start = ticks;

    while ((ticks - start) < wait_ticks)
    {
        asm volatile("hlt");
    }
}
