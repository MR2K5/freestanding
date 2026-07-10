#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

[[noreturn, gnu::nothrow]] void abort();
[[gnu::nothrow]] int atexit(void (*func)());
[[gnu::nothrow]] int at_quick_exit(void (*func)());
[[gnu::nothrow]] [[noreturn]] void exit(int status);
[[gnu::nothrow]] [[noreturn]] void _Exit(int status);
[[gnu::nothrow]] [[noreturn]] void quick_exit(int status);

[[noreturn, gnu::nothrow]] void _exit();

size_t memalignment(void const* p);

void __libc_init_array();

#ifdef __cplusplus
}
#endif
