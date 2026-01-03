// kernel/user/user_blob.c
#include <stdint.h>

/* user_prog.bin.inc a fost generat cu `xxd -i build/user_prog.bin`
   și conține:
     unsigned char build_user_prog_bin[] = { ... };
     unsigned int  build_user_prog_bin_len = ...;
*/

/* Include direct .inc — acesta definește array-ul și len */
#include "user_prog.bin.inc"

/* Expunem un nume consistent (tipuri fixe) pe care restul kernel-ului îl poate folosi */
__attribute__((section(".rodata")))
const uint8_t *user_prog = (const uint8_t *)build_user_prog_bin;

const uint32_t user_prog_len = (const uint32_t)build_user_prog_bin_len;
