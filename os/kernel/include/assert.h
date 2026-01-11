#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NDEBUG
#define assert(ignore) ((void)0)
#else
void panic(const char* message);
#define assert(expression) ((expression) ? (void)0 : panic("Assertion failed: " #expression))
#endif

#ifdef __cplusplus
}
#endif