#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Main APIC Initialization */
void apic_init(void);

/* Check if APIC is active */
bool apic_is_active(void);
void apic_disable(void);
void apic_set_forced_off(bool off);
bool apic_is_forced_off(void);

#ifdef __cplusplus
}
#endif
