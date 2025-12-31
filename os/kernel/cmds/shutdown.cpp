#include "command.h"

static inline void outw(unsigned short port, unsigned short val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

void cmd_shutdown(const char*) {
    outw(0x604, 0x2000); // QEMU / Bochs poweroff
    while (1);
}
