#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Structurile publice (folosite intern) */
struct IDTEntry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

struct IDTPointer {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/* API public pentru restul kernelului */
void idt_init(void);

/* Setează o intrare din IDT:
   num   - index (0..255)
   base  - adresa handler-ului (pointer)
   sel   - segment selector (de obicei 0x08)
   flags - flags (ex: 0x8E = intr gate ring0, 0xEE = intr gate DPL=3)
*/
void idt_set_gate(int num, uint32_t base, uint16_t sel, uint8_t flags);

#ifdef __cplusplus
}
#endif
