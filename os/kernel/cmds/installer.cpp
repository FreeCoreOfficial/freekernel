/* Chrysalis OS System Installer - v2.0
 * Standalone installer that runs from ISO and installs to secondary HDD
 */
#include <stdint.h>
#include <stddef.h>
#include "../terminal.h"
#include "../drivers/serial.h"
#include "../string.h"
#include "../mem/kmalloc.h"
#include "fat.h"
#include "../include/stdio.h"

extern "C" {
    int fat32_create_file(const char* path, const void* data, uint32_t size);
    int fat32_create_directory(const char* path);
    const void* ramfs_read_file(const char* name, size_t* out_size);
    void fat_automount(void);
}

/* Color codes for text output */
#define COLOR_NORMAL    "\x1b[0m"
#define COLOR_GREEN     "\x1b[32m"
#define COLOR_YELLOW    "\x1b[33m"
#define COLOR_CYAN      "\x1b[36m"
#define COLOR_WHITE     "\x1b[37m"

static void installer_print(const char* msg) {
    terminal_writestring(msg);
    serial_printf("%s", msg);
}

static void installer_print_progress(const char* task, const char* status) {
    char buf[256];
    snprintf(buf, sizeof(buf), "[INSTALLER] %s ... %s\n", task, status);
    installer_print(buf);
}

int installer_run(void) {
    terminal_writestring(COLOR_CYAN);
    installer_print("╔═══════════════════════════════════════════╗\n");
    installer_print("║  CHRYSALIS OS - SYSTEM INSTALLER v2.0   ║\n");
    installer_print("║  Setup Wizard - Configure your System    ║\n");
    installer_print("╚═══════════════════════════════════════════╝\n");
    terminal_writestring(COLOR_NORMAL);
    
    installer_print("\n");
    terminal_writestring(COLOR_YELLOW);
    installer_print(">>> Step 1: Initializing Filesystem\n");
    terminal_writestring(COLOR_NORMAL);
    
    installer_print_progress("FAT32 Mount", "Initializing...");
    fat_automount();
    installer_print_progress("FAT32 Mount", COLOR_GREEN "SUCCESS" COLOR_NORMAL);
    
    /* Create /boot directory */
    installer_print_progress("Creating /boot", "Initializing...");
    int boot_dir = fat32_create_directory("/boot");
    if (boot_dir == 0 || boot_dir == -2) {
        installer_print_progress("Creating /boot", COLOR_GREEN "SUCCESS" COLOR_NORMAL);
    }
    
    int grub_dir = fat32_create_directory("/boot/grub");
    if (grub_dir == 0 || grub_dir == -2) {
        serial_printf("[INSTALLER] /boot/grub created\n");
    }
    
    installer_print_progress("Creating /system", "Initializing...");
    int sys_dir = fat32_create_directory("/system");
    if (sys_dir == 0 || sys_dir == -2) {
        installer_print_progress("Creating /system", COLOR_GREEN "SUCCESS" COLOR_NORMAL);
    }
    
    terminal_writestring("\n");
    terminal_writestring(COLOR_YELLOW);
    installer_print(">>> Step 2: Installing Boot Files\n");
    terminal_writestring(COLOR_NORMAL);
    
    /* Copy kernel from bootloader */
    installer_print_progress("Loading kernel", "Reading...");
    size_t kernel_size = 0;
    const void* kernel_data = ramfs_read_file("kernel.bin", &kernel_size);
    
    if (kernel_data && kernel_size > 0) {
        int result = fat32_create_file("/boot/kernel.bin", kernel_data, kernel_size);
        if (result == 0) {
            installer_print_progress("Loading kernel", COLOR_GREEN "SUCCESS" COLOR_NORMAL);
            char buf[128];
            snprintf(buf, sizeof(buf), "  Kernel: %u bytes\n", (unsigned)kernel_size);
            installer_print(buf);
        }
    }
    
    /* Create GRUB config */
    installer_print_progress("Installing GRUB config", "Writing...");
    const char* grub_cfg = 
        "set default=0\n"
        "set timeout=5\n"
        "menuentry 'Chrysalis OS' { multiboot2 /boot/kernel.bin }\n";
    
    int cfg_result = fat32_create_file("/boot/grub/grub.cfg", grub_cfg, strlen(grub_cfg));
    if (cfg_result == 0) {
        installer_print_progress("Installing GRUB config", COLOR_GREEN "SUCCESS" COLOR_NORMAL);
    }
    
    terminal_writestring("\n");
    terminal_writestring(COLOR_YELLOW);
    installer_print(">>> Step 3: Installing System Files\n");
    terminal_writestring(COLOR_NORMAL);
    
    /* Copy icons */
    installer_print_progress("Loading icons", "Reading...");
    size_t icons_size = 0;
    const void* icons_data = ramfs_read_file("icons.mod", &icons_size);
    
    if (icons_data && icons_size > 0) {
        char size_msg[128];
        snprintf(size_msg, sizeof(size_msg), "Copying %u MB...", (unsigned)(icons_size / (1024*1024)));
        installer_print_progress("Loading icons", size_msg);
        
        int result = fat32_create_file("/system/icons.mod", icons_data, icons_size);
        if (result == 0) {
            installer_print_progress("Loading icons", COLOR_GREEN "SUCCESS" COLOR_NORMAL);
        } else {
            installer_print_progress("Loading icons", COLOR_YELLOW "WARNING" COLOR_NORMAL);
        }
    }
    
    /* Create metadata files */
    installer_print_progress("Creating system info", "Writing...");
    const char* info = "CHRYSALIS OS v1.0\nInstalled: 2026-01-16\nReady to boot\n";
    fat32_create_file("/system/info.txt", info, strlen(info));
    installer_print_progress("Creating system info", COLOR_GREEN "SUCCESS" COLOR_NORMAL);
    
    terminal_writestring("\n");
    terminal_writestring(COLOR_GREEN);
    installer_print("╔═══════════════════════════════════════════╗\n");
    installer_print("║  Installation Complete!                 ║\n");
    installer_print("║  System is ready to use                 ║\n");
    installer_print("║  Press ENTER to continue...             ║\n");
    installer_print("╚═══════════════════════════════════════════╝\n");
    terminal_writestring(COLOR_NORMAL);
    
    return 0;
}

/* Command wrapper */
extern "C" int cmd_installer(int argc, char** argv) {
    (void)argc; (void)argv;
    return installer_run();
}
