/* stdlib_wrappers.cpp - Connects C++ new/delete to kmalloc */
#include <stddef.h>
#include "kernel/include/types.h"
#include "kernel/mem/kmalloc.h"

extern "C" void* malloc(size_t size) {
    return kmalloc(size);
}

extern "C" void free(void* ptr) {
    kfree(ptr);
}

void* operator new(size_t size) {
    return kmalloc(size);
}

void* operator new[](size_t size) {
    return kmalloc(size);
}

void operator delete(void* p) {
    kfree(p);
}

void operator delete[](void* p) {
    kfree(p);
}

void operator delete(void* p, unsigned int) {
    kfree(p);
}

void operator delete[](void* p, unsigned int) {
    kfree(p);
}
