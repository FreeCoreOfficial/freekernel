// kernel/detect/ram.h
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHRYSALIS_MIN_RAM_MB 20

uint32_t ram_detect_mb(uint32_t multiboot_magic, uint32_t multiboot_addr);
void ram_check_or_panic(uint32_t multiboot_magic, uint32_t multiboot_addr);

#ifdef __cplusplus
}
#endif
