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
    void installer_main(void);
}

extern "C" void kernel_main(void) {
    /* Call the real installer implementation */
    installer_main();
}
