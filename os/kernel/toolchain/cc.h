#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void toolchain_init();
void toolchain_test_read(const char *path);
int toolchain_compile_and_run(const char *source);

#ifdef __cplusplus
}
#endif
