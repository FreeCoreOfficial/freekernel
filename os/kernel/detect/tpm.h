// kernel/detect/tpm.h
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Return values for tpm_detect_any():
 *  0  = no TPM detected
 * 12  = TPM 1.2 (TIS) detected
 * 20  = TPM 2.0 (CRB / VirtualBox / other) detected
 */
int tpm_detect_any(void);

/* panics if no acceptable TPM detected (accepts TPM1.2 OR TPM2.0 as present) */
void tpm_check_or_panic(void);

/* convenience helpers */
int tpm_detect_v12(void);
int tpm_detect_v2(void);

#ifdef __cplusplus
}
#endif
