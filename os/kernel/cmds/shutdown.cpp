#include "command.h"

static inline void outb(unsigned short port, unsigned char val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(unsigned short port, unsigned short val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

void cmd_shutdown(const char*) {

    // QEMU (new)
    outw(0x604, 0x2000);

    // QEMU (old)
    outw(0xB004, 0x2000);

    // Bochs
    outw(0x4004, 0x3400);

    // VirtualBox
    outw(0x4004, 0x3400);

    // ACPI / APM fallback
    outb(0xB2, 0x20);

    // Dacă nimic nu merge → halt
    for (;;) {
        asm volatile("cli; hlt");
    }
}
