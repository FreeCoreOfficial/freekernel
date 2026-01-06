#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Partition Assignment (Legacy support for FAT driver) */
struct part_assign {
    int used;
    char letter;
    uint32_t lba;
    uint32_t count;
    uint8_t type;
};

/* Global partition table (accessed by FAT/VFS) */
extern struct part_assign g_assigns[26];

/* Unified I/O API (AHCI/ATA auto-detect) */
int disk_read_sector(uint32_t lba, uint8_t* buf);
int disk_write_sector(uint32_t lba, const uint8_t* buf);
uint32_t disk_get_capacity(void);

/* Helper pentru automount: scanează partițiile și populează g_assigns */
void disk_probe_partitions(void);

/* Shell Command Entry Point */
void cmd_disk(int argc, char** argv);

#ifdef __cplusplus
}
#endif