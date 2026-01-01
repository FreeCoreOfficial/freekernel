#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void speaker_init();
void speaker_on(uint32_t freq);
void speaker_off();
void speaker_beep(uint32_t freq, uint32_t ms);

#ifdef __cplusplus
}
#endif
