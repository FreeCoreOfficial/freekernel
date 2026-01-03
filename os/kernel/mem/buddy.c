/* kernel/mem/buddy.c
   Simple buddy allocator backed by a contiguous page range (buddy_base .. buddy_base + buddy_page_count*PAGE).
   It expects buddy_init_from_heap() to set buddy_base and buddy_page_count and call buddy_init().
*/

#include "buddy.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* If you use PMM functions elsewhere, keep the include; not needed here though.
   #include "../memory/pmm.h"
*/

/* Linker-provided heap bounds (defined in linker.ld) */
extern uint8_t __heap_start;
extern uint8_t __heap_end;

/* configuration */
#define PAGE_SIZE 0x1000
#ifndef BUDDY_MAX_ORDER
#define BUDDY_MAX_ORDER 10
#endif

typedef struct free_block {
    struct free_block* next;
} free_block_t;

/* free lists indexed by order: list of blocks of size (1 << order) pages */
static free_block_t* free_lists[BUDDY_MAX_ORDER + 1];

/* total frames the buddy manages and base physical address */
static uint32_t frames_total = 0;
static uintptr_t buddy_base = 0;
static uint32_t buddy_page_count = 0;

/* helpers: convert page-index <-> address */
static inline uintptr_t page_index_to_addr(uint32_t idx) {
    return buddy_base + (uintptr_t)idx * PAGE_SIZE;
}
static inline uint32_t addr_to_page_index(uintptr_t addr) {
    return (uint32_t)((addr - buddy_base) / PAGE_SIZE);
}

/* push block into free list (order) */
static void fl_push(int order, free_block_t* block) {
    block->next = free_lists[order];
    free_lists[order] = block;
}

/* remove a specific block pointer from free list order; returns 1 if removed */
static int fl_remove(int order, free_block_t* target) {
    free_block_t** cur = &free_lists[order];
    while (*cur) {
        if (*cur == target) {
            *cur = target->next;
            return 1;
        }
        cur = &((*cur)->next);
    }
    return 0;
}

/* find a buddy block pointer in free list at given order matching address; returns pointer or NULL */
static free_block_t* fl_find_by_addr(int order, uintptr_t addr) {
    free_block_t* cur = free_lists[order];
    while (cur) {
        if ((uintptr_t)cur == addr) return cur;
        cur = cur->next;
    }
    return NULL;
}

/* Initialize buddy free lists for total_frames pages.
   It assumes buddy_base and buddy_page_count are set by buddy_init_from_heap() caller.
*/
void buddy_init(uint32_t total_frames_param) {
    /* clear lists */
    frames_total = total_frames_param;
    buddy_page_count = total_frames_param;
    for (int i = 0; i <= BUDDY_MAX_ORDER; ++i) free_lists[i] = NULL;

    if (frames_total == 0 || buddy_base == 0) return;

    /* cover the region with largest power-of-two blocks (greedy) */
    uint32_t remaining = frames_total;
    uint32_t base_idx = 0;

    while (remaining > 0) {
        /* find largest order <= BUDDY_MAX_ORDER s.t. (1<<order) <= remaining */
        int order = 0;
        while ((1u << (order + 1)) <= remaining && order + 1 <= BUDDY_MAX_ORDER) order++;
        /* create block at base_idx with 'order' */
        uintptr_t addr = page_index_to_addr(base_idx);
        free_block_t* blk = (free_block_t*)addr;
        blk->next = NULL;
        fl_push(order, blk);

        uint32_t block_pages = (1u << order);
        base_idx += block_pages;
        remaining -= block_pages;
    }
}

/* Allocate a block of size (1<<order) pages. Returns physical address (page aligned) or NULL. */
void* buddy_alloc_page(int order) {
    if (order < 0 || order > BUDDY_MAX_ORDER) return NULL;

    /* find first order >= requested that has a free block */
    int o;
    for (o = order; o <= BUDDY_MAX_ORDER; ++o) {
        if (free_lists[o]) break;
    }
    if (o > BUDDY_MAX_ORDER) return NULL; /* no block available */

    /* take block from free_lists[o] */
    free_block_t* blk = free_lists[o];
    free_lists[o] = blk->next;

    /* split down to requested order */
    while (o > order) {
        --o;
        /* split: current block at address 'blk' -> two buddies:
           left = blk (same address)
           right = left + (1<<o)*PAGE_SIZE
           we push right into free_lists[o]
        */
        uintptr_t left_addr = (uintptr_t)blk;
        uintptr_t right_addr = left_addr + ((uintptr_t)(1u << o) * PAGE_SIZE);
        free_block_t* right_blk = (free_block_t*)right_addr;
        right_blk->next = free_lists[o];
        free_lists[o] = right_blk;
        /* keep left as blk (already) and continue */
        blk = (free_block_t*)left_addr;
    }

    return (void*)blk;
}

/* Free a block at phys with given order. Will try to coalesce up. */
void buddy_free_page(void* phys, int order) {
    if (!phys) return;
    if (order < 0 || order > BUDDY_MAX_ORDER) return;
    if (buddy_base == 0) return;

    uintptr_t addr = (uintptr_t)phys;
    /* ensure address aligned and inside managed region */
    if (addr < buddy_base) return;
    uintptr_t end_addr = buddy_base + (uintptr_t)buddy_page_count * PAGE_SIZE;
    if (addr >= end_addr) return;
    if ((addr - buddy_base) % PAGE_SIZE != 0) return;

    uint32_t index = addr_to_page_index(addr);

    int o = order;
    uintptr_t cur_addr = addr;
    while (o < BUDDY_MAX_ORDER) {
        uint32_t block_pages = (1u << o);
        uint32_t buddy_index = index ^ block_pages; /* flip o-th bit */
        uintptr_t buddy_addr = page_index_to_addr(buddy_index);

        /* check if buddy block is free at this order */
        free_block_t* buddy_blk = fl_find_by_addr(o, buddy_addr);
        if (buddy_blk == NULL) {
            break; /* cannot coalesce */
        }

        /* remove buddy from free list and merge */
        fl_remove(o, buddy_blk);

        /* new merged block is at min(cur_addr, buddy_addr) */
        if (buddy_addr < cur_addr) cur_addr = buddy_addr;
        index = addr_to_page_index(cur_addr);
        ++o; /* try to coalesce one order up */
    }

    /* push merged block into free_lists[o] */
    free_block_t* newblk = (free_block_t*)cur_addr;
    newblk->next = free_lists[o];
    free_lists[o] = newblk;
}

/* Bind buddy base to heap region and call buddy_init with computed pages.
   This is called externally (from kernel_main) after heap_init has been run.
*/
void buddy_init_from_heap(void) {
    uintptr_t start = (uintptr_t)&__heap_start;
    uintptr_t end   = (uintptr_t)&__heap_end;

    /* align start up to page boundary */
    uintptr_t aligned_start = (start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if (end <= aligned_start) {
        /* nothing to manage */
        buddy_base = 0;
        buddy_page_count = 0;
        buddy_init(0);
        return;
    }

    size_t bytes = end - aligned_start;
    uint32_t pages = (uint32_t)(bytes / PAGE_SIZE);
    buddy_base = aligned_start;
    buddy_page_count = pages;

    buddy_init(pages);
}
