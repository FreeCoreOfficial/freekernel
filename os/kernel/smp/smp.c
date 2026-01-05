#include "smp.h"
#include "../hardware/acpi.h"
#include "../hardware/lapic.h"
#include "../hardware/apic.h"
#include "../drivers/serial.h"
#include "../drivers/pit.h"
#include "../arch/i386/gdt.h"
#include "../arch/i386/idt.h"

cpu_info_t cpus[MAX_CPUS];
volatile int cpu_count = 0;

/*
 * Main C entry point for Application Processors (APs).
 * This function is called by the trampoline code.
 */
void ap_main(void) {
    uint32_t id = lapic_get_id();
    serial_printf("[SMP] AP started! APIC ID=%d\n", id);

    // Each AP needs its own GDT and IDT
    // NOTE: These functions must be AP-safe.
    // If they modify global state without locks, you'll have races.
    gdt_init();
    idt_init();

    // Enable APIC on this core
    // The base address is the same for all cores.
    struct MADT* madt = (struct MADT*)acpi_get_madt();
    if (madt) {
        lapic_init(madt->local_apic_addr);
    }

    // Mark this CPU as online
    for (int i = 0; i < cpu_count; i++) {
        if (cpus[i].apic_id == id) {
            cpus[i].online = 1;
            break;
        }
    }

    serial_printf("[SMP] AP %d online\n", id);

    // Enable interrupts on this core
    asm volatile("sti");

    // Halt until an interrupt arrives
    while (1) {
        asm volatile("hlt");
    }
}

void smp_detect_cpus(void) {
    serial_printf("[SMP] Detecting CPUs via MADT...\n");

    struct MADT* madt = (struct MADT*)acpi_get_madt();
    if (!madt) {
        serial_printf("[SMP] MADT not found. Cannot detect CPUs.\n");
        cpu_count = 1; // Only BSP
        cpus[0].apic_id = lapic_get_id();
        cpus[0].online = 1;
        return;
    }

    uint8_t* start = (uint8_t*)madt + sizeof(struct MADT);
    uint8_t* end   = (uint8_t*)madt + ((struct MADT*)madt)->h.length;

    while (start < end) {
        MADT_Record* rec = (MADT_Record*)start;
        if (rec->type == 0 && cpu_count < MAX_CPUS) { // Local APIC
            struct MADT_LocalAPIC* lapic = (struct MADT_LocalAPIC*)rec;
            if (lapic->flags & 1) { // Enabled
                cpus[cpu_count].apic_id = lapic->apic_id;
                cpus[cpu_count].online = (cpu_count == 0); // BSP is initially online
                serial_printf("[SMP] Found CPU %d (APIC ID=%d)\n", cpu_count, lapic->apic_id);
                cpu_count++;
            }
        }
        start += rec->length;
    }

    if (cpu_count == 0) {
        serial_printf("[SMP] Warning: No CPUs found in MADT, assuming 1 (BSP).\n");
        cpu_count = 1;
        cpus[0].apic_id = lapic_get_id();
        cpus[0].online = 1;
    }

    serial_printf("[SMP] Total CPUs detected: %d\n", cpu_count);
}