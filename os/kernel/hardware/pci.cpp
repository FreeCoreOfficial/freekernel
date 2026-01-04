// kernel/hardware/pci.cpp
#include "pci.h"
#include "../terminal.h"
#include "../arch/i386/io.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static uint32_t pci_read(uint8_t bus, uint8_t device,
                         uint8_t function, uint8_t offset)
{
    uint32_t address =
        (1U << 31) |
        ((uint32_t)bus << 16) |
        ((uint32_t)device << 11) |
        ((uint32_t)function << 8) |
        (offset & 0xFC);

    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

static uint16_t pci_get_vendor(uint8_t bus, uint8_t device, uint8_t function)
{
    return pci_read(bus, device, function, 0x00) & 0xFFFF;
}

static uint16_t pci_get_device(uint8_t bus, uint8_t device, uint8_t function)
{
    return (pci_read(bus, device, function, 0x00) >> 16) & 0xFFFF;
}

void pci_init(void)
{
    terminal_writestring("[pci] scanning PCI bus...\n");

    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            for (uint8_t function = 0; function < 8; function++) {

                uint16_t vendor = pci_get_vendor(bus, device, function);
                if (vendor == 0xFFFF)
                    continue;

                uint16_t dev = pci_get_device(bus, device, function);

                terminal_writestring("[pci] device found: ");
                terminal_writehex(vendor);
                terminal_writestring(":");
                terminal_writehex(dev);
                terminal_writestring("\n");
            }
        }
    }

    terminal_writestring("[pci] scan complete\n");
}
