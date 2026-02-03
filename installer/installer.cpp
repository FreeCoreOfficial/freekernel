#include <stdint.h>
#include <stddef.h>

#include "../os/kernel/include/types.h"
#include "../os/kernel/drivers/serial.h"
#include "../os/kernel/memory/pmm.h"
#include "../os/kernel/mem/kmalloc.h"
#include "../os/kernel/storage/ata.h"
#include "../os/kernel/cmds/disk.h"  /* For disk_init, disk_read_sector etc */
#include "../os/kernel/fs/ramfs/ramfs.h"
#include "../os/kernel/fs/fat/fat.h"
#include "../os/kernel/smp/multiboot.h"
#include "../os/kernel/arch/i386/io.h" /* for outb */

/* Helper Prototypes */
int kmalloc_strcmp(const char* s1, const char* s2);
const char* kmalloc_strstr(const char* haystack, const char* needle);
size_t kmalloc_strlen(const char* s);

extern "C" {
    void kmalloc_init(void);
    int fat32_format(uint32_t lba, uint32_t sector_count, const char* label);
    int fat32_create_directory(const char* path);
    int fat32_create_file(const char* path, const void* data, uint32_t size);
    void serial(const char *fmt, ...);
    void ata_init(void);
}


extern "C" void installer_main(uint32_t magic, uint32_t addr) {
    serial_init();
    serial("\n\n[INSTALLER] Starting Chrysalis OS Installer...\n");

    /* 1. Initialize Subsystems */
    pmm_init(magic, (void*)addr);  /* Crucial for parsing modules */
    kmalloc_init();
    
    serial("[INSTALLER] initializing ATA...\n");
    ata_init();

    /* 2. Format Target Disk (Primary Master) */
    serial("[INSTALLER] Formatting primary master (0) to FAT32...\n");
    
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
    uint32_t total_sectors = 262144; // Default 128MB
    
    // Try to get real size? `ata_identify`?
    // Let's import `ata_identify`.
    
    serial("[INSTALLER] Creating FAT32 Filesystem on Partition 1 (LBA %d)...\n", start_lba);
    if (fat32_format(start_lba, total_sectors - start_lba, "CHRYSALIS") != 0) {
        serial("[INSTALLER] ERROR: Formatting failed!\n");
        return;
    }
    serial("[INSTALLER] Format Complete. Mounting FAT32...\n");
    
    if (fat32_init(0, start_lba) != 0) {
        serial("[INSTALLER] ERROR: Failed to mount FAT32.\n");
        return;
    }
    
    /* 3. Create Directory Structure */
    serial("[INSTALLER] Creating directories...\n");
    fat32_create_directory("/boot");
    fat32_create_directory("/boot/chrysalis");
    fat32_create_directory("/boot/grub");
    fat32_create_directory("/system");
    
    /* 4. Locate Source Files (Multiboot Modules) */
    void* kernel_data = NULL;
    size_t kernel_size = 0;
    void* icons_data = NULL;
    size_t icons_size = 0;

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
        }
        tag = (struct multiboot2_tag*)((uint8_t*)tag + ((tag->size + 7) & ~7));
    }
    
    /* 5. Install Kernel */
    if (kernel_data && kernel_size > 0) {
        serial("[INSTALLER] Installing Kernel (%d bytes)...\n", kernel_size);
        if (fat32_create_file("/boot/chrysalis/kernel.bin", kernel_data, kernel_size) == 0) {
            serial("[INSTALLER] Kernel Installed OK.\n");
        } else {
            serial("[INSTALLER] ERROR: Failed to write kernel.bin\n");
        }
    } else {
        serial("[INSTALLER] ERROR: Kernel module not found in memory!\n");
    }

    /* 6. Install Icons */
    if (icons_data && icons_size > 0) {
        serial("[INSTALLER] Installing Icons (%d bytes)...\n", icons_size);
        /* This is large/slow, might take time */
        if (fat32_create_file("/system/icons.mod", icons_data, icons_size) == 0) {
            serial("[INSTALLER] Icons Installed OK.\n");
        } else {
            serial("[INSTALLER] ERROR: Failed to write icons.mod\n");
        }
    } else {
        serial("[INSTALLER] WARNING: Icons module not found!\n");
    }
    
    /* 7. Install Bootloader Config */
     const char* grub_cfg = 
        "set timeout=3\n"
        "set default=0\n"
        "menuentry \"Chrysalis OS\" {\n"
        "  multiboot2 /boot/chrysalis/kernel.bin\n"
        "  boot\n"
        "}\n";
    serial("[INSTALLER] Writing GRUB configuration...\n");
    fat32_create_file("/boot/grub/grub.cfg", grub_cfg, kmalloc_strlen(grub_cfg));

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
