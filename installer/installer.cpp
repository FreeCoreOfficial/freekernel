#include <stdint.h>
#include <stddef.h>

#include "../os/kernel/include/types.h"
#include "../os/kernel/drivers/serial.h"
#include "../os/kernel/mem/kmalloc.h"
#include "../os/kernel/storage/ata.h"
#include "../os/kernel/cmds/disk.h"  /* For disk_init, disk_read_sector etc */
#include "../os/kernel/fs/ramfs/ramfs.h"
#include "../os/kernel/fs/fat/fat.h"
#include "../os/kernel/cmds/fat.h"
#include "../os/kernel/smp/multiboot.h"
#include "../os/kernel/arch/i386/io.h" /* for outb */
#include "../os/kernel/string.h"

/* Helper Prototypes */
int kmalloc_strcmp(const char* s1, const char* s2);
const char* kmalloc_strstr(const char* haystack, const char* needle);
size_t kmalloc_strlen(const char* s);

extern "C" {
    void kmalloc_init(void);
    int fat32_format(uint32_t lba, uint32_t sector_count, const char* label);
    int fat32_create_directory(const char* path);
    int fat32_create_directory_verified(const char* path, int verify);
    int fat32_create_file(const char* path, const void* data, uint32_t size);
    int fat32_create_file_verified(const char* path, const void* data, uint32_t size, int verify);
    void fat32_set_mounted(uint32_t lba, char letter);
    int fat32_read_directory(const char* path, fat_file_info_t* out, int max_entries);
    int32_t fat32_get_file_size(const char* path);
    void serial(const char *fmt, ...);
    void ata_init(void);
    void ata_set_skip_cache_flush(int enabled);
}

static void write_sectors(uint32_t lba, const void* data, uint32_t bytes) {
    const uint8_t* p = (const uint8_t*)data;
    uint8_t* tmp = (uint8_t*)kmalloc(512);
    if (!tmp) return;
    uint32_t sectors = (bytes + 511) / 512;
    for (uint32_t i = 0; i < sectors; i++) {
        uint32_t copy = 512;
        if ((i + 1) * 512 > bytes) copy = bytes - i * 512;
        memset(tmp, 0, 512);
        memcpy(tmp, p + i * 512, copy);
        if (disk_write_sector(lba + i, tmp) != 0) {
            serial("[INSTALLER] ERROR: disk_write_sector failed at LBA %u\n", lba + i);
            break;
        }
    }
    kfree(tmp);
}

static void build_mbr(uint8_t* mbr, const uint8_t* boot_img, uint32_t start_lba, uint32_t total_sectors) {
    memset(mbr, 0, 512);
    if (boot_img) {
        memcpy(mbr, boot_img, 446); /* boot code only */
    }
    /* Partition entry 0 */
    uint8_t* p = mbr + 446;
    p[0] = 0x80; /* bootable */
    p[1] = 0; p[2] = 0; p[3] = 0; /* CHS start (unused) */
    p[4] = 0x0C; /* FAT32 LBA */
    p[5] = 0; p[6] = 0; p[7] = 0; /* CHS end (unused) */
    p[8]  = (uint8_t)(start_lba & 0xFF);
    p[9]  = (uint8_t)((start_lba >> 8) & 0xFF);
    p[10] = (uint8_t)((start_lba >> 16) & 0xFF);
    p[11] = (uint8_t)((start_lba >> 24) & 0xFF);
    uint32_t count = (total_sectors > start_lba) ? (total_sectors - start_lba) : 0;
    p[12] = (uint8_t)(count & 0xFF);
    p[13] = (uint8_t)((count >> 8) & 0xFF);
    p[14] = (uint8_t)((count >> 16) & 0xFF);
    p[15] = (uint8_t)((count >> 24) & 0xFF);
    mbr[510] = 0x55;
    mbr[511] = 0xAA;
}


extern "C" void installer_main(uint32_t magic, uint32_t addr) {
    serial_init();
    serial("\n\n[INSTALLER] Starting Chrysalis OS Installer...\n");

    /* 1. Initialize Subsystems */
    kmalloc_init();
    
    serial("[INSTALLER] initializing ATA...\n");
    ata_init();
    ata_set_skip_cache_flush(1);

    /* 2. Format Target Disk (Primary Master) */
    serial("[INSTALLER] Preparing target disk...\n");
    ata_set_allow_mbr_write(1);
    
    // Quick & Dirty Partitioning:
    // 1. Get Capacity (using a fixed large size if identify fails or just assume 128MB+ for qemu)
    // Real ata_identify returns sectors. Let's try to just assume a partition at 2048.
    
    // Create MBR - Skipped for now, relying on raw device or pre-existing partition table for simple tests.
    // uint8_t* mbr = (uint8_t*)kmalloc(512);
    // ... clear ...
    // ... write partition 1 entry at offset 446 ...
    //   type 0x0C (FAT32 LBA)
    //   start 2048
    //   size = 200000 (approx 100MB dummy) or we need real size.
    //   active = 0x80
    
    // Actually, let's skip MBR creation for now and just format the "loop" device or assumed partition logic?
    // OS uses partitions. If I overwrite MBR, I need to be careful.
    // For specific task: "Standalone Installer". 
    // I will replace `disk_format_fat32(0)` with `fat32_format(0, 0, ...)` if I format FLOPPY style? No, HDD.
    // I will use `disk_format_fat32(0)` if I assume that function DID the partitioning.
    // Since `disk_format_fat32` is missing, I'll replace it with `fat32_format`.
    // I'll define `fat32_format` external.
    
    /* 2. Format Disk */
    serial("[INSTALLER] Formatting Setup...\n");
    // We will treat the whole disk as one FAT32 volume starting at sector 0 (Superfloppy) OR partition 1.
    // Let's go with Partition 1 at LBA 2048 for better compatibility.
    uint32_t start_lba = 2048;
    uint32_t total_sectors = disk_get_capacity();
    if (total_sectors == 0) total_sectors = 262144; // Default 128MB
    if (total_sectors <= start_lba + 4096) {
        serial("[INSTALLER] ERROR: Disk too small (%u sectors)\n", total_sectors);
        return;
    }

    serial("[INSTALLER] Creating FAT32 Filesystem on Partition 1 (LBA %d)...\n", start_lba);
    if (fat32_format(start_lba, total_sectors - start_lba, "CHRYSALIS") != 0) {
        serial("[INSTALLER] ERROR: Formatting failed!\n");
        return;
    }
    /* Validate VBR */
    uint8_t* vbr = (uint8_t*)kmalloc(512);
    if (vbr) {
        if (disk_read_sector(start_lba, vbr) != 0) {
            serial("[INSTALLER] ERROR: VBR read failed at LBA %d\n", start_lba);
            kfree(vbr);
            return;
        }
        uint16_t bps = *(uint16_t*)(vbr + 11);
        if (vbr[510] != 0x55 || vbr[511] != 0xAA || bps != 512) {
            serial("[INSTALLER] ERROR: VBR invalid (bps=%d sig=%x%x)\n", (int)bps, vbr[510], vbr[511]);
            serial("[INSTALLER] VBR dump: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   vbr[0], vbr[1], vbr[2], vbr[3], vbr[4], vbr[5], vbr[6], vbr[7],
                   vbr[8], vbr[9], vbr[10], vbr[11], vbr[12], vbr[13], vbr[14], vbr[15]);
            kfree(vbr);
            return;
        }
        kfree(vbr);
    }
    serial("[INSTALLER] Format Complete. Mounting FAT32...\n");
    
    if (fat32_init(0, start_lba) != 0) {
        serial("[INSTALLER] mount failed at LBA %d, trying LBA 0\n", start_lba);
        if (fat32_init(0, 0) != 0) {
            serial("[INSTALLER] ERROR: Failed to mount FAT32.\n");
            return;
        }
        fat32_set_mounted(0, 'a');
    } else {
        fat32_set_mounted(start_lba, 'a');
    }
    
    /* 3. Create Directory Structure */
    serial("[INSTALLER] Creating directories...\n");
    int mr = fat32_create_directory_verified("/boot", 1);
    if (mr != 0) { serial("[INSTALLER] ERROR: mkdir /boot failed (err=%d)\n", mr); return; }
    mr = fat32_create_directory_verified("/boot/chrysalis", 1);
    if (mr != 0) { serial("[INSTALLER] ERROR: mkdir /boot/chrysalis failed (err=%d)\n", mr); return; }
    mr = fat32_create_directory_verified("/boot/grub", 1);
    if (mr != 0) { serial("[INSTALLER] ERROR: mkdir /boot/grub failed (err=%d)\n", mr); return; }
    mr = fat32_create_directory_verified("/system", 1);
    if (mr != 0) { serial("[INSTALLER] WARN: mkdir /system failed (err=%d), continuing\n", mr); }

    fat_file_info_t root_entries[16];
    int root_count = fat32_read_directory("/", root_entries, 16);
    serial("[INSTALLER] / entries: %d\n", root_count);
    for (int i = 0; i < root_count; i++) {
        serial("  %s%s (%u)\n", root_entries[i].name, root_entries[i].is_dir ? "/" : "", root_entries[i].size);
    }

    fat_file_info_t boot_entries[16];
    int boot_count = fat32_read_directory("/boot", boot_entries, 16);
    serial("[INSTALLER] /boot entries: %d\n", boot_count);
    for (int i = 0; i < boot_count; i++) {
        serial("  %s%s (%u)\n", boot_entries[i].name, boot_entries[i].is_dir ? "/" : "", boot_entries[i].size);
    }
    
    /* 4. Locate Source Files (Multiboot Modules) */
    void* kernel_data = NULL;
    size_t kernel_size = 0;
    void* icons_data = NULL;
    size_t icons_size = 0;
    void* boot_img = NULL;
    size_t boot_img_size = 0;
    void* core_img = NULL;
    size_t core_img_size = 0;

    /* Scan multidoob tags (parsed manually here as we need raw addresses) */
    struct multiboot2_tag *tag = (struct multiboot2_tag*)(addr + 8);
    while (tag->type != MULTIBOOT2_TAG_TYPE_END) {
        if (tag->type == MULTIBOOT2_TAG_TYPE_MODULE) {
             struct multiboot2_tag_module *mod = (struct multiboot2_tag_module*)tag;
             const char* cmdline = mod->string; /* Correct field name: char string[] */
             
             serial("[INSTALLER] Found Module: %s at 0x%x\n", cmdline, mod->mod_start);
             
             if (cmdline && (kmalloc_strcmp(cmdline, "kernel.bin") == 0 || kmalloc_strstr(cmdline, "kernel.bin"))) {
                 kernel_data = (void*)mod->mod_start;
                 kernel_size = mod->mod_end - mod->mod_start;
             }
             else if (cmdline && (kmalloc_strcmp(cmdline, "icons.mod") == 0 || kmalloc_strstr(cmdline, "icons.mod"))) {
                 icons_data = (void*)mod->mod_start;
                 icons_size = mod->mod_end - mod->mod_start;
             }
             else if (cmdline && (kmalloc_strcmp(cmdline, "boot.img") == 0 || kmalloc_strstr(cmdline, "boot.img"))) {
                 boot_img = (void*)mod->mod_start;
                 boot_img_size = mod->mod_end - mod->mod_start;
             }
             else if (cmdline && (kmalloc_strcmp(cmdline, "core.img") == 0 || kmalloc_strstr(cmdline, "core.img"))) {
                 core_img = (void*)mod->mod_start;
                 core_img_size = mod->mod_end - mod->mod_start;
             }
        }
        tag = (struct multiboot2_tag*)((uint8_t*)tag + ((tag->size + 7) & ~7));
    }

    /* 4.1 Install GRUB boot code + core.img */
    if (!boot_img || boot_img_size < 446 || !core_img || core_img_size == 0) {
        serial("[INSTALLER] ERROR: GRUB boot/core images missing in installer ISO.\n");
        return;
    }

    uint32_t core_sectors = (core_img_size + 511) / 512;
    if (1 + core_sectors >= start_lba) {
        serial("[INSTALLER] ERROR: core.img too large (%u sectors)\n", core_sectors);
        return;
    }

    uint8_t* mbr = (uint8_t*)kmalloc(512);
    if (!mbr) { serial("[INSTALLER] ERROR: no memory for MBR\n"); return; }
    build_mbr(mbr, (const uint8_t*)boot_img, start_lba, total_sectors);
    disk_write_sector(0, mbr);
    kfree(mbr);

    serial("[INSTALLER] Writing GRUB core.img (%d bytes)...\n", (int)core_img_size);
    write_sectors(1, core_img, (uint32_t)core_img_size);
    
    /* 5. Write GRUB config early */
     const char* grub_cfg =
        "set timeout=3\n"
        "set default=0\n"
        "menuentry \"Chrysalis OS\" {\n"
        "  multiboot2 /boot/chrysalis/kernel.bin\n"
        "  boot\n"
        "}\n";
    serial("[INSTALLER] Writing GRUB configuration...\n");
    fat32_create_file_verified("/boot/grub/grub.cfg", grub_cfg, kmalloc_strlen(grub_cfg), 1);
    fat32_create_file_verified("/grub.cfg", grub_cfg, kmalloc_strlen(grub_cfg), 1);

    /* Install core.img into /boot/grub for normal GRUB stage2 lookup */
    serial("[INSTALLER] Copying core.img to /boot/grub/core.img...\n");
    fat32_create_file_verified("/boot/grub/core.img", core_img, (uint32_t)core_img_size, 1);

    /* 6. Install Kernel */
    if (kernel_data && kernel_size > 0) {
        serial("[INSTALLER] Installing Kernel (%d bytes)...\n", (int)kernel_size);
        int r = fat32_create_file_verified("/boot/chrysalis/kernel.bin", kernel_data, kernel_size, 1);
        if (r == 0) {
            serial("[INSTALLER] Kernel Installed OK.\n");
            int32_t ksz = fat32_get_file_size("/boot/chrysalis/kernel.bin");
            serial("[INSTALLER] kernel.bin size on disk: %d\n", ksz);
            if (ksz != (int32_t)kernel_size) {
                serial("[INSTALLER] ERROR: kernel.bin size mismatch\n");
                return;
            }
        } else {
            serial("[INSTALLER] ERROR: Failed to write kernel.bin (err=%d)\n", r);
            return;
        }
    } else {
        serial("[INSTALLER] ERROR: Kernel module not found in memory!\n");
        return;
    }

    /* 7. Install Icons */
    if (icons_data && icons_size > 0) {
        serial("[INSTALLER] Installing Icons (%d bytes)...\n", icons_size);
        /* This is large/slow, might take time */
        int r = fat32_create_file_verified("/system/icons.mod", icons_data, icons_size, 1);
        if (r == 0) {
            serial("[INSTALLER] Icons Installed OK.\n");
        } else {
            serial("[INSTALLER] ERROR: Failed to write icons.mod (err=%d)\n", r);
            return;
        }
    } else {
        serial("[INSTALLER] WARNING: Icons module not found!\n");
    }

    /* Debug: list /boot/grub */
    fat_file_info_t entries[16];
    int count = fat32_read_directory("/boot/grub", entries, 16);
    serial("[INSTALLER] /boot/grub entries: %d\n", count);
    for (int i = 0; i < count; i++) {
        serial("  %s%s (%u)\n", entries[i].name, entries[i].is_dir ? "/" : "", entries[i].size);
    }

    int32_t gsz = fat32_get_file_size("/boot/grub/grub.cfg");
    serial("[INSTALLER] grub.cfg size: %d\n", gsz);

    fat_file_info_t c_entries[16];
    int c_count = fat32_read_directory("/boot/chrysalis", c_entries, 16);
    serial("[INSTALLER] /boot/chrysalis entries: %d\n", c_count);
    for (int i = 0; i < c_count; i++) {
        serial("  %s%s (%u)\n", c_entries[i].name, c_entries[i].is_dir ? "/" : "", c_entries[i].size);
    }

    serial("\n[INSTALLER] Installation Complete. Rebooting in 5s...\n");
    // Busy wait
    for(volatile int i=0; i<50000000; i++);
    
    outb(0x64, 0xFE); // Reboot
    while(1) asm volatile("hlt");
}

/* Helper string functions since we don't have Full libc */
int kmalloc_strcmp(const char* s1, const char* s2) {
    while(*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

const char* kmalloc_strstr(const char* haystack, const char* needle) {
    // primitive strstr
    if (!*needle) return haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return haystack;
    }
    return NULL;
}

size_t kmalloc_strlen(const char* s) {
    size_t len = 0;
    while(s[len]) len++;
    return len;
}
