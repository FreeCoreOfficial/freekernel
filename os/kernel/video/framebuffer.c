#include "framebuffer.h"
#include "../mm/vmm.h"
#include "../drivers/serial.h"
#include "../memory/pmm.h"
#include "../mm/paging.h" /* for KERNEL_BASE */
#include "gpu.h"

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

/* --- GPU Driver Implementation --- */

static void vesa_putpixel(gpu_device_t* dev, uint32_t x, uint32_t y, uint32_t color) {
    if (!dev->virt_addr || x >= dev->width || y >= dev->height) return;

    uint32_t offset = y * dev->pitch + x * (dev->bpp / 8);
    uint8_t* pixel = (uint8_t*)dev->virt_addr + offset;

    if (dev->bpp == 32) {
        *(uint32_t*)pixel = color;
    } else if (dev->bpp == 24) {
        pixel[0] = (color) & 0xFF;
        pixel[1] = (color >> 8) & 0xFF;
        pixel[2] = (color >> 16) & 0xFF;
    }
}

static void vesa_fillrect(gpu_device_t* dev, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (!dev->virt_addr) return;
    
    /* Clipping */
    if (x >= dev->width || y >= dev->height) return;
    if (x + w > dev->width) w = dev->width - x;
    if (y + h > dev->height) h = dev->height - y;

    for (uint32_t j = 0; j < h; j++) {
        for (uint32_t i = 0; i < w; i++) {
            vesa_putpixel(dev, x + i, y + j, color);
        }
    }
}

static void vesa_clear(gpu_device_t* dev, uint32_t color) {
    if (!dev->virt_addr) return;

    /* Optimized clear for 32bpp */
    if (dev->bpp == 32) {
        uint32_t* buf = (uint32_t*)dev->virt_addr;
        uint32_t count = dev->width * dev->height;
        /* Note: This assumes pitch == width * 4, which is usually true for VESA 32bpp 
           but strictly we should respect pitch. For full correctness line-by-line is safer,
           but for a clear operation on linear LFB, this is usually fine. */
        for (uint32_t i = 0; i < count; ++i) buf[i] = color;
    } else {
        vesa_fillrect(dev, 0, 0, dev->width, dev->height, color);
    }
}

static int vesa_set_mode(gpu_device_t* dev, uint32_t width, uint32_t height, uint32_t bpp) {
    /* VESA driver cannot switch modes at runtime (requires BIOS call / vm86) */
    if (width == dev->width && height == dev->height && bpp == dev->bpp) return 0;
    serial("[GPU] VESA: Mode switch requested but not supported.\n");
    return -1;
}

static void vesa_flush(gpu_device_t* dev) {
    (void)dev;
    /* Linear Framebuffer is usually uncached (WC) or direct, no explicit flush needed */
    asm volatile("sfence" ::: "memory");
}

static gpu_ops_t vesa_ops = {
    .set_mode = vesa_set_mode,
    .putpixel = vesa_putpixel,
    .fillrect = vesa_fillrect,
    .clear    = vesa_clear,
    .flush    = vesa_flush
};

static gpu_device_t vesa_device = {0};

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

    /* Align size to page boundary (4KB) to ensure complete mapping coverage */
    fb_size = (fb_size + 0xFFF) & 0xFFFFF000;
    
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

    /* Register as GPU Device */
    vesa_device.type = GPU_TYPE_VESA;
    vesa_device.width = fb_width;
    vesa_device.height = fb_height;
    vesa_device.pitch = fb_pitch;
    vesa_device.bpp = fb_bpp;
    vesa_device.phys_addr = (uintptr_t)phys_addr;
    vesa_device.virt_addr = fb_buffer;
    vesa_device.ops = &vesa_ops;

    gpu_register_device(&vesa_device);

    /* Clear screen to black to verify write access */
    vesa_clear(&vesa_device, 0x000000);
}

/* Legacy wrappers for compatibility if needed, redirecting to GPU ops */
void fb_putpixel(uint32_t x, uint32_t y, uint32_t color) {
    vesa_putpixel(&vesa_device, x, y, color);
}

void fb_clear(uint32_t color) {
    vesa_clear(&vesa_device, color);
}

void fb_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    vesa_fillrect(&vesa_device, x, y, w, h, color);
}

void fb_get_info(uint32_t* width, uint32_t* height, uint32_t* pitch, uint8_t* bpp, uint8_t** buffer) {
    if (width) *width = fb_width;
    if (height) *height = fb_height;
    if (pitch) *pitch = fb_pitch;
    if (bpp) *bpp = fb_bpp;
    if (buffer) *buffer = fb_buffer;
}
