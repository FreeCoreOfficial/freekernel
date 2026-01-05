#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Inițializează subsistemul ACPI:
   - Caută RSDP
   - Parsează RSDT
   - Mapează tabelele în memorie
   - Parsează MADT (APIC) și FADT
   - Instalează handler pentru SCI (System Control Interrupt)
   - Loghează erorile pe portul serial pentru debug
*/
void acpi_init(void);

/* Încearcă oprirea sistemului folosind ACPI (S5 Soft Off) */
void acpi_shutdown(void);

/* Returnează numărul de CPU-uri detectate în MADT */
uint8_t acpi_get_cpu_count(void);

/* Returnează true dacă ACPI este activ și funcțional */
bool acpi_is_enabled(void);

#ifdef __cplusplus
}
#endif
