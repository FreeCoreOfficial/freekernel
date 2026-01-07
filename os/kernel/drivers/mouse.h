#pragma once
#include <stdint.h>
#include "../interrupts/isr.h"

#ifdef __cplusplus
extern "C" {
#endif

void mouse_init(void);
void mouse_handler(registers_t* regs);

#ifdef __cplusplus
}
#endif
