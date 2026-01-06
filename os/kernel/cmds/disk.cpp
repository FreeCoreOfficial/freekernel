/* kernel/cmds/disk.cpp - Linux-style disk utilities */
#include "disk.h"
#include <stdint.h>
#include <stddef.h>

#include "../storage/block.h"
#include "../storage/ata.h"
#include "../terminal.h"
#include "../string.h"
#include "../mem/kmalloc.h"

/* Global partition table */
struct part_assign g_assigns[26];

static void init_assigns_table(void) {
    for (int i = 0; i < 26; i++) {
        g_assigns[i].used = 0;
    }
}

/* --- Helpers --- */

static block_device_t* get_main_disk(void) {
    block_device_t* bd = block_get("ahci0");
    if (!bd) bd = block_get("ata0");
    return bd;
}

static void print_size(uint64_t sectors) {
    uint64_t bytes = sectors * 512;
    if (bytes >= 1024*1024*1024) {
        uint32_t gb = (uint32_t)(bytes / (1024*1024*1024));
        uint32_t rem = (uint32_t)((bytes % (1024*1024*1024)) * 10 / (1024*1024*1024));
        terminal_printf("%u.%uG", gb, rem);
    } else if (bytes >= 1024*1024) {
        uint32_t mb = (uint32_t)(bytes / (1024*1024));
        terminal_printf("%uM", mb);
    } else if (bytes >= 1024) {
        uint32_t kb = (uint32_t)(bytes / 1024);
        terminal_printf("%uK", kb);
    } else {
        terminal_printf("%uB", (uint32_t)bytes);
    }
}

static const char* get_part_type_name(uint8_t type) {
    switch (type) {
        case 0x00: return "Empty";
        case 0x01: return "FAT12";
        case 0x04: return "FAT16 <32M";
        case 0x05: return "Extended";
        case 0x06: return "FAT16";
        case 0x07: return "NTFS/exFAT";
        case 0x0B: return "FAT32";
        case 0x0C: return "FAT32 LBA";
        case 0x83: return "Linux";
        case 0x82: return "Linux Swap";
        default:   return "Unknown";
    }
}

/* --- Exported API --- */

int disk_read_sector(uint32_t lba, uint8_t* buf) {
    block_device_t* bd = get_main_disk();
    if (bd) return bd->read(bd, lba, 1, buf);
    return -1;
}

int disk_write_sector(uint32_t lba, const uint8_t* buf) {
    block_device_t* bd = get_main_disk();
    if (bd) return bd->write(bd, lba, 1, buf);
    return -1;
}

uint32_t disk_get_capacity(void) {
    block_device_t* bd = get_main_disk();
    if (bd) return (uint32_t)bd->sector_count;
    return 0;
}

/* --- Commands --- */

/* Like 'lsblk' */
static void cmd_lsblk(void) {
    terminal_writestring("NAME    MAJ:MIN RM  SIZE RO TYPE MOUNTPOINTS\n");

    char name[32];
    for (int i = 0; i < 4; i++) {
        /* Construct name ahci0..3 */
        name[0]='a'; name[1]='h'; name[2]='c'; name[3]='i'; 
        name[4]='0'+i; name[5]=0;

        block_device_t* bd = block_get(name);
        if (bd) {
            terminal_printf("%s    8:%d    0  ", bd->name, i*16);
            print_size(bd->sector_count);
            terminal_writestring("  0 disk \n");
        }
    }

    block_device_t* ata = block_get("ata0");
    if (ata) {
        terminal_printf("%s     3:0    0  ", ata->name);
        print_size(ata->sector_count);
        terminal_writestring("  0 disk \n");
    }
}

/* Like 'fdisk -l' */
static void cmd_fdisk_list(void) {
    block_device_t* bd = get_main_disk();
    if (!bd) {
        terminal_writestring("fdisk: cannot open disk: No medium found\n");
        return;
    }

    uint64_t bytes = bd->sector_count * 512;
    terminal_printf("Disk /dev/%s: ", bd->name);
    print_size(bd->sector_count);
    terminal_printf(", %u bytes, %u sectors\n", (uint32_t)bytes, (uint32_t)bd->sector_count);
    terminal_writestring("Units: sectors of 1 * 512 = 512 bytes\n");
    terminal_writestring("Sector size (logical/physical): 512 bytes / 512 bytes\n");
    terminal_writestring("Disklabel type: dos\n\n");

    uint8_t* mbr = (uint8_t*)kmalloc(512);
    if (!mbr || bd->read(bd, 0, 1, mbr) != 0) {
        terminal_writestring("fdisk: unable to read MBR\n");
        if(mbr) kfree(mbr);
        return;
    }

    if (mbr[510] != 0x55 || mbr[511] != 0xAA) {
        terminal_writestring("Device does not contain a recognized partition table.\n");
        kfree(mbr);
        return;
    }

    terminal_writestring("Device        Boot   Start       End   Sectors  Size Id Type\n");

    for (int i = 0; i < 4; ++i) {
        int off = 446 + i * 16;
        uint8_t type = mbr[off + 4];
        uint32_t lba = *(uint32_t*)(mbr + off + 8);
        uint32_t count = *(uint32_t*)(mbr + off + 12);
        uint8_t boot = mbr[off];

        if (type == 0 || count == 0) continue;

        terminal_printf("/dev/%sp%d  %c   %8u  %8u  %8u  ", 
            bd->name, i+1, (boot & 0x80) ? '*' : ' ', lba, lba + count - 1, count);
        print_size(count);
        terminal_printf(" %2x %s\n", type, get_part_type_name(type));
    }
    
    kfree(mbr);
}

/* Internal: Scan and assign letters (like mounting) */
static void cmd_probe(void) {
    block_device_t* bd = get_main_disk();
    if (!bd) return;

    uint8_t* mbr = (uint8_t*)kmalloc(512);
    if (!mbr || bd->read(bd, 0, 1, mbr) != 0) {
        if(mbr) kfree(mbr);
        return;
    }

    if (mbr[510] != 0x55 || mbr[511] != 0xAA) {
        kfree(mbr);
        return;
    }

    init_assigns_table();
    int found = 0;

    for (int i = 0; i < 4; ++i) {
        int off = 446 + i * 16;
        uint8_t type = mbr[off + 4];
        uint32_t lba = *(uint32_t*)(mbr + off + 8);
        uint32_t count = *(uint32_t*)(mbr + off + 12);

        if (type == 0) continue;

        int slot = -1;
        for (int j = 0; j < 26; ++j) {
            if (!g_assigns[j].used) { slot = j; break; }
        }
        
        if (slot >= 0) {
            g_assigns[slot].used = 1;
            g_assigns[slot].letter = 'a' + slot;
            g_assigns[slot].lba = lba;
            g_assigns[slot].count = count;
            g_assigns[slot].type = type;
            found++;
        }
    }
    
    kfree(mbr);
    terminal_printf("Probed %d partitions. Use 'disk list' to see details.\n", found);
}

/* Create a fresh MBR with one partition */
static void cmd_mklabel(void) {
    block_device_t* bd = get_main_disk();
    if (!bd) {
        terminal_writestring("No disk found.\n");
        return;
    }

    uint8_t* mbr = (uint8_t*)kmalloc(512);
    if (!mbr) return;
    memset(mbr, 0, 512);
    
    uint32_t sectors = (uint32_t)bd->sector_count;
    uint32_t start = 2048;
    uint32_t size = sectors - start;
    
    int off = 446;
    mbr[off] = 0x80;     // Bootable
    mbr[off + 4] = 0x0B; // Type: FAT32
    *(uint32_t*)(mbr + off + 8) = start;
    *(uint32_t*)(mbr + off + 12) = size;
    
    mbr[510] = 0x55;
    mbr[511] = 0xAA;

    ata_set_allow_mbr_write(1);
    int r = bd->write(bd, 0, 1, mbr);
    ata_set_allow_mbr_write(0);

    if (r == 0) {
        terminal_writestring("Created a new DOS disklabel with disk identifier 0x00000000.\n");
        terminal_writestring("Created a new partition 1 of type 'FAT32' and of size ");
        print_size(size);
        terminal_writestring(".\n");
    } else {
        terminal_writestring("Write failed.\n");
    }
    
    kfree(mbr);
}

static void cmd_usage(void) {
    terminal_writestring("Usage: disk <command>\n");
    terminal_writestring("Commands:\n");
    terminal_writestring("  list     List block devices (lsblk)\n");
    terminal_writestring("  print    Show partition table (fdisk -l)\n");
    terminal_writestring("  probe    Scan and assign partitions\n");
    terminal_writestring("  mklabel  Create fresh MBR with 1 partition\n");
    terminal_writestring("  read     Read sector 0 (test)\n");
}

void cmd_disk(int argc, char** argv)
{
    static bool initialized = false;
    if (!initialized) {
        init_assigns_table();
        initialized = true;
    }

    if (argc < 2) { cmd_usage(); return; }
    const char* sub = argv[1];

    if (strcmp(sub, "list") == 0)    { cmd_lsblk(); return; }
    if (strcmp(sub, "print") == 0)   { cmd_fdisk_list(); return; }
    if (strcmp(sub, "probe") == 0)   { cmd_probe(); return; }
    if (strcmp(sub, "mklabel") == 0) { cmd_mklabel(); return; }
    
    if (strcmp(sub, "read") == 0) {
        uint8_t* buf = (uint8_t*)kmalloc(512);
        if (disk_read_sector(0, buf) == 0) {
            terminal_printf("Read LBA 0 OK. Signature: %x %x\n", buf[510], buf[511]);
        } else {
            terminal_writestring("Read failed.\n");
        }
        kfree(buf);
        return;
    }

    cmd_usage();
}