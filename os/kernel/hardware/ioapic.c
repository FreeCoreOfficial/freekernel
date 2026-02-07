#include "ioapic.h"
#include "../paging.h"
#include "../drivers/serial.h"

// IOAPIC Registers
#define IOAPICID  0x00
#define IOAPICVER 0x01
#define IOAPICARB 0x02
#define IOREDTBL  0x10

// Default virtual address for IOAPIC (mapped to 0xFEC00000)
static volatile uint32_t* ioapic_base = (volatile uint32_t*)0xFEC00000;
static uint32_t ioapic_gsi_base = 0;
static uint32_t ioapic_max_redir = 0;

static uint32_t ioapic_read(uint32_t reg) {
    *(volatile uint32_t*)(ioapic_base) = reg;
    return *(volatile uint32_t*)((uint8_t*)ioapic_base + 0x10);
}

static void ioapic_write(uint32_t reg, uint32_t value) {
    *(volatile uint32_t*)(ioapic_base) = reg;
    *(volatile uint32_t*)((uint8_t*)ioapic_base + 0x10) = value;
}

bool ioapic_init(uint32_t base_addr, uint32_t gsi_base) {
    // Map IOAPIC physical address to virtual address
    // Flags: Present | RW (0x3)
    paging_map_page((uint32_t)ioapic_base, base_addr, 0x3);

    ioapic_gsi_base = gsi_base;

    uint32_t ver = ioapic_read(IOAPICVER);
    uint32_t count = ((ver >> 16) & 0xFF) + 1;
    ioapic_max_redir = count;
    
    serial_printf("[ioapic] init: base=0x%x (phys=0x%x) gsi=%d max_redir=%d\n", 
                  (uint32_t)ioapic_base, base_addr, gsi_base, count);

    return true;
}

void ioapic_enable_irq(uint8_t irq) {
    ioapic_enable_irq_gsi(irq, irq, 0);
}

static uint32_t ioapic_build_low(uint8_t vector, uint16_t flags) {
    uint32_t low = vector; /* Fixed delivery, physical dest */

    /* Polarity: bits 1:0 */
    uint16_t pol = flags & 0x3;
    if (pol == 3) {
        low |= (1u << 13); /* active low */
    }

    /* Trigger: bits 3:2 */
    uint16_t trig = (flags >> 2) & 0x3;
    if (trig == 3) {
        low |= (1u << 15); /* level */
    }

    return low;
}

void ioapic_enable_irq_gsi(uint8_t irq, uint32_t gsi, uint16_t flags) {
    uint8_t vector = 0x20 + irq;

    if (gsi < ioapic_gsi_base || (gsi - ioapic_gsi_base) >= ioapic_max_redir) {
        serial_printf("[ioapic] GSI %u out of range (base=%u, max=%u)\n",
                      (unsigned)gsi, (unsigned)ioapic_gsi_base, (unsigned)ioapic_max_redir);
        return;
    }

    uint32_t index = gsi - ioapic_gsi_base;
    uint32_t low = ioapic_build_low(vector, flags);
    uint32_t high = 0; // Dest APIC ID 0 (BSP)

    ioapic_write(IOREDTBL + 2 * index, low);
    ioapic_write(IOREDTBL + 2 * index + 1, high);

    serial_printf("[ioapic] IRQ %d (GSI %u) -> vector 0x%x flags=0x%x\n",
                  irq, (unsigned)gsi, vector, flags);
}

void ioapic_mask_all(void) {
    if (ioapic_max_redir == 0) {
        uint32_t ver = ioapic_read(IOAPICVER);
        ioapic_max_redir = ((ver >> 16) & 0xFF) + 1;
    }
    for (uint32_t i = 0; i < ioapic_max_redir; i++) {
        uint32_t low = ioapic_read(IOREDTBL + 2 * i);
        low |= (1u << 16); /* mask bit */
        ioapic_write(IOREDTBL + 2 * i, low);
    }
    serial_printf("[ioapic] All IRQs masked\n");
}
