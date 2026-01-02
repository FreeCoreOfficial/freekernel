#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Init / probe */
void ata_init(void);

/* IDENTIFY DEVICE
 * buffer must be at least 256 uint16_t words (512 bytes)
 * returns 0 on success, negative on error:
 *  -1: no device (status == 0)
 *  -2: error bit set
 *  -3: timeout / didn't set DRQ
 */
int ata_identify(uint16_t* buffer);

/* Decode identify buffer:
 * - model: output ASCII nul-terminated (model_len bytes available)
 * - lba28_sectors: total sectors (if supported), 0 otherwise
 * returns 0 on success
 */
int ata_decode_identify(const uint16_t* id_buf, char* model, size_t model_len, uint32_t* lba28_sectors);

/* Low-level PIO read (LBA28), reads 1 sector (512 bytes) */
int ata_pio_read28(uint32_t lba, uint8_t* buffer);

/* Convenience read wrapper */
int ata_read_sector(uint32_t lba, uint8_t* buffer);

/* Low-level PIO write (LBA28), writes 1 sector (512 bytes).
 * Returns 0 on success, negative on error.
 * Be careful: sector 0 (MBR) is protected unless you enable it with ata_set_allow_mbr_write().
 */
int ata_write_sector(uint32_t lba, const uint8_t* buffer);

/* Enable/disable writing to MBR (sector 0). Default: disabled. */
void ata_set_allow_mbr_write(int enabled);

#ifdef __cplusplus
}
#endif
