/* kernel/mem/kmalloc_init.c */
#include <stdint.h>
#include "kmalloc.h"   /* declară kmalloc_init în header deja */

extern uint8_t __heap_start; /* definite în linker.ld */
extern uint8_t __heap_end;

void kmalloc_init(void) {
    void* heap_start = (void*)&__heap_start;
    size_t heap_size = (size_t)(&__heap_end - &__heap_start);
    heap_init(heap_start, heap_size);
}
