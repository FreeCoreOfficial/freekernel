/* file: kernel/detect/videomemory.c */
#include "videomemory.h"


#include "../terminal.h"
#include "../panic.h"
#include "../video/vga.h"
#include <stdint.h>


#define MULTIBOOT_MAGIC 0x2BADB002u


/* Helper to compute MB from bytes */
static uint32_t bytes_to_mb(uint64_t bytes) {
return (uint32_t)(bytes / (1024ULL * 1024ULL));
}


/* Return estimated framebuffer size in MB using VGA subsystem, or 0 if unknown */
uint32_t video_detect_mb(uint32_t multiboot_magic, uint32_t multiboot_addr)
{
(void)multiboot_magic; (void)multiboot_addr;


if (!vga_has_framebuffer()) return 0;


uint32_t width = vga_get_width();
uint32_t height = vga_get_height();
uint32_t bpp = vga_get_bpp();


if (width == 0 || height == 0) return 0;


uint32_t bytes_per_pixel = (bpp + 7) / 8;
if (bytes_per_pixel == 0) bytes_per_pixel = 1;


uint64_t total_bytes = (uint64_t)width * (uint64_t)height * (uint64_t)bytes_per_pixel;
return bytes_to_mb(total_bytes);
}


/* Main check: ensure framebuffer exists and meets minimum capability requirements.
If not, print diagnostics and call panic() with a concise message. */
void video_check_or_panic(uint32_t multiboot_magic, uint32_t multiboot_addr)
{
(void)multiboot_magic; (void)multiboot_addr;


/* Must have framebuffer available */
if (!vga_has_framebuffer()) {
terminal_writestring("[video] no framebuffer available via vga subsystem\n");
panic(
"INSUFFICIENT VIDEO MODE\n"
"----------------------\n"
"No framebuffer detected. Chrysalis requires a graphical framebuffer\n"
"capable of at least 1024x768@24bpp. Enable framebuffer/graphics mode\n"
"in your bootloader or VM, or pass a kernel cmdline like: vram=32M\n"
);
}


/* Query current framebuffer mode */
uint32_t width = vga_get_width();
uint32_t height = vga_get_height();
uint32_t bpp = vga_get_bpp();


terminal_printf("[video] framebuffer detected: %ux%u @ %u bpp\n", width, height, bpp);


/* Capability checks */
if (width < VIDEO_MIN_WIDTH || height < VIDEO_MIN_HEIGHT || bpp < VIDEO_MIN_BPP) {
terminal_printf("[video] insufficient video capabilities: required >= %ux%u@%ubpp\n",
(unsigned)VIDEO_MIN_WIDTH, (unsigned)VIDEO_MIN_HEIGHT, (unsigned)VIDEO_MIN_BPP);
terminal_printf("[video] detected: %ux%u@%ubpp\n", width, height, bpp);


/* Provide helpful hints and then panic with a short message */
terminal_writestring("[video] Hints:\n");
terminal_writestring(" - Increase the guest framebuffer resolution (GRUB gfxmode or VM settings)\n");
terminal_writestring(" - If running in a VM, enable 3D/VDI/VBoxVGA compatibility or use QEMU with -device VGA/virtio-gpu\n");
terminal_writestring(" - As a last resort, pass kernel cmdline: vram=32M (if supported by your bootloader)\n");


panic(
"INSUFFICIENT VIDEO MODE\n"
"----------------------\n"
"Framebuffer does not meet minimum capability (>= 1024x768 @ 24bpp).\n"
);
}


terminal_writestring("[video] video capability check OK\n");
}