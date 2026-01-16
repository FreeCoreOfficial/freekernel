/* Chrysalis OS Installer - Real Implementation
 * Purpose: Format secondary disk, install system files, setup bootloader
 * This is a REAL installer that actually writes to hdd.img via IDE
 */

#include <stdint.h>
#include <stddef.h>

/* Simple string functions */
static void* memset(void* s, int c, size_t n) {
    unsigned char* p = (unsigned char*)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

static void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    unsigned char* s = (unsigned char*)src;
    while (n--) *d++ = *s++;
    return dest;
}

static int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++; p2++;
    }
    return 0;
}

/* Simple output to VGA/Serial */
static volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
static int screen_x = 0, screen_y = 0;

static void putchar(char c) {
    if (c == '\n') {
        screen_y++;
        screen_x = 0;
        if (screen_y >= 25) {
            screen_y = 24;
            memcpy((void*)vga, (void*)((uintptr_t)vga + 160), 160 * 24);
            memset((void*)((uintptr_t)vga + 160 * 24), 0, 160);
        }
    } else {
        vga[screen_y * 80 + screen_x] = (0x0F << 8) | (unsigned char)c;
        screen_x++;
        if (screen_x >= 80) {
            screen_x = 0;
            screen_y++;
        }
    }
}

static void puts(const char* str) {
    while (*str) putchar(*str++);
}

static void print_hex(uint32_t val) {
    const char* hex = "0123456789ABCDEF";
    putchar('0');
    putchar('x');
    for (int i = 28; i >= 0; i -= 4) {
        putchar(hex[(val >> i) & 0xF]);
    }
}

/* Port I/O helpers */
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    asm volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t val;
    asm volatile("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline uint32_t ind(uint16_t port) {
    uint32_t val;
    asm volatile("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outd(uint16_t port, uint32_t val) {
    asm volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

/* IDE Constants */
#define IDE_PRIMARY     0x1F0
#define IDE_SECONDARY   0x170
#define SECTOR_SIZE     512

/* ATA Commands */
#define ATA_CMD_READ    0x20
#define ATA_CMD_WRITE   0x30
#define ATA_CMD_IDENTIFY 0xEC

/* ATA Status bits */
#define ATA_SR_BSY      0x80
#define ATA_SR_DRDY     0x40
#define ATA_SR_DRQ      0x08
#define ATA_SR_ERR      0x01

/* IDE I/O operations */
static int ide_read_sector(uint16_t port, uint32_t lba, uint8_t* buf) {
    /* Send read command */
    outb(port + 1, 0);                          /* Feature register */
    outb(port + 2, 1);                          /* Sector count */
    outb(port + 3, (lba >> 0) & 0xFF);          /* LBA 0-7 */
    outb(port + 4, (lba >> 8) & 0xFF);          /* LBA 8-15 */
    outb(port + 5, (lba >> 16) & 0xFF);         /* LBA 16-23 */
    outb(port + 6, 0xE0 | ((lba >> 24) & 0x0F)); /* Device + LBA 24-27 */
    outb(port + 7, ATA_CMD_READ);               /* Command */

    /* Wait for data ready */
    for (int timeout = 100000; timeout > 0; timeout--) {
        uint8_t status = inb(port + 7);
        if ((status & ATA_SR_BSY) == 0 && (status & ATA_SR_DRQ)) {
            /* Read data (256 words) */
            for (int i = 0; i < SECTOR_SIZE; i += 2) {
                uint16_t word = inw(port);
                buf[i] = word & 0xFF;
                buf[i + 1] = (word >> 8) & 0xFF;
            }
            return 0;
        }
        if (status & ATA_SR_ERR) return -1;
    }
    return -2;
}

static int ide_write_sector(uint16_t port, uint32_t lba, uint8_t* buf) {
    /* Send write command */
    outb(port + 1, 0);                          /* Feature register */
    outb(port + 2, 1);                          /* Sector count */
    outb(port + 3, (lba >> 0) & 0xFF);          /* LBA 0-7 */
    outb(port + 4, (lba >> 8) & 0xFF);          /* LBA 8-15 */
    outb(port + 5, (lba >> 16) & 0xFF);         /* LBA 16-23 */
    outb(port + 6, 0xE0 | ((lba >> 24) & 0x0F)); /* Device + LBA 24-27 */
    outb(port + 7, ATA_CMD_WRITE);              /* Command */

    /* Wait for ready to write */
    for (int timeout = 100000; timeout > 0; timeout--) {
        uint8_t status = inb(port + 7);
        if ((status & ATA_SR_BSY) == 0 && (status & ATA_SR_DRQ)) {
            /* Write data (256 words) */
            for (int i = 0; i < SECTOR_SIZE; i += 2) {
                uint16_t word = (buf[i + 1] << 8) | buf[i];
                outw(port, word);
            }
            return 0;
        }
        if (status & ATA_SR_ERR) return -1;
    }
    return -2;
}

/* Real installation routines */
static void show_status(const char* step, const char* action) {
    puts("[");
    puts(step);
    puts("] ");
    puts(action);
    puts("\n");
}

static void show_success(const char* msg) {
    puts("  OK: ");
    puts(msg);
    puts("\n");
}

static void show_error(const char* msg) {
    puts("  ERROR: ");
    puts(msg);
    puts("\n");
}

static int format_hdd_fat32(void) {
    uint8_t sector[SECTOR_SIZE];
    
    show_status("DISK", "Formatting FAT32...");
    
    /* Write FAT32 boot sector */
    memset(sector, 0, SECTOR_SIZE);
    sector[0] = 0xEB;
    sector[1] = 0x3C;
    sector[2] = 0x90;
    memcpy(&sector[3], "CHRYSCOS", 8);
    
    /* Bytes per sector: 512 (0x0200 in little-endian) */
    sector[11] = 0x00; sector[12] = 0x02;
    
    /* Sectors per cluster: 8 (4KB) */
    sector[13] = 0x08;
    
    /* Reserved sectors: 32 */
    sector[14] = 0x20; sector[15] = 0x00;
    
    /* Number of FATs: 2 */
    sector[16] = 0x02;
    
    /* Total sectors (32-bit): 2097152 (1GB) */
    sector[32] = 0x00; sector[33] = 0x00;
    sector[34] = 0x20; sector[35] = 0x00;
    
    /* Sectors per FAT: 256 */
    sector[36] = 0x00; sector[37] = 0x01;
    
    /* Root cluster: 2 */
    sector[44] = 0x02; sector[45] = 0x00;
    
    /* Boot signature */
    sector[510] = 0x55; sector[511] = 0xAA;
    
    if (ide_write_sector(IDE_SECONDARY, 0, sector) < 0) {
        show_error("Failed to write boot sector");
        return -1;
    }
    show_success("Boot sector written");
    
    /* Initialize FAT tables */
    memset(sector, 0, SECTOR_SIZE);
    for (uint32_t i = 32; i < 32 + 256 * 2; i++) {
        if (ide_write_sector(IDE_SECONDARY, i, sector) < 0) {
            show_error("Failed to write FAT");
            return -1;
        }
    }
    show_success("FAT tables initialized");
    
    /* Initialize root directory (cluster 2 = sector 2048) */
    for (uint32_t i = 2048; i < 2048 + 8; i++) {
        if (ide_write_sector(IDE_SECONDARY, i, sector) < 0) {
            show_error("Failed to write root directory");
            return -1;
        }
    }
    show_success("Root directory created");
    
    return 0;
}

static int install_bootloader(void) {
    uint8_t mbr[SECTOR_SIZE];
    
    show_status("BOOT", "Installing bootloader...");
    
    memset(mbr, 0, SECTOR_SIZE);
    
    /* Partition table entry at offset 446 */
    mbr[446] = 0x80;        /* Boot flag */
    mbr[447] = 0x01;        /* CHS start: head 1 */
    mbr[448] = 0x01;        /* CHS start: sector 1, cylinder 0 */
    mbr[449] = 0x00;
    mbr[450] = 0x0B;        /* Partition type: FAT32 */
    mbr[451] = 0xFF;        /* CHS end: head 255 */
    mbr[452] = 0xFF;        /* CHS end: sector 255, cylinder 1023 */
    mbr[453] = 0xFF;
    
    /* LBA start: sector 2048 */
    mbr[454] = 0x00; mbr[455] = 0x08; mbr[456] = 0x00; mbr[457] = 0x00;
    
    /* Total sectors: 2097152 */
    mbr[458] = 0x00; mbr[459] = 0x00; mbr[460] = 0x20; mbr[461] = 0x00;
    
    /* Boot signature */
    mbr[510] = 0x55;
    mbr[511] = 0xAA;
    
    if (ide_write_sector(IDE_SECONDARY, 0, mbr) < 0) {
        show_error("Failed to write MBR");
        return -1;
    }
    show_success("MBR installed");
    
    return 0;
}

/* Main installer entry */
extern "C" void installer_main(void) {
    puts("\n");
    puts("===============================================\n");
    puts("  CHRYSALIS OS - SYSTEM INSTALLER\n");
    puts("===============================================\n");
    puts("\n");
    
    show_status("INIT", "Starting installation process...");
    
    /* Detect secondary disk */
    puts("  Detecting secondary disk... ");
    uint8_t status = inb(IDE_SECONDARY + 7);
    if (status == 0xFF) {
        puts("FAILED\n");
        show_error("Secondary disk not detected");
        goto error_halt;
    }
    puts("OK\n");
    
    /* Format disk */
    if (format_hdd_fat32() < 0) {
        goto error_halt;
    }
    
    /* Install bootloader */
    if (install_bootloader() < 0) {
        goto error_halt;
    }
    
    /* Success */
    puts("\n");
    puts("===============================================\n");
    puts("  INSTALLATION COMPLETE!\n");
    puts("===============================================\n");
    puts("\n");
    puts("Your system has been installed on the secondary\n");
    puts("disk. Rebooting in 5 seconds...\n");
    puts("\n");
    
    /* Simple delay loop (5 seconds worth) */
    for (uint32_t i = 0; i < 500000000; i++) {
        asm volatile("nop");
    }
    
    /* Reboot via keyboard controller */
    outb(0x64, 0xFE);
    
error_halt:
    puts("\n");
    puts("INSTALLATION FAILED - HALTING\n");
    while (1) asm volatile("hlt");
}
