#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Inițializează subsistemul ACPI:
   - Caută RSDP
   - Parsează RSDT
   - Mapează tabelele în memorie
   - Loghează erorile pe portul serial pentru debug
*/
void acpi_init(void);

#ifdef __cplusplus
}
#endif
