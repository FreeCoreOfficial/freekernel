/* kernel/string.cpp
   Simple, small implementations of common string/memory routines
   suitable for a freestanding kernel.
*/

#include "string.h"
#include <stddef.h>
#include <stdint.h>

/* ---------- string ops ---------- */

size_t strlen(const char* s)
{
    const char* p = s;
    while (*p) ++p;
    return (size_t)(p - s);
}

int strcmp(const char* a, const char* b)
{
    while (*a && (*a == *b)) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char* a, const char* b, size_t n)
{
    size_t i = 0;
    for (; i < n; ++i) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca != cb) return ca - cb;
        if (ca == 0) return 0;
    }
    return 0;
}

char* strcpy(char* dst, const char* src)
{
    char* r = dst;
    while ((*dst++ = *src++)) {}
    return r;
}

char* strncpy(char* dst, const char* src, size_t n)
{
    size_t i = 0;
    for (; i < n && src[i]; ++i) dst[i] = src[i];
    for (; i < n; ++i) dst[i] = '\0';
    return dst;
}

char* strcat(char* dst, const char* src)
{
    char* d = dst;
    while (*d) ++d;
    while ((*d++ = *src++)) {}
    return dst;
}

char* strchr(const char* s, int c)
{
    unsigned char uc = (unsigned char)c;
    while (*s) {
        if ((unsigned char)*s == uc) return (char*)s;
        ++s;
    }
    return (uc == 0) ? (char*)s : NULL;
}

char* strstr(const char* haystack, const char* needle)
{
    if (!*needle) return (char*)haystack;
    for (; *haystack; ++haystack) {
        const char* h = haystack;
        const char* n = needle;
        while (*n && *h == *n) { ++h; ++n; }
        if (!*n) return (char*)haystack;
    }
    return NULL;
}

/* ---------- memory ops ---------- */

void* memcpy(void* dest, const void* src, size_t n)
{
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    for (size_t i = 0; i < n; ++i) d[i] = s[i];
    return dest;
}

void* memmove(void* dest, const void* src, size_t n)
{
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    if (d < s) {
        for (size_t i = 0; i < n; ++i) d[i] = s[i];
    } else if (d > s) {
        for (size_t i = n; i > 0; --i) d[i-1] = s[i-1];
    }
    return dest;
}

void* memset(void* s, int c, size_t n)
{
    unsigned char* p = (unsigned char*)s;
    unsigned char uc = (unsigned char)c;
    for (size_t i = 0; i < n; ++i) p[i] = uc;
    return s;
}

int memcmp(const void* s1, const void* s2, size_t n)
{
    const unsigned char* a = (const unsigned char*)s1;
    const unsigned char* b = (const unsigned char*)s2;
    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i]) return a[i] - b[i];
    }
    return 0;
}

/* ---------- small integer->string helpers (useful for debug) ---------- */

char* itoa_dec(char* out, int32_t v)
{
    char buf[16];
    int i = 0;
    int neg = 0;
    uint32_t val;
    if (v < 0) { neg = 1; val = (uint32_t)(- (int64_t)v); }
    else val = (uint32_t)v;

    if (val == 0) buf[i++] = '0';
    while (val) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    if (neg) buf[i++] = '-';
    int j = 0;
    while (i > 0) out[j++] = buf[--i];
    out[j] = '\0';
    return out;
}

static const char* hex_digits = "0123456789abcdef";
char* utoa_hex(char* out, uint32_t v)
{
    /* produce minimal hex without 0x prefix */
    char buf[9];
    int i = 0;
    if (v == 0) buf[i++] = '0';
    while (v) {
        buf[i++] = hex_digits[v & 0xF];
        v >>= 4;
    }
    int j = 0;
    while (i > 0) out[j++] = buf[--i];
    out[j] = '\0';
    return out;
}
