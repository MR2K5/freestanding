#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__cplusplus)
  #if __cplusplus >= 201103L
    #define NULL nullptr
  #else
    #define NULL __null
  #endif
#else
  #define NULL ((void *)0)
#endif

#define offsetof(T, mem) __builtin_offsetof(T, mem)

typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;

#if defined(__cplusplus) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L)
typedef __typeof(nullptr) nullptr_t;
#endif

typedef struct {
    _Alignas(__BIGGEST_ALIGNMENT__) char _;
} max_align_t;

#ifdef __cplusplus
}
#endif
