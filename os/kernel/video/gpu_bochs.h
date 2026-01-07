#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize Bochs/QEMU BGA driver if device is present */
void gpu_bochs_init(void);

#ifdef __cplusplus
}
#endif