#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

[[noreturn, gnu::nothrow]] void abort();
[[gnu::nothrow]] int atexit(void (*func)());
[[gnu::nothrow]] int at_quick_exit(void (*func)());
[[gnu::nothrow, noreturn]] void exit(int status);
[[gnu::nothrow, noreturn]] void _Exit(int status);
[[gnu::nothrow, noreturn]] void quick_exit(int status);

[[gnu::nothrow]] size_t memalignment(void const* p);

void __std_init_globals();

struct div_t {
    int quot;
    int rem;
};
struct ldiv_t {
    long quot;
    long rem;
};
struct lldiv_t {
    long long quot;
    long long rem;
};

[[gnu::nothrow]] int abs(int j);
[[gnu::nothrow]] long int labs(long int j);
[[gnu::nothrow]] long long int llabs(long long int j);

[[gnu::nothrow]] struct div_t div(int numer, int denom);
[[gnu::nothrow]] struct ldiv_t ldiv(long int numer, long int denom);
[[gnu::nothrow]] struct lldiv_t lldiv(long long int numer, long long int denom);

[[gnu::nothrow]] void __std_init_heap(void* ptr, size_t size);
[[gnu::nothrow]] void __std_add_heap(void* ptr, size_t size);
[[gnu::nothrow]] void free(void* ptr);
[[gnu::nothrow]] void free_sized(void* ptr, size_t size);
[[gnu::nothrow]] void free_aligned_sized(void* ptr, size_t alignment, size_t size);

[[gnu::nothrow, gnu::malloc, gnu::alloc_size(2), gnu::alloc_align(1), nodiscard]] void*
aligned_alloc(size_t alignment, size_t size);
[[gnu::nothrow, gnu::malloc, gnu::alloc_size(2, 1), nodiscard]] void*
calloc(size_t nmemb, size_t size);
[[gnu::nothrow, gnu::malloc, gnu::alloc_size(1), nodiscard]] void* malloc(size_t size);
[[gnu::nothrow, gnu::alloc_size(2), nodiscard]] void* realloc(void* ptr, size_t size);

[[clang::callback(comp, -1, -1)]] void
qsort(void* ptr, size_t count, size_t size, int (*comp)(void const*, void const*));

[[clang::callback(comp, -1, -1)]] void* bsearch(
    void const* key, void const* ptr, size_t count, size_t size,
    int (*comp)(void const*, void const*)
);

#define alloca(n)                __builtin_alloca(n)
#define aligned_alloca(align, n) __builtin_alloca_with_align(n, align)

#ifdef __cplusplus
}
#endif
