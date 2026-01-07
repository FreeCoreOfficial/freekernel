#include <stdint.h>

/* Multiboot 2 Header
 * MUST be in first 32KB of the kernel image
 * MUST be 8-byte aligned
 */
__attribute__((section(".multiboot"), aligned(8), used))
const uint32_t mb2_hdr[] = {
    0xE85250D6,        // MULTIBOOT2_HEADER_MAGIC
    0,                // architecture (i386)
    44,               // header length
    (uint32_t)-(0xE85250D6 + 0 + 44),

    // ---- Framebuffer tag ----
    5,                // type = framebuffer
    20,               // size
    1024,             // width
    768,              // height
    32,               // bpp

    // ---- End tag ----
    0,                // type = end
    8                 // size
};
