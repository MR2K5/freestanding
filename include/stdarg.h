#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef __builtin_va_list va_list;

#define va_start(ap, ...)  __builtin_c23_va_start(ap, __VA_OPT__(, ) __VA_ARGS__)
#define va_arg(ap, T)      __builtin_va_arg(ap, T)
#define va_end(ap)         __builtin_va_end(ap)
#define va_copy(dest, src) __builtin_va_copy(dest, src)

#ifdef __cplusplus
}
#endif
