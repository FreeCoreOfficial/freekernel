/* Chrysalis OS Installer - Main Entry Point
 * Purpose: Initialize and start the installer
 */

#include <stdint.h>
#include <stddef.h>

/* Forward declarations from kernel/ */
extern "C" {
    void terminal_clear(void);
    void terminal_writestring(const char* data);
    void terminal_setcolor(uint8_t color);
    int sprintf(char* str, const char* format, ...);
    void installer_main(uint32_t magic, uint32_t addr);
    void serial_init(void);
    void serial(const char* fmt, ...);
}

extern "C" void kernel_main(uint32_t magic, uint32_t addr) {
    serial_init();
    serial("[INSTALLER] kernel_main: magic=0x%x addr=0x%x\n", magic, addr);
    installer_main(magic, addr);
}
