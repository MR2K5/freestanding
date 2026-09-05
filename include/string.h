#pragma once

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

[[gnu::nothrow]] void* memcpy(void* __restrict dst, void const* __restrict src, size_t sz);
[[gnu::nothrow]] void* memmove(void* dst, void const* src, size_t sz);
[[gnu::nothrow]] void*
memccpy(void* __restrict s1, void const* __restrict s2, int c, size_t n);  // freestanding
[[gnu::nothrow]] void* memset(void* dst, int c, size_t sz);

[[gnu::nothrow]] char* strcpy(char* __restrict s1, char const* __restrict s2);  // freestanding
[[gnu::nothrow]] char*
strncpy(char* __restrict s1, char const* __restrict s2, size_t n);  // freestanding
// [[gnu::nothrow]] char* strdup(char const* s);
// [[gnu::nothrow]] char* strndup(char const* s, size_t size);
[[gnu::nothrow]] char* strcat(char* s1, char const* s2);                // freestanding
[[gnu::nothrow]] char* strncat(char* s1, char const* s2, size_t n);     // freestanding
[[gnu::nothrow]] int memcmp(void const* s1, void const* s2, size_t n);  // freestanding
[[gnu::nothrow]] int strcmp(char const* s1, char const* s2);            // freestanding
[[gnu::nothrow]] int strcoll(char const* s1, char const* s2);
[[gnu::nothrow]] int strncmp(char const* s1, char const* s2, size_t n);  // freestanding
[[gnu::nothrow]] size_t strxfrm(char* s1, char const* s2, size_t n);
[[gnu::nothrow]] void const*
memchr(void const* s, int c, size_t n);  // freestanding; see [library.c]
// [[gnu::nothrow]] void* memchr(void* s, int c, size_t n);               // freestanding; see
// [library.c]
[[gnu::nothrow]] char* strchr(char const* s, int c);  // freestanding; see [library.c]
// [[gnu::nothrow]] char* strchr(char* s, int c);                         // freestanding; see
// [library.c]
[[gnu::nothrow]] size_t strcspn(char const* s1, char const* s2);  // freestanding
[[gnu::nothrow]] char* strpbrk(char const* s1, char const* s2);   // freestanding; see [library.c]
// [[gnu::nothrow]] char* strpbrk(char* s1, char const* s2);              // freestanding; see
// [library.c]
[[gnu::nothrow]] char* strrchr(char const* s, int c);  // freestanding; see [library.c]
// [[gnu::nothrow]] char* strrchr(char* s, int c);                        // freestanding; see
// [library.c]
[[gnu::nothrow]] size_t strspn(char const* s1, char const* s2);  // freestanding
[[gnu::nothrow]] char* strstr(char const* s1, char const* s2);   // freestanding; see [library.c]
// [[gnu::nothrow]] char* strstr(char* s1, char const* s2);               // freestanding; see
// [library.c]
[[gnu::nothrow]] char* strtok(char* s1, char const* s2);
[[gnu::nothrow]] void* memset(void* s, int c, size_t n);           // freestanding
[[gnu::nothrow]] void* memset_explicit(void* s, int c, size_t n);  // freestanding
[[gnu::nothrow]] char* strerror(int errnum);
[[gnu::nothrow]] size_t strlen(char const* s);

[[gnu::nothrow]] char* strtok_r(char* str, char const* delim, char** saveptr);

#ifdef __AVR__

[[gnu::nothrow]] void*
memcpy_p(void* __restrict dst, void const __flash* __restrict src, size_t sz);
[[gnu::nothrow]] void* memmove_p(void* dst, void const __flash* src, size_t sz);

#endif

#ifdef __cplusplus
}
#endif
