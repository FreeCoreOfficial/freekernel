#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void pit_init(uint32_t freq);
uint64_t pit_get_ticks();
void pit_sleep(uint32_t ms);

#ifdef __cplusplus
}
#endif
