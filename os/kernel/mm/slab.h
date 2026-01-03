#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct slab_cache slab_cache_t;

slab_cache_t* slab_create(const char* name, size_t obj_size);
void* slab_alloc(slab_cache_t* cache);
void slab_free(slab_cache_t* cache, void* obj);
