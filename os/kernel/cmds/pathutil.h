#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void cmd_resolve_path(const char* input, char* out, size_t out_size);

#ifdef __cplusplus
}
#endif
