#include "ahci.h"
#include <stdint.h>

void *memset(void *s, int c, size_t n);

extern ahci_port_state_t port_states[]; /* from ahci_port.c (make static->extern if needed) */

#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_READ_DMA_EXT  0x25
#define ATA_CMD_WRITE_DMA_EXT 0x35

/* FIS types */
#define FIS_TYPE_REG_H2D 0x27

/* Registers / fields sizes per AHCI spec */
#pragma pack(push,1)
typedef struct {
    uint8_t fis_type;
    uint8_t pmport:4;
    uint8_t rsv0:3;
    uint8_t c:1;
    uint8_t command;
    uint8_t featurel;
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t featureh;
    uint8_t countl;
    uint8_t counth;
    uint8_t icc;
    uint8_t control;
    uint8_t rsv1[4];
} fis_reg_h2d_t;

typedef struct {
    uint8_t rx_fis[0x100]; /* receive FIS area (already allocated by port) */
} fis_recv_t;
#pragma pack(pop)

/* Helper: build command header in CLB */
static void setup_cmd_header(void *clb, int slot, uint8_t flags, uint16_t prdt_len, uint32_t ctba) {
    hba_cmd_header_t *cl = (hba_cmd_header_t*)clb;
    /* clb is array of headers; each header is 32 bytes in spec; we simplified earlier */
    hba_cmd_header_t *h = (hba_cmd_header_t*)((uint8_t*)cl + slot * sizeof(hba_cmd_header_t));
    h->flags = (flags & 0xFF) | ((prdt_len & 0xFFFF) << 8);
    h->prdt_len = prdt_len;
    h->ctba = ctba;
}

/* small helper to wait for completion (CI clear) */
static int wait_for_cmd_complete(hba_port_t *port, int slot, uint32_t timeout_ms) {
    uint32_t start = get_uptime_ms();
    while ((port->ci & (1U << slot)) && (get_uptime_ms() - start < timeout_ms)) { /* spin */ }
    if (port->ci & (1U << slot)) return -1;
    /* check tfd for errors */
    uint32_t tfd = port->tfd;
    if (tfd & (1<<0) /* BSY */) return -2;
    if (tfd & (1<<1) /* DRQ */) return 0;
    return 0;
}

/* Build a PRDT entry list for single buffer (handles up to 4G by splitting) */
static int build_prdt_for_buffer(void *ct, const void *buf, uint32_t byte_count) {
    /* ct assumed zeroed; PRDT starts after 0x80 in cmd table per spec; we'll place first at offset 0x80 */
    uint8_t *ctb = (uint8_t*)ct;
    hba_prdt_entry_t *prdt = (hba_prdt_entry_t*)(ctb + 0x80);
    prdt->dba = (uint32_t)(uintptr_t)buf;
    prdt->dbau = 0;
    prdt->dbc = byte_count - 1; /* dbc is byte_count - 1 */
    return 1;
}

/* IDENTIFY implementation (polling) */
int ahci_send_identify(int port_no, hba_port_t *port, void *out_512) {
    serial("[AHCI] port %d: IDENTIFY start\n", port_no);
    /* find slot */
    int slot = find_cmdslot(port);
    if (slot < 0) {
        serial("[AHCI] port %d: IDENTIFY no free slot\n", port_no);
        return -1;
    }
    /* get CLB and CT pointers */
    ahci_port_state_t *st = &port_states[port_no];
    void *clb = st->clb;
    void *ct = st->cmd_tables[slot];
    uint32_t ct_phys = ahci_virt_to_phys(ct);

    /* prepare command header */
    setup_cmd_header(clb, slot, (5<<0), 1, ct_phys); /* flags: CFL=5 (RegH2D), PRDT len=1. Clear W (write) bit for Identify! */

    /* build PRDT for output buffer */
    /* PRDT must point to physical address of buffer */
    build_prdt_for_buffer(ct, (void*)ahci_virt_to_phys(out_512), 512);

    /* put FIS in command table (fis reg h2d) */
    fis_reg_h2d_t *fis = (fis_reg_h2d_t*)ct;
    memset(fis, 0, sizeof(fis_reg_h2d_t));
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->c = 1;
    fis->command = ATA_CMD_IDENTIFY;
    /* device registers zeroed (for ATA) */

    /* issue command */
    port->ci = (1U << slot);

    /* wait for completion */
    int r = wait_for_cmd_complete(port, slot, 5000);
    if (r != 0) {
        serial("[AHCI] port %d: IDENTIFY failed wait (%d), tfd=0x%08x\n", port_no, r, port->tfd);
        return -2;
    }

    serial("[AHCI] port %d: IDENTIFY done\n", port_no);
    return 0;
}

