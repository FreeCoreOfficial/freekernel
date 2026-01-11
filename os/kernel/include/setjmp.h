#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ebx, esi, edi, ebp, esp, eip */
typedef struct {
    uint32_t regs[6];
} jmp_buf[1];

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);

#ifdef __cplusplus
}
#endif