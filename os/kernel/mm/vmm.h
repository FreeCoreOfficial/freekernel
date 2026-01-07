#pragma once

/* kernel/mm/vmm.h
 * Virtual Memory Manager helpers for Chrysalis OS
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Page Flags */
#define PAGE_PRESENT 0x01
#define PAGE_RW      0x02
#define PAGE_USER    0x04
#define PAGE_PWT     0x08 /* Page Write Through */
#define PAGE_PCD     0x10 /* Page Cache Disable */

/* Avoid redefining PAGE_SIZE if another header already defined it
 * (pmm.h uses #define PAGE_SIZE 4096).
 */
/*#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000u
#endif*/

extern uint32_t* kernel_page_directory;

/* Allocate one 4KiB page and return a kernel-virtual pointer to it.
 * The returned page is zeroed.
 */
void* vmm_alloc_page(void);

/* Free a page previously allocated with vmm_alloc_page.
 */
void vmm_free_page(void* page);

/* Map a single 4KiB page into the specified page-directory.
 * - pagedir: kernel-virtual pointer to the page directory (uint32_t*)
 * - vaddr  : virtual address to map (page-aligned)
 * - phys   : physical frame address (page-aligned)
 * - flags  : flags (PAGE_PRESENT | PAGE_RW | PAGE_USER ...)
 */
void vmm_map_page(uint32_t* pagedir, uint32_t vaddr, uint32_t phys, uint32_t flags);

void vmm_map_region(uint32_t* pagedir, uint32_t vaddr, uint32_t paddr, uint32_t size, uint32_t flags);

void vmm_unmap_page(uint32_t* pagedir, uint32_t vaddr);

void vmm_identity_map(uint32_t phys, uint32_t size);



uint32_t vmm_virt_to_phys(void* vaddr);

/* Get the current active page directory (virtual pointer) */
uint32_t* vmm_get_current_pd(void);

#ifdef __cplusplus
}
#endif
