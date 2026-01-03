#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Flags for pages */
#define PAGE_PRESENT   0x1
#define PAGE_RW        0x2
#define PAGE_USER      0x4

/* Initialize paging and identity-map the low memory region.
 * identity_map_mb: how many MB to identity-map (default 16 if use 0)
 */
void paging_init(uint32_t identity_map_mb);

/* Map a single 4KB page (virtual -> physical) with flags (PAGE_PRESENT|PAGE_RW ...).
 * Returns 0 on success, non-zero on error (no free page-table).
 * Note: virtual and physical must be page-aligned.
 */
int paging_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);

/* Unmap a single 4KB page (clears entry). */
void paging_unmap_page(uint32_t virtual_addr);

/* Get physical address of current page-directory (for debug). */
uint32_t paging_get_page_directory_phys(void);

#ifdef __cplusplus
}
#endif
