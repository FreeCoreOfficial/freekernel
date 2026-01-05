// kernel/hardware/acpi.cpp
#include "acpi.h"
#include "../terminal.h"
#include "../mm/vmm.h"
#include "../drivers/serial.h" // Necesar pentru serial_write_string
#include <stddef.h>

/* ============================================================
   ACPI Data Structures (Packed)
   ============================================================ */

struct RSDPDescriptor {
    char     signature[8];   // "RSD PTR "
    uint8_t  checksum;
    char     oemid[6];
    uint8_t  revision;       // 0 = ACPI 1.0, 2 = ACPI 2.0+
    uint32_t rsdt_address;   // Physical address of RSDT
} __attribute__((packed));

struct RSDPDescriptor20 {
    RSDPDescriptor firstPart;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t  extended_checksum;
    uint8_t  reserved[3];
} __attribute__((packed));

struct ACPISDTHeader {
    char     signature[4];   // e.g. "APIC", "FACP"
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
   Internal Helpers
   ============================================================ */

/* Helper local pentru a scrie hex pe serial (fără printf complex) */
static void serial_print_hex(uint32_t n) {
    const char* digits = "0123456789ABCDEF";
    serial_write_string("0x");
    for (int i = 28; i >= 0; i -= 4) {
        char c[2];
        c[0] = digits[(n >> i) & 0xF];
        c[1] = 0;
        serial_write_string(c);
    }
}

/* Loghează mesajul atât pe ecran cât și pe serial */
static void acpi_log(const char* msg) {
    terminal_writestring("[ACPI] ");
    terminal_writestring(msg);
    terminal_writestring("\n");
    serial_write_string("[ACPI] ");
    serial_write_string(msg);
    serial_write_string("\r\n");
}

/* 
 * Helper pentru a mapa o regiune fizică în spațiul virtual (identity map).
 * Gestionează alinierea la pagină (4KB) automat.
 */
static void acpi_map_phys(uint32_t phys_addr, uint32_t size)
{
    if (phys_addr == 0 || size == 0) return;

    serial_write_string("[ACPI] Mapping phys: ");
    serial_print_hex(phys_addr);
    serial_write_string(" size: ");
    serial_print_hex(size);
    serial_write_string("\r\n");

    // Rotunjim în jos la începutul paginii (4KB)
    uint32_t base = phys_addr & 0xFFFFF000;
    // Calculăm sfârșitul
    uint32_t end = phys_addr + size;
    
    // Calculăm dimensiunea aliniată (trebuie să acopere tot intervalul)
    // (end + 0xFFF) & 0xFFFFF000 este sfârșitul rotunjit la următoarea pagină
    uint32_t aligned_end = (end + 0xFFF) & 0xFFFFF000;
    uint32_t aligned_size = aligned_end - base;

    vmm_identity_map(base, aligned_size);
    
    serial_write_string("[ACPI] Map call finished.\r\n");
}

/* Verifică suma de control a unui tabel ACPI */
static bool acpi_checksum(void* ptr, uint32_t length)
{
    uint8_t sum = 0;
    uint8_t* p = (uint8_t*)ptr;
    for (uint32_t i = 0; i < length; i++) {
        sum += p[i];
    }
    return (sum == 0);
}

/* ============================================================
   RSDP Discovery
   ============================================================ */

static RSDPDescriptor* acpi_scan_memory(uint32_t start, uint32_t length)
{
    serial_write_string("[ACPI] Scanning memory from ");
    serial_print_hex(start);
    serial_write_string(" len ");
    serial_print_hex(length);
    serial_write_string("\r\n");

    // Mapează zona pe care urmează să o scanăm
    acpi_map_phys(start, length);

    // Caută semnătura "RSD PTR " la fiecare 16 bytes
    for (uint32_t addr = start; addr < start + length; addr += 16) {
        RSDPDescriptor* rsdp = (RSDPDescriptor*)addr;
        
        // Verificare rapidă semnătură
        if (rsdp->signature[0] == 'R' && 
            rsdp->signature[1] == 'S' && 
            rsdp->signature[2] == 'D' && 
            rsdp->signature[3] == ' ' &&
            rsdp->signature[4] == 'P' &&
            rsdp->signature[5] == 'T' &&
            rsdp->signature[6] == 'R' &&
            rsdp->signature[7] == ' ') 
        {
            // Verificare checksum
            if (acpi_checksum(rsdp, sizeof(RSDPDescriptor))) {
                return rsdp;
            }
        }
    }
    return nullptr;
}

static RSDPDescriptor* acpi_find_rsdp(void)
{
    // 1. Caută în EBDA (Extended BIOS Data Area)
    acpi_log("Checking EBDA...");
    // Pointerul către EBDA se află la adresa fizică 0x40E
    
    // Mapează pagina 0 pentru a citi BDA
    acpi_map_phys(0, 0x1000);
    
    uint16_t ebda_seg = *(uint16_t*)0x40E;
    uint32_t ebda_phys = (uint32_t)ebda_seg << 4;

    if (ebda_phys != 0 && ebda_phys < 0xA0000) {
        acpi_log("EBDA pointer valid.");
        // EBDA are de obicei 1KB
        RSDPDescriptor* rsdp = acpi_scan_memory(ebda_phys, 1024);
        if (rsdp) return rsdp;
    }

    // 2. Caută în zona principală BIOS (0xE0000 - 0xFFFFF)
    acpi_log("Checking BIOS area (0xE0000)...");
    return acpi_scan_memory(0xE0000, 0x20000);
}

/* ============================================================
   Initialization
   ============================================================ */

void acpi_init(void)
{
    acpi_log("Initializing...");

    RSDPDescriptor* rsdp = acpi_find_rsdp();
    if (!rsdp) {
        acpi_log("Error: RSDP not found.");
        return;
    }

    terminal_printf("[acpi] RSDP found at 0x%x, Revision: %d\n", (uint32_t)rsdp, rsdp->revision);
    serial_write_string("[ACPI] RSDP found.\r\n");

    // Verificăm adresa RSDT
    uint32_t rsdt_phys = rsdp->rsdt_address;
    serial_write_string("[ACPI] RSDT Phys Addr: "); serial_print_hex(rsdt_phys); serial_write_string("\r\n");
    if (rsdt_phys == 0) {
        acpi_log("Error: RSDT address is NULL.");
        return;
    }

    // FALLBACK CRITIC: Verificăm dacă RSDT este în zona mapată sigur (120MB)
    // Adresa 0x07800000 = 120 MB. Dacă RSDT e mai sus, VMM-ul actual poate crăpa.
    if (rsdt_phys >= 0x07800000) {
        acpi_log("WARNING: RSDT address > 120MB (outside initial paging).");
        acpi_log("Skipping ACPI init to prevent VMM crash/Kernel Panic.");
        return;
    }

    // 1. Mapăm doar header-ul RSDT pentru a citi lungimea
    acpi_map_phys(rsdt_phys, sizeof(ACPISDTHeader));
    ACPISDTHeader* rsdt = (ACPISDTHeader*)rsdt_phys;

    // Validare semnătură RSDT
    if (rsdt->signature[0] != 'R' || rsdt->signature[1] != 'S' || 
        rsdt->signature[2] != 'D' || rsdt->signature[3] != 'T') {
        acpi_log("Error: Invalid RSDT signature.");
        return;
    }

    // Validare lungime (sanity check: max 4MB)
    if (rsdt->length > 0x400000) {
        terminal_printf("[acpi] Error: RSDT length too big (%d)\n", rsdt->length);
        serial_write_string("[ACPI] Error: RSDT length too big.\r\n");
        return;
    }

    // 2. Mapăm tot tabelul RSDT acum că știm lungimea corectă
    acpi_map_phys(rsdt_phys, rsdt->length);

    if (!acpi_checksum(rsdt, rsdt->length)) {
        acpi_log("Error: RSDT checksum failed.");
        return;
    }

    terminal_printf("[acpi] RSDT at 0x%x mapped, Length: %d\n", rsdt_phys, rsdt->length);
    acpi_log("RSDT mapped and validated.");

    // Iterăm prin intrările RSDT
    // Header-ul are 36 bytes. Intrările sunt pointeri de 32-bit (4 bytes).
    int entries = (rsdt->length - sizeof(ACPISDTHeader)) / 4;
    uint32_t* table_ptrs = (uint32_t*)((uint8_t*)rsdt + sizeof(ACPISDTHeader));

    for (int i = 0; i < entries; i++) {
        uint32_t table_phys = table_ptrs[i];
        if (table_phys == 0) continue;

        // Mapăm header-ul tabelului pentru a-i vedea semnătura
        acpi_map_phys(table_phys, sizeof(ACPISDTHeader));
        ACPISDTHeader* header = (ACPISDTHeader*)table_phys;

        // Afișăm semnătura (ex: FACP, APIC)
        char sig[5];
        sig[0] = header->signature[0];
        sig[1] = header->signature[1];
        sig[2] = header->signature[2];
        sig[3] = header->signature[3];
        sig[4] = '\0';

        terminal_printf("[acpi] Found Table: %s at 0x%x\n", sig, table_phys);
        serial_write_string("[ACPI] Found Table: "); serial_write_string(sig); 
        serial_write_string(" at "); serial_print_hex(table_phys); serial_write_string("\r\n");
        
        // Aici poți adăuga logica pentru a parsa tabele specifice (ex: FACP pentru shutdown)
        // if (memcmp(sig, "FACP", 4) == 0) parse_fadt(header);
    }

    acpi_log("Initialization complete.");
}
