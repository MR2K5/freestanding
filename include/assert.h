#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
#endif
    [[noreturn]] void __assert_fail(char const* file, char const* expr, char const* func, int line);

#ifdef NDEBUG
#  define assert(...) ((void)0)
#else

#  define assert(...)                                                                              \
      ((bool)(__VA_ARGS__) ? ((void)0)                                                                  \
                         : __assert_fail(__FILE__, #__VA_ARGS__, __PRETTY_FUNCTION__, __LINE__))
#endif
