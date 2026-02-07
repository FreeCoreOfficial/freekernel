#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize IOAPIC with physical base address and GSI base */
bool ioapic_init(uint32_t base_addr, uint32_t gsi_base);

/* Enable a specific IRQ (0-23) mapping it to a vector */
void ioapic_enable_irq(uint8_t irq);
void ioapic_mask_all(void);

/* Enable a specific IRQ using a given GSI and ACPI flags */
void ioapic_enable_irq_gsi(uint8_t irq, uint32_t gsi, uint16_t flags);

#ifdef __cplusplus
}
#endif
