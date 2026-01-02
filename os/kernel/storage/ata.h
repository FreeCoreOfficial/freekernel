#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* init (optional la început) */
void ata_init(void);

/* read 1 sector (512 bytes), LBA28 */
int ata_pio_read28(uint32_t lba, uint8_t* buffer);

#ifdef __cplusplus
}
#endif
