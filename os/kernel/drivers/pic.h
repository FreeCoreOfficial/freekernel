#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void pic_remap(void);
void pic_send_eoi(uint8_t irq);

#ifdef __cplusplus
}
#endif
