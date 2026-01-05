#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CPUS 8

typedef struct {
    uint8_t apic_id;
    int online;
    // Add other per-cpu data here later (e.g., current_task)
} cpu_info_t;

extern cpu_info_t cpus[MAX_CPUS];
extern volatile int cpu_count;

/* Detects CPUs from ACPI MADT */
void smp_detect_cpus(void);

/* Starts all Application Processors (APs) */
void smp_start_aps(void);

#ifdef __cplusplus
}
#endif