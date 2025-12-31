#include "command.h"

static inline void outb(unsigned short port, unsigned char val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void cmd_reboot(const char*) {
    outb(0x64, 0xFE); // CPU reset
    while (1);
}
