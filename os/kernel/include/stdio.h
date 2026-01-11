#pragma once
#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Minimal FILE type for freestanding kernel */
typedef struct FILE {
    int dummy;
} FILE;

/* Streams (optional, but useful) */
#define stdin  ((FILE*)0)
#define stdout ((FILE*)1)
#define stderr ((FILE*)2)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int printf(const char* fmt, ...);
int putchar(int c);
int puts(const char* s);
int fprintf(FILE* stream, const char* fmt, ...);
int vfprintf(FILE* stream, const char* format, va_list arg);
int sprintf(char* str, const char* fmt, ...);
int snprintf(char* str, size_t size, const char* fmt, ...);
int vsnprintf(char* str, size_t size, const char* fmt, va_list ap);
int sscanf(const char* str, const char* format, ...);
int fflush(FILE* stream);

/* File I/O Stubs */
FILE* fopen(const char* filename, const char* mode);
size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
int fseek(FILE* stream, long offset, int whence);
long ftell(FILE* stream);
int fclose(FILE* stream);
int remove(const char* filename);
int rename(const char* oldname, const char* newname);

#ifdef __cplusplus
}
#endif
