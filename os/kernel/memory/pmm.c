#include "pmm.h"
#include "../smp/multiboot.h"
#include <stddef.h>
#include <stdint.h>
#include "../drivers/serial.h"

/* kernel symbol (defined in linker.ld) */
extern uint32_t kernel_end;

/* bitmap data */
static uint32_t* bitmap;
static uint32_t  total_frames;
static uint32_t  used_frames;

/* Import serial logging from kernel glue */
extern void serial(const char *fmt, ...);

/* align helper – SINGLE VERSION */
static inline uintptr_t align_up_uintptr(uintptr_t v)
{
    return (v + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

/* bitmap helpers */
static inline void bitmap_set(uint32_t frame)
{
    bitmap[frame / 32] |= (1u << (frame % 32));
}

static inline void bitmap_clear(uint32_t frame)
{
    bitmap[frame / 32] &= ~(1u << (frame % 32));
}

static inline int bitmap_test(uint32_t frame)
{
    return (bitmap[frame / 32] & (1u << (frame % 32))) != 0;
}

/* API */
void pmm_init(uint32_t mb_magic, void* mb_addr)
{
    serial("[PMM] init start\n");

    uint64_t total_ram_bytes = 0;
    uintptr_t mmap_addr = 0;
    uint32_t mmap_len = 0;
    uint32_t mmap_entry_size = 0;
    int is_mb2 = 0;

    /* 1. Detect RAM size and find mmap tag */
    if (mb_magic == MULTIBOOT2_BOOTLOADER_MAGIC && mb_addr != 0) {
        is_mb2 = 1;
        struct multiboot2_tag *tag = (struct multiboot2_tag*)((uint8_t*)mb_addr + 8);
        while (tag->type != MULTIBOOT2_TAG_TYPE_END) {
            if (tag->type == MULTIBOOT2_TAG_TYPE_BASIC_MEMINFO) {
                 struct multiboot2_tag_basic_meminfo *mem = (struct multiboot2_tag_basic_meminfo*)tag;
                 if (total_ram_bytes == 0) total_ram_bytes = (uint64_t)(mem->mem_upper + 1024) * 1024;
            } else if (tag->type == MULTIBOOT2_TAG_TYPE_MMAP) {
                 struct multiboot2_tag_mmap *mmap = (struct multiboot2_tag_mmap*)tag;
                 mmap_addr = (uintptr_t)mmap->entries;
                 mmap_entry_size = mmap->entry_size;
                 
                 // Calculate total RAM from available regions
                 uint64_t max_addr = 0;
                 for (struct multiboot2_mmap_entry *entry = mmap->entries;
                      (uint8_t*)entry < (uint8_t*)mmap + mmap->common.size;
                      entry = (struct multiboot2_mmap_entry*)((uint8_t*)entry + mmap->entry_size)) {
                     if (entry->type == MULTIBOOT2_MEMORY_AVAILABLE) {
                         uint64_t end = entry->addr + entry->len;
                         if (end > max_addr) max_addr = end;
                     }
                 }
                 if (max_addr > total_ram_bytes) total_ram_bytes = max_addr;
            }
            tag = (struct multiboot2_tag*)((uint8_t*)tag + ((tag->size + 7) & ~7));
        }
    } else if (mb_magic == MULTIBOOT_MAGIC && mb_addr != 0) {
        multiboot_info_t* mbi = (multiboot_info_t*)mb_addr;
        if (mbi->flags & 1) {
            total_ram_bytes = (uint64_t)(mbi->mem_upper + 1024) * 1024;
        }
        if (mbi->flags & (1<<6)) {
            mmap_addr = mbi->mmap_addr;
            mmap_len = mbi->mmap_length;
        }
    }

    total_frames = total_ram_bytes / PAGE_SIZE;

    /* bitmap size in bytes, aligned up to PAGE_SIZE */
    uintptr_t bitmap_bytes = (uintptr_t)((total_frames + 7) / 8);
    bitmap_bytes = align_up_uintptr(bitmap_bytes);

    /* place bitmap right after kernel_end, page-aligned */
    bitmap = (uint32_t*)align_up_uintptr((uintptr_t)&kernel_end);

    /* 2. mark all frames as USED initially (set all bits = 1) */
    uint32_t bitmap_dwords = (uint32_t)(bitmap_bytes / 4);
    for (uint32_t i = 0; i < bitmap_dwords; i++)
        bitmap[i] = 0xFFFFFFFFu;

    used_frames = total_frames;

    /* 3. parse multiboot memory map and free AVAILABLE (type == 1) frames */
    if (is_mb2 && mmap_addr != 0) {
        // Re-find the tag to iterate cleanly or use stored pointers
        struct multiboot2_tag *tag = (struct multiboot2_tag*)((uint8_t*)mb_addr + 8);
        while (tag->type != MULTIBOOT2_TAG_TYPE_END) {
            if (tag->type == MULTIBOOT2_TAG_TYPE_MMAP) {
                struct multiboot2_tag_mmap *mmap = (struct multiboot2_tag_mmap*)tag;
                for (struct multiboot2_mmap_entry *entry = mmap->entries;
                     (uint8_t*)entry < (uint8_t*)mmap + mmap->common.size;
                     entry = (struct multiboot2_mmap_entry*)((uint8_t*)entry + mmap->entry_size)) {
                    
                    if (entry->type == MULTIBOOT2_MEMORY_AVAILABLE) {
                        uint64_t start = entry->addr;
                        uint64_t end = entry->addr + entry->len;
                        
                        // Align to page boundaries
                        for (uint64_t addr = start; addr < end; addr += PAGE_SIZE) {
                            uint32_t frame = (uint32_t)(addr / PAGE_SIZE);
                            if (frame < total_frames) {
                                if (bitmap_test(frame)) {
                                    bitmap_clear(frame);
                                    used_frames--;
                                }
                            }
                        }
                    }
                }
                break; 
            }
            tag = (struct multiboot2_tag*)((uint8_t*)tag + ((tag->size + 7) & ~7));
        }
    } else if (!is_mb2 && mmap_addr != 0) {
        // Legacy Multiboot 1 logic (omitted for brevity as we are on MB2 now, but structure is similar)
        // ...
    }

    /* 4. reserve kernel frames (0 .. kernel_end_frame) + bitmap frames */
    // Reserve everything below 1MB (BIOS/VGA/Trampoline area)
    // Reserve kernel code/data
    uint32_t kernel_end_f = (uint32_t)(((uintptr_t)&kernel_end) >> 12);
    for (uint32_t f = 0; f <= kernel_end_f; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            used_frames++;
        }
    }

    /* Reserve bitmap itself */
    uintptr_t bitmap_start = (uintptr_t)bitmap;
    uintptr_t bitmap_end   = bitmap_start + bitmap_bytes;

    for (uintptr_t addr = bitmap_start; addr < bitmap_end; addr += PAGE_SIZE) {
        uint32_t f = (uint32_t)(addr >> 12);
        if (!bitmap_test(f)) {
            bitmap_set(f);
            used_frames++;
        }
    }

    /* Silence unused warnings */
    (void)mmap_len;
    (void)mmap_entry_size;

    serial("[PMM] init done. Total: %u, Used: %u, Free: %u\n", total_frames, used_frames, total_frames - used_frames);
}

uint32_t pmm_alloc_frame(void)
{
    for (uint32_t f = 0; f < total_frames; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            used_frames++;
            return f * PAGE_SIZE;
        }
    }
    return 0; /* out of memory */
}

void pmm_free_frame(uint32_t phys_addr)
{
    uint32_t frame = phys_addr / PAGE_SIZE;
    if (frame >= total_frames) return;
    if (bitmap_test(frame)) {
        bitmap_clear(frame);
        used_frames--;
    }
}

void pmm_reserve_area(uint32_t start_addr, uint32_t size)
{
    uint32_t start_frame = start_addr / PAGE_SIZE;
    uint32_t num_frames = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint32_t i = 0; i < num_frames; i++) {
        uint32_t f = start_frame + i;
        if (f < total_frames) {
            if (!bitmap_test(f)) {
                bitmap_set(f);
                used_frames++;
            }
        }
    }
}

uint32_t pmm_total_frames(void)
{
    return total_frames;
}

uint32_t pmm_used_frames(void)
{
    return used_frames;
}

/* Inspect helper exported to commands/debugging */
int pmm_is_frame_used(uint32_t frame)
{
    if (frame >= total_frames) return -1;
    return (bitmap[frame / 32] & (1u << (frame % 32))) ? 1 : 0;
}
