#include "framebuffer.h"
#include "../mm/vmm.h"
#include "../drivers/serial.h"
#include "../memory/pmm.h"
#include "../mm/paging.h" /* for KERNEL_BASE */

/* Import serial logging from kernel glue */
extern void serial(const char *fmt, ...);

/* Internal State */
static uint8_t* fb_buffer = 0; /* Virtual address pointer */
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint32_t fb_pitch = 0;
static uint8_t  fb_bpp = 0;

/* Virtual base for Framebuffer (Must not conflict with ACPI_TEMP_VIRT_BASE at 0xE1000000) */
#define FB_VIRT_BASE 0xE0000000

void fb_init(uint64_t addr, uint32_t width, uint32_t height, uint32_t pitch, uint8_t bpp, uint8_t type) {
    serial("[FB] Initializing VESA Framebuffer...\n");

    serial_printf("[FB] addr  = 0x%x\n", (uint32_t)addr);
    serial_printf("[FB] type  = %u\n", type);
    serial_printf("[FB] w/h   = %ux%u\n",
                  width,
                  height);
    serial_printf("[FB] bpp   = %u\n", bpp);

    /* Validate Framebuffer Type (1 = RGB) */
    if (type != 1) {
        serial("[FB] Error: Framebuffer type is not RGB (type=%d).\n", type);
        if (type == 2) {
            serial("[FB] Type 2 = Text Mode. GRUB failed to switch video mode.\n");
            serial("[FB] Fix: set gfxpayload=1024x768x32 in grub.cfg (do not use 'keep').\n");
        }
        return;
    }

    /* Store Info */
    uint64_t phys_addr = addr;
    fb_width = width;
    fb_height = height;
    fb_pitch = pitch;
    fb_bpp = bpp;

    if (phys_addr == 0 || fb_width == 0 || fb_height == 0) return;

    serial("[FB] Found LFB at Phys: 0x%x\n", (uint32_t)phys_addr);
    serial("[FB] Resolution: %dx%d, BPP: %d, Pitch: %d\n", fb_width, fb_height, fb_bpp, fb_pitch);

    /* 4. Reserve Framebuffer Memory in PMM (Critical!) */
    /* Calculate total size in bytes */
    uint32_t fb_size = fb_height * fb_pitch;
    
    pmm_reserve_area((uint32_t)phys_addr, fb_size);

    /* Get active page directory from VMM */
    uint32_t* active_pd = vmm_get_current_pd();

    /* 5. Map Framebuffer Memory */
    /* Map physical framebuffer to virtual memory at FB_VIRT_BASE.
     * We use PAGE_PCD | PAGE_PWT to disable caching for video memory (avoids artifacts/crashes).
     */
    vmm_map_region(active_pd, 
                   FB_VIRT_BASE, 
                   (uint32_t)phys_addr, 
                   fb_size, 
                   PAGE_PRESENT | PAGE_RW | PAGE_PCD | PAGE_PWT);

    /* Force TLB flush to ensure mappings are visible immediately */
    asm volatile("mov %%cr3, %%eax; mov %%eax, %%cr3" ::: "eax", "memory");
    
    /* Store virtual pointer */
    fb_buffer = (uint8_t*)FB_VIRT_BASE;
    
    serial("[FB] Mapped %d bytes at virtual 0x%x\n", fb_size, fb_buffer);
    serial("[FB] Initialization successful.\n");

    /* Clear screen to black to verify write access */
    fb_clear(0x000000);
}

void fb_putpixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb_buffer || x >= fb_width || y >= fb_height) return;

    /* Calculate offset in bytes */
    uint32_t offset = y * fb_pitch + x * (fb_bpp / 8);
    uint8_t* pixel = fb_buffer + offset;

    if (fb_bpp == 32) {
        /* 32-bit (ARGB/RGBA) */
        *(uint32_t*)pixel = color;
    } else if (fb_bpp == 24) {
        /* 24-bit (RGB) - Write 3 bytes */
        pixel[0] = (color) & 0xFF;       // Blue
        pixel[1] = (color >> 8) & 0xFF;  // Green
        pixel[2] = (color >> 16) & 0xFF; // Red
    }
}

void fb_clear(uint32_t color) {
    if (!fb_buffer) return;

    /* Optimized clear could use memset/rep stosd for 32bpp, 
     * but generic loop covers all BPPs safely for now. */
    for (uint32_t y = 0; y < fb_height; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            fb_putpixel(x, y, color);
        }
    }
}

void fb_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (!fb_buffer) return;

    /* Clipping */
    if (x >= fb_width || y >= fb_height) return;
    if (x + w > fb_width) w = fb_width - x;
    if (y + h > fb_height) h = fb_height - y;

    for (uint32_t j = 0; j < h; j++) {
        for (uint32_t i = 0; i < w; i++) {
            fb_putpixel(x + i, y + j, color);
        }
    }
}

void fb_get_info(uint32_t* width, uint32_t* height, uint32_t* pitch, uint8_t* bpp, uint8_t** buffer) {
    if (width) *width = fb_width;
    if (height) *height = fb_height;
    if (pitch) *pitch = fb_pitch;
    if (bpp) *bpp = fb_bpp;
    if (buffer) *buffer = fb_buffer;
}
