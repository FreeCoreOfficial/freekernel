#include "slab.h"
#include "buddy.h" /* pentru pagini */
#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 0x1000

typedef struct slab {
    struct slab* next;
    void* free_list;
    uint32_t free_count;
    char data[]; /* începutul obiectelor */
} slab_t;

struct slab_cache {
    const char* name;
    size_t obj_size;
    slab_t* slabs;
};

static inline size_t align_up(size_t v, size_t a){ return (v + a - 1) & ~(a-1); }

slab_cache_t* slab_create(const char* name, size_t obj_size) {
    slab_cache_t* c = (slab_cache_t*)buddy_alloc_page(0); /* folosește o pagină pentru struct cache */
    if (!c) return NULL;
    c->name = name;
    c->obj_size = (obj_size < sizeof(void*)) ? sizeof(void*) : obj_size;
    c->slabs = NULL;
    return c;
}

static slab_t* slab_alloc_new_slab(slab_cache_t* cache) {
    void* page = buddy_alloc_page(0);
    if (!page) return NULL;
    slab_t* s = (slab_t*)page;
    s->next = cache->slabs;
    cache->slabs = s;
    size_t hdr = sizeof(slab_t);
    size_t usable = PAGE_SIZE - hdr;
    size_t obj_sz = align_up(cache->obj_size, sizeof(void*));
    int count = usable / obj_sz;
    s->free_count = count;
    s->free_list = NULL;
    uint8_t* ptr = (uint8_t*)s + hdr;
    for (int i=0;i<count;i++) {
        *(void**)ptr = s->free_list;
        s->free_list = ptr;
        ptr += obj_sz;
    }
    return s;
}

void* slab_alloc(slab_cache_t* cache) {
    slab_t* s = cache->slabs;
    while (s && s->free_list == NULL) s = s->next;
    if (!s) s = slab_alloc_new_slab(cache);
    if (!s) return NULL;
    void* obj = s->free_list;
    s->free_list = *(void**)obj;
    s->free_count--;
    return obj;
}

void slab_free(slab_cache_t* cache, void* obj) {
    /* simplificat: găsim slab-ul care conține obj parcurgând listele (cost O(n)) */
    for (slab_t* s = cache->slabs; s; s = s->next) {
        uintptr_t start = (uintptr_t)s;
        uintptr_t end = start + PAGE_SIZE;
        if ((uintptr_t)obj >= start && (uintptr_t)obj < end) {
            *(void**)obj = s->free_list;
            s->free_list = obj;
            s->free_count++;
            return;
        }
    }
}
