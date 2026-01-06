#include "ahci.h"
#include <stdint.h>
#include "../../paging.h"

static uint8_t controller_bus = 0, controller_dev = 0, controller_func = 0;
static hba_mem_t *abar = NULL;
static uint32_t abar_phys = 0;

int ahci_pci_probe_and_map(void) {
    serial("[AHCI] pci: probing for AHCI controller...\n");
    int ok = pci_find_device_by_class(0x01, 0x06, 0x01, &controller_bus, &controller_dev, &controller_func);
    if (!ok) {
        serial("[AHCI] pci: AHCI controller NOT found\n");
        return -1;
    }
    serial("[AHCI] pci: found at bus %u dev %u func %u\n", controller_bus, controller_dev, controller_func);

    /* BAR5 (index 5) contains ABAR for AHCI */
    uint32_t bar5 = pci_read_bar32(controller_bus, controller_dev, controller_func, 5);
    if ((bar5 & 0x1) == 0x1) {
        /* I/O BAR — not expected for AHCI; bail */
        serial("[AHCI] pci: BAR5 is IO, unsupported\n");
        return -2;
    }
    abar_phys = bar5 & ~0xF;
    if (!abar_phys) {
        serial("[AHCI] pci: ABAR phys == 0\n");
        return -3;
    }
    serial("[AHCI] pci: ABAR phys = 0x%08x\n", abar_phys);

    /* Map ABAR identity (Present | RW | PCD/Uncached recommended but standard RW works for now) */
    /* Using 0x3 (Present | RW) */
    paging_map_page(abar_phys, abar_phys, 0x3);
    
    /* Use identity mapping */
    abar = (hba_mem_t *)abar_phys;
    serial("[AHCI] pci: ABAR virt = %p\n", (void*)abar);

    /* enable bus mastering for DMA */
    pci_enable_busmaster(controller_bus, controller_dev, controller_func);
    serial("[AHCI] pci: busmaster enabled\n");

    return 0;
}

/* expose abar pointer to other modules */
hba_mem_t *ahci_get_abar(void) {
    return abar;
}
