// kernel/detect/videomemory.c
#include "videomemory.h"

#include "../terminal.h"
#include "../panic.h"

#define MULTIBOOT_MAGIC 0x2BADB002u
#define CHRYSALIS_MIN_VIDEO_MB 8

/* minimal multiboot framebuffer fields */
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;

    /* framebuffer info (offsets matter only after flags) */
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
} multiboot_info_t;

uint32_t video_detect_mb(uint32_t multiboot_magic, uint32_t multiboot_addr)
{
    if (multiboot_magic != MULTIBOOT_MAGIC || multiboot_addr == 0) {
        /* no multiboot info → assume VGA legacy */
        return CHRYSALIS_MIN_VIDEO_MB;
    }

    multiboot_info_t *mb =
        (multiboot_info_t *)(uintptr_t)multiboot_addr;

    /* bit 12 = framebuffer info present */
    if (!(mb->flags & (1 << 12))) {
        /* text mode / VGA fallback */
        return CHRYSALIS_MIN_VIDEO_MB;
    }

    if (mb->framebuffer_pitch == 0 ||
        mb->framebuffer_height == 0) {
        return 0;
    }

    uint64_t bytes =
        (uint64_t)mb->framebuffer_pitch *
        (uint64_t)mb->framebuffer_height;

    return (uint32_t)(bytes / (1024 * 1024));
}

void video_check_or_panic(uint32_t multiboot_magic, uint32_t multiboot_addr)
{
    uint32_t vram_mb = video_detect_mb(multiboot_magic, multiboot_addr);

    terminal_printf("[video] detected %u MB video memory\n", vram_mb);

    if (vram_mb < CHRYSALIS_MIN_VIDEO_MB) {
        panic(
            "INSUFFICIENT VIDEO MEMORY\n"
            "-------------------------\n"
            "Minimum required: 8 MB\n\n"
            "Detected video memory is below minimum.\n"
            "Increase VRAM in VM settings or enable framebuffer mode."
        );
    }
}
