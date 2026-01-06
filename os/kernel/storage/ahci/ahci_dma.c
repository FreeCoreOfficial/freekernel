#include "ahci.h"

/* 
 * AHCI DMA Pool
 * Static buffer in BSS (guaranteed physically contiguous by bootloader/linker layout in simple kernels).
 * Size: 64KB (enough for 32 ports * (1KB CLB + 256B FB + CTs))
 * Alignment: 4096 to be safe for page boundaries.
 */
static uint8_t __attribute__((aligned(4096))) ahci_dma_pool[64 * 1024];
static uint32_t dma_offset = 0;

/* Simple bump allocator with alignment support */
void* ahci_dma_alloc(size_t size, size_t align) {
    uint32_t start = (uint32_t)&ahci_dma_pool[dma_offset];
    
    /* Calculate aligned address */
    uint32_t aligned = (start + align - 1) & ~(align - 1);
    uint32_t padding = aligned - start;
    
    if (dma_offset + padding + size > sizeof(ahci_dma_pool)) {
        serial("[AHCI] DMA Allocator OOM! Requested %d bytes\n", size);
        return NULL;
    }
    
    dma_offset += padding + size;
    
    /* Zero the memory */
    uint8_t* p = (uint8_t*)aligned;
    for(size_t i=0; i<size; i++) p[i] = 0;
    
    return (void*)aligned;
}

uint32_t ahci_virt_to_phys(void* v) {
    uint32_t virt = (uint32_t)v;
    /* Kernel mapped at 0xC0000000 -> Phys 0x00100000 */
    if (virt >= 0xC0000000) {
        return virt - 0xC0000000 + 0x00100000;
    }
    return virt; /* Identity otherwise */
}