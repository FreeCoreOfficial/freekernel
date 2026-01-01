#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void pit_init(uint32_t freq);
uint64_t pit_get_ticks();

#ifdef __cplusplus
}
#endif
