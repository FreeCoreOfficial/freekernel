#pragma once
#include <stddef.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

void* malloc(size_t size);
void free(void* ptr);
void* calloc(size_t nmemb, size_t size);
void* realloc(void* ptr, size_t size);
int atoi(const char* nptr);
double atof(const char* nptr);
void exit(int status);
char* getenv(const char* name);
int system(const char* command);
int abs(int j);

#ifdef __cplusplus
}
#endif