// kernel/detect/tpm.c
#include "tpm.h"

#include "../terminal.h"
#include "../panic.h"
#include <stdint.h>

#define TPM_BASE_PHYS 0xFED40000u
#define TPM_DID_VID   0xF00u  /* common TIS DID_VID offset (legacy) */

/* simple MMIO read helpers */
static inline uint32_t mmio_read32(uintptr_t addr)
{
    volatile uint32_t *p = (volatile uint32_t *)addr;
    return *p;
}

static inline uint8_t mmio_read8(uintptr_t addr)
{
    volatile uint8_t *p = (volatile uint8_t *)addr;
    return *p;
}

/* Detect presence of TPM 1.2 (TIS-ish).
 * Heuristic: read 32-bit DID_VID register at base+0xF00.
 * If value is 0 or 0xFFFFFFFF -> no mapping. Otherwise treat as TPM1.2 present.
 */
int tpm_detect_v12(void)
{
    uintptr_t reg = (uintptr_t)TPM_BASE_PHYS + TPM_DID_VID;
    uint32_t raw = mmio_read32(reg);

    if (raw == 0xFFFFFFFFu || raw == 0x00000000u)
        return 0;

    /* basic sanity: treat non-empty DID_VID as TIS presence */
    return 1;
}

/* Detect presence of TPM 2.0 (CRB-ish / VirtualBox).
 * Heuristic: try reading from TPM base (offset 0). If it's mapped (non-0/0xFF..)
 * then assume some TPM (likely TPM2/CRB) is present. We don't deeply parse CRB here.
 */
int tpm_detect_v2(void)
{
    uintptr_t reg = (uintptr_t)TPM_BASE_PHYS;
    uint32_t raw = mmio_read32(reg);

    if (raw == 0xFFFFFFFFu || raw == 0x00000000u)
        return 0;

    /* Additional heuristic: many TPM2 implementations expose a version byte or
       a register that isn't all zeros/ones at base. If base is non-empty,
       we treat it as TPM2-capable (VirtualBox uses CRB mapping). */
    return 1;
}

/* Return 12 (v1.2) or 20 (v2) or 0 (none) */
int tpm_detect_any(void)
{
    if (tpm_detect_v12()) return 12;
    if (tpm_detect_v2())  return 20;
    return 0;
}

/* Panic if no acceptable TPM; otherwise print what was found */
void tpm_check_or_panic(void)
{
    terminal_writestring("[tpm] probing TPM...\n");

    int ver = tpm_detect_any();

    if (ver == 12) {
        terminal_writestring("[tpm] TPM 1.2 (TIS) detected -> OK\n");
        return;
    }

    if (ver == 20) {
        terminal_writestring("[tpm] TPM 2.0/CRB detected (compat mode) -> OK\n");
        terminal_writestring("[tpm] Note: full TPM2 support not implemented; running in compatibility mode.\n");
        return;
    }

    /* none found -> panic (hard requirement) */
    panic(
        "TPM REQUIRED\n"
        "------------\n"
        "No TPM detected. Chrysalis requires a TPM device to boot.\n\n"
        "If you're running in a VM:\n"
        " - For QEMU: use swtpm + -device tpm-tis (or tpm-crb as needed)\n"
        " - For VirtualBox: enable the TPM module in the VM settings (may expose TPM2/CRB)\n\n"
        "If you believe this is incorrect, verify your VM/hardware exposes TPM MMIO at 0xFED40000\n"
    );
}
