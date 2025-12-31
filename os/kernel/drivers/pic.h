#pragma once
#include <stdint.h>

extern "C" void pic_remap();
extern "C" void pic_send_eoi(uint8_t irq);
