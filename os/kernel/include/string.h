#pragma once
#include "../string.h"

#ifdef __cplusplus
extern "C" {
#endif

char* strdup(const char* s);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);

#ifdef __cplusplus
}
#endif