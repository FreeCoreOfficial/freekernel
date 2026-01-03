#include "kmalloc.h"
#include "slab.h"
#include "buddy.h"
#include <stddef.h>

static slab_cache_t* cache_8;
static slab_cache_t* cache_16;
static slab_cache_t* cache_32;
static slab_cache_t* cache_64;

void kmalloc_init(void) {
    buddy_init(1024); /* exemplu: 1024 cadre; adaptează la memoria reală */
    cache_8  = slab_create("8", 8);
    cache_16 = slab_create("16", 16);
    cache_32 = slab_create("32", 32);
    cache_64 = slab_create("64", 64);
}

void* kmalloc(size_t size) {
    if (size <= 8) return slab_alloc(cache_8);
    if (size <= 16) return slab_alloc(cache_16);
    if (size <= 32) return slab_alloc(cache_32);
    if (size <= 64) return slab_alloc(cache_64);
    /* pentru alocări mari, alocăm pagini (order 0 = 4KB; calculează order după size) */
    size_t pages = (size + 0xFFF) >> 12;
    int order = 0;
    while ((1u<<order) < pages) order++;
    return buddy_alloc_page(order);
}

void kfree(void* ptr) {
    /* simplificat: detectează dacă ptr aparține unui slab (caută la cache-uri) */
    /* pentru moment: dacă ptr este page-aligned, dăm buddy_free_page order 0 */
    if (((uintptr_t)ptr & 0xFFF) == 0) {
        buddy_free_page(ptr, 0);
    } else {
        /* încercăm la fiecare cache */
        slab_free(cache_8, ptr);
        slab_free(cache_16, ptr);
        slab_free(cache_32, ptr);
        slab_free(cache_64, ptr);
    }
}
