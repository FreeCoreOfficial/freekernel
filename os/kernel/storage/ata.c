#include "ata.h"
#include <stdint.h>
#include "../arch/i386/io.h"        // inb / outb
#include "../terminal.h"  // pentru debug printf

/* Primary bus */
#define ATA_PRIMARY_IO   0x1F0
#define ATA_PRIMARY_CTRL 0x3F6

void ata_init(void)
{
    terminal_writestring("[ATA] init\n");
}
