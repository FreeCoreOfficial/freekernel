#include <stdint.h>
#include "../os/kernel/storage/ata.h"

extern "C" int disk_read_sector(uint32_t lba, uint8_t* buf) {
    return ata_read_sector_retry(lba, buf, 3);
}

extern "C" int disk_write_sector(uint32_t lba, const uint8_t* buf) {
    return ata_write_sector_retry(lba, buf, 3);
}

extern "C" uint32_t disk_get_capacity(void) {
    uint16_t id[256];
    char model[41];
    uint32_t sectors = 0;
    if (ata_identify(id) == 0) {
        if (ata_decode_identify(id, model, sizeof(model), &sectors) == 0 && sectors != 0) {
            return sectors;
        }
    }
    return 262144; /* fallback 128MB */
}