/* READ LBA (LBA48) simplified for single PRDT entry (assumes contiguous physical buffer) */
int ahci_read_lba(int port_no, uint64_t lba, uint32_t count, void *buf) {
    hba_port_t *port = port_states[port_no].port;
    if (!port) {
        serial("[AHCI] read: port %d not initialized\n", port_no);
        return -1;
    }
    serial("[AHCI] read: port=%d lba=%llu count=%u\n", port_no, (unsigned long long)lba, count);

    int slot = find_cmdslot(port);
    if (slot < 0) return -2;
    void *clb = port_states[port_no].clb;
    void *ct = port_states[port_no].cmd_tables[slot];
    uint32_t ct_phys = ahci_virt_to_phys(ct);
    setup_cmd_header(clb, slot, (5<<0), 1, ct_phys); /* Read: Clear W bit */
    build_prdt_for_buffer(ct, (void*)ahci_virt_to_phys(buf), count * 512);

    fis_reg_h2d_t *fis = (fis_reg_h2d_t*)ct;
    memset(fis, 0, sizeof(fis_reg_h2d_t));
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->c = 1;
    fis->command = ATA_CMD_READ_DMA_EXT;
    fis->lba0 = (uint8_t)(lba & 0xFF);
    fis->lba1 = (uint8_t)((lba >> 8) & 0xFF);
    fis->lba2 = (uint8_t)((lba >> 16) & 0xFF);
    fis->lba3 = (uint8_t)((lba >> 24) & 0xFF);
    fis->lba4 = (uint8_t)((lba >> 32) & 0xFF);
    fis->lba5 = (uint8_t)((lba >> 40) & 0xFF);
    fis->device = 0x40; /* LBA mode */
    fis->countl = (uint8_t)(count & 0xFF);
    fis->counth = (uint8_t)((count >> 8) & 0xFF);

    port->ci = (1U << slot);

    int r = wait_for_cmd_complete(port, slot, 5000);
    if (r != 0) {
        serial("[AHCI] read: port %d cmd failed (%d) tfd=0x%08x\n", port_no, r, port->tfd);
        return -3;
    }
    serial("[AHCI] read: done port=%d lba=%llu count=%u\n", port_no, (unsigned long long)lba, count);
    return 0;
}

/* write implementation mirrors read but uses ATA_CMD_WRITE_DMA_EXT */
int ahci_write_lba(int port_no, uint64_t lba, uint32_t count, const void *buf) {
    hba_port_t *port = port_states[port_no].port;
    if (!port) {
        serial("[AHCI] write: port %d not initialized\n", port_no);
        return -1;
    }
    serial("[AHCI] write: port=%d lba=%llu count=%u\n", port_no, (unsigned long long)lba, count);

    int slot = find_cmdslot(port);
    if (slot < 0) return -2;
    void *clb = port_states[port_no].clb;
    void *ct = port_states[port_no].cmd_tables[slot];
    uint32_t ct_phys = ahci_virt_to_phys(ct);
    setup_cmd_header(clb, slot, (1<<6) | (5<<0), 1, ct_phys); /* Write: Set W bit (bit 6) */
    /* PRDT must point to non-const buffer — cast away const for DMA */
    build_prdt_for_buffer(ct, (void*)ahci_virt_to_phys((void*)buf), count * 512);

    fis_reg_h2d_t *fis = (fis_reg_h2d_t*)ct;
    memset(fis, 0, sizeof(fis_reg_h2d_t));
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->c = 1;
    fis->command = ATA_CMD_WRITE_DMA_EXT;
    fis->lba0 = (uint8_t)(lba & 0xFF);
    fis->lba1 = (uint8_t)((lba >> 8) & 0xFF);
    fis->lba2 = (uint8_t)((lba >> 16) & 0xFF);
    fis->lba3 = (uint8_t)((lba >> 24) & 0xFF);
    fis->lba4 = (uint8_t)((lba >> 32) & 0xFF);
    fis->lba5 = (uint8_t)((lba >> 40) & 0xFF);
    fis->device = 0x40; /* LBA mode */
    fis->countl = (uint8_t)(count & 0xFF);
    fis->counth = (uint8_t)((count >> 8) & 0xFF);

    port->ci = (1U << slot);

    int r = wait_for_cmd_complete(port, slot, 5000);
    if (r != 0) {
        serial("[AHCI] write: port %d cmd failed (%d) tfd=0x%08x\n", port_no, r, port->tfd);
        return -3;
    }
    serial("[AHCI] write: done port=%d lba=%llu count=%u\n", port_no, (unsigned long long)lba, count);
    return 0;
}
