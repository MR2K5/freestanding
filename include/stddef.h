#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define NULL             __null
#define offsetof(T, mem) __builtin_offsetof(T, mem)

typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;

#ifdef __cplusplus
typedef decltype(nullptr) nullptr_t;
#else
typedef typeof(__nullptr) nullptr_t;
#endif

typedef struct {
    _Alignas(__BIGGEST_ALIGNMENT__) char _;
} max_align_t;

#ifdef __cplusplus
}
#endif
