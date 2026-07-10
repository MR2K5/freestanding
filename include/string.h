#pragma once

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

[[gnu::nothrow]] void* memcpy(void* __restrict dst, void const* __restrict src, size_t sz);
[[gnu::nothrow]] void* memmove(void* dst, void const* src, size_t sz);
[[gnu::nothrow]] void* memset(void* dst, int c, size_t sz);

#ifdef __AVR__

[[gnu::nothrow]] void* memcpy_p(void* __restrict dst, void const __flash* __restrict src, size_t sz);
[[gnu::nothrow]] void* memmove_p(void* dst, void const __flash* src, size_t sz);

#endif

#ifdef __cplusplus
}
#endif