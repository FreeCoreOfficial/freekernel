#include <stdint.h>
#include <stddef.h>
#include "../os/kernel/mem/kmalloc.h"

static uint8_t* g_heap = 0;
static size_t g_heap_size = 0;
static size_t g_heap_off = 0;

extern "C" void kmalloc_init(void) {
    static uint8_t heap[2 * 1024 * 1024];
    g_heap = heap;
    g_heap_size = sizeof(heap);
    g_heap_off = 0;
}

static void* kmalloc_align(size_t size, size_t alignment) {
    if (!g_heap) return 0;
    if (alignment == 0) alignment = 1;
    size_t off = g_heap_off;
    size_t aligned = (off + (alignment - 1)) & ~(alignment - 1);
    if (aligned + size > g_heap_size) return 0;
    g_heap_off = aligned + size;
    return g_heap + aligned;
}

extern "C" void* kmalloc(size_t size) {
    return kmalloc_align(size, 8);
}

extern "C" void* kmalloc_aligned(size_t size, size_t alignment) {
    return kmalloc_align(size, alignment);
}

extern "C" void kfree(void* p) {
    (void)p;
}
