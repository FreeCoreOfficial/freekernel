#include "ahci.h"

extern int ahci_port_init(int port_no, hba_port_t *port);
extern void* ahci_get_abar(void);

int ahci_init(void) {
    serial("[AHCI] init start\n");

    /* 1. PCI Probe & Map */
    if (ahci_pci_probe_and_map() != 0) {
        serial("[AHCI] PCI probe failed\n");
        return -1;
    }

    /* 2. Get ABAR */
    hba_mem_t *abar = (hba_mem_t *)ahci_get_abar();
    if (!abar) return -2;

    /* 3. Check Ports Implemented (PI) */
    uint32_t pi = abar->pi;
    serial("[AHCI] Ports Implemented (PI): 0x%08x\n", pi);

    int ports_found = 0;
    for (int i = 0; i < 32; i++) {
        if (pi & (1U << i)) {
            /* Check device signature */
            hba_port_t *port = (hba_port_t *)((uint8_t *)abar + 0x100 + (i * sizeof(hba_port_t)));
            uint32_t ssts = port->ssts;
            uint32_t sig = port->sig;
            
            /* Check if device present (Det=3) and Phy ready (Ipm=1) */
            uint8_t det = ssts & 0x0F;
            if (det != 3) continue;
            if (sig == 0xEB140101) continue; /* ATAPI/SATAPI skip for now if desired, or init */

            serial("[AHCI] Port %d present (SSTS=0x%x, SIG=0x%08x)\n", i, ssts, sig);
            if (ahci_port_init(i, port) == 0) {
                ports_found++;
            }
        }
    }
    return ports_found;
}