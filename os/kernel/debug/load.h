#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void load_begin(const char* name);
void load_ok();
void load_fail();

#ifdef __cplusplus
}
#endif
