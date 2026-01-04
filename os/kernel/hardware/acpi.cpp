// kernel/hardware/acpi.cpp
#include "acpi.h"
#include "../terminal.h"
#include <stdint.h>

/* ============================================================
   ACPI structs (packed, conform spec)
   ============================================================ */

struct RSDPDescriptor {
    char     signature[8];   // "RSD PTR "
    uint8_t  checksum;
    char     oemid[6];
    uint8_t  revision;
    uint32_t rsdt_address;
} __attribute__((packed));

struct ACPISDTHeader {
    char     signature[4];
    uint32_t length;
    uint8_t  revision;
    uint8_t  checksum;
    char     oemid[6];
    char     oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed));

/* ============================================================
   Helpers
   ============================================================ */

static uint8_t acpi_checksum(void* ptr, uint32_t length)
{
    uint8_t sum = 0;
    uint8_t* p = (uint8_t*)ptr;

    for (uint32_t i = 0; i < length; i++)
        sum += p[i];

    return sum;
}

/* ============================================================
   RSDP scan (EBDA + BIOS area)
   ============================================================ */

static RSDPDescriptor* acpi_find_rsdp(void)
{
    /* 1) EBDA */
    uint16_t* ebda_seg = (uint16_t*)0x40E;
    uint32_t ebda_addr = ((uint32_t)(*ebda_seg)) << 4;

    for (uint32_t addr = ebda_addr; addr < ebda_addr + 1024; addr += 16) {
        RSDPDescriptor* rsdp = (RSDPDescriptor*)addr;
        if (__builtin_memcmp(rsdp->signature, "RSD PTR ", 8) == 0)
            return rsdp;
    }

    /* 2) BIOS area */
    for (uint32_t addr = 0xE0000; addr < 0x100000; addr += 16) {
        RSDPDescriptor* rsdp = (RSDPDescriptor*)addr;
        if (__builtin_memcmp(rsdp->signature, "RSD PTR ", 8) == 0)
            return rsdp;
    }

    return nullptr;
}

/* ============================================================
   ACPI init (STAGE 1 — READ ONLY, SAFE)
   ============================================================ */

void acpi_init(void)
{
    terminal_writestring("[acpi] searching for RSDP...\n");

    RSDPDescriptor* rsdp = acpi_find_rsdp();
    if (!rsdp) {
        terminal_writestring("[acpi] RSDP not found\n");
        return;
    }

    if (acpi_checksum(rsdp, sizeof(RSDPDescriptor)) != 0) {
        terminal_writestring("[acpi] RSDP checksum invalid\n");
        return;
    }

    terminal_writestring("[acpi] RSDP found at ");
    terminal_writehex((uint32_t)rsdp);
    terminal_writestring("\n");

    /* ---- RSDT guard ---- */
    if (!rsdp->rsdt_address) {
        terminal_writestring("[acpi] RSDT address is null\n");
        return;
    }

    ACPISDTHeader* rsdt =
        (ACPISDTHeader*)(uintptr_t)rsdp->rsdt_address;

    if (__builtin_memcmp(rsdt->signature, "RSDT", 4) != 0) {
        terminal_writestring("[acpi] invalid RSDT signature\n");
        return;
    }

    if (acpi_checksum(rsdt, rsdt->length) != 0) {
        terminal_writestring("[acpi] RSDT checksum invalid\n");
        return;
    }

    terminal_writestring("[acpi] RSDT loaded\n");

    uint32_t entries =
        (rsdt->length - sizeof(ACPISDTHeader)) / 4;

    uint32_t* table_ptrs =
        (uint32_t*)((uint8_t*)rsdt + sizeof(ACPISDTHeader));

    for (uint32_t i = 0; i < entries; i++) {
        if (!table_ptrs[i])
            continue;

        ACPISDTHeader* hdr =
            (ACPISDTHeader*)(uintptr_t)table_ptrs[i];

        terminal_writestring("[acpi] table: ");
        for (int j = 0; j < 4; j++)
            terminal_putchar(hdr->signature[j]);
        terminal_writestring("\n");
    }

    terminal_writestring("[acpi] init complete (stage 1)\n");
}


/* ACPISDTHeader* rsdt = (ACPISDTHeader*)(uintptr_t)rsdp->rsdt_address; */