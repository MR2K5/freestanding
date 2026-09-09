#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef __INT8_TYPE__ int8_t;
typedef __INT16_TYPE__ int16_t;
typedef __INT32_TYPE__ int32_t;
typedef __INT64_TYPE__ int64_t;
typedef __UINT8_TYPE__ uint8_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;

typedef __INT_FAST8_TYPE__ int_fast8_t;
typedef __INT_FAST16_TYPE__ int_fast16_t;
typedef __INT_FAST32_TYPE__ int_fast32_t;
typedef __INT_FAST64_TYPE__ int_fast64_t;
typedef __UINT_FAST8_TYPE__ uint_fast8_t;
typedef __UINT_FAST16_TYPE__ uint_fast16_t;
typedef __UINT_FAST32_TYPE__ uint_fast32_t;
typedef __UINT_FAST64_TYPE__ uint_fast64_t;

typedef __INT_LEAST8_TYPE__ int_least8_t;
typedef __INT_LEAST16_TYPE__ int_least16_t;
typedef __INT_LEAST32_TYPE__ int_least32_t;
typedef __INT_LEAST64_TYPE__ int_least64_t;
typedef __UINT_LEAST8_TYPE__ uint_least8_t;
typedef __UINT_LEAST16_TYPE__ uint_least16_t;
typedef __UINT_LEAST32_TYPE__ uint_least32_t;
typedef __UINT_LEAST64_TYPE__ uint_least64_t;

typedef __INTMAX_TYPE__ intmax_t;
typedef __INTPTR_TYPE__ intptr_t;
typedef __UINTMAX_TYPE__ uintmax_t;
typedef __UINTPTR_TYPE__ uintptr_t;

#define INT8_WIDTH  __INT8_WIDTH__
#define INT16_WIDTH __INT16_WIDTH__
#define INT32_WIDTH __INT32_WIDTH__
#define INT64_WIDTH __INT64_WIDTH__

#define UINT8_WIDTH  __UINT8_WIDTH__
#define UINT16_WIDTH __UINT16_WIDTH__
#define UINT32_WIDTH __UINT32_WIDTH__
#define UINT64_WIDTH __UINT64_WIDTH__

#define INT_LEAST8_WIDTH  __INT_LEAST8_WIDTH__
#define INT_LEAST16_WIDTH __INT_LEAST16_WIDTH__
#define INT_LEAST32_WIDTH __INT_LEAST32_WIDTH__
#define INT_LEAST64_WIDTH __INT_LEAST64_WIDTH__

#define UINT_LEAST8_WIDTH  __UINT_LEAST8_WIDTH__
#define UINT_LEAST16_WIDTH __UINT_LEAST16_WIDTH__
#define UINT_LEAST32_WIDTH __UINT_LEAST32_WIDTH__
#define UINT_LEAST64_WIDTH __UINT_LEAST64_WIDTH__

#define INT_FAST8_WIDTH  __INT_FAST8_WIDTH__
#define INT_FAST16_WIDTH __INT_FAST16_WIDTH__
#define INT_FAST32_WIDTH __INT_FAST32_WIDTH__
#define INT_FAST64_WIDTH __INT_FAST64_WIDTH__

#define UINT_FAST8_WIDTH  __UINT_FAST8_WIDTH__
#define UINT_FAST16_WIDTH __UINT_FAST16_WIDTH__
#define UINT_FAST32_WIDTH __UINT_FAST32_WIDTH__
#define UINT_FAST64_WIDTH __UINT_FAST64_WIDTH__

#define INTPTR_WIDTH  __INTPTR_WIDTH__
#define UINTPTR_WIDTH __UINTPTR_WIDTH__
#define INTMAX_WIDTH  __INTMAX_WIDTH__
#define UINTMAX_WIDTH __UINTMAX_WIDTH__

#define SIZE_WIDTH       __SIZE_WIDTH__
#define PTRDIFF_WIDTH    __PTRDIFF_WIDTH__
#define SIG_ATOMIC_WIDTH __SIG_ATOMIC_WIDTH__
#define WCHAR_WIDTH      __WCHAR_WIDTH__
#define WINT_WIDTH       __WINT_WIDTH__

// ============================================================================
// 3. Limits Limits Limits (Min/Max Boundary Expressions)
// ============================================================================
#define INT8_MAX  __INT8_MAX__
#define INT16_MAX __INT16_MAX__
#define INT32_MAX __INT32_MAX__
#define INT64_MAX __INT64_MAX__

#define INT8_MIN  (-__INT8_MAX__ - 1)
#define INT16_MIN (-__INT16_MAX__ - 1)
#define INT32_MIN (-__INT32_MAX__ - 1)
#define INT64_MIN (-__INT64_MAX__ - 1)

#define UINT8_MAX  __UINT8_MAX__
#define UINT16_MAX __UINT16_MAX__
#define UINT32_MAX __UINT32_MAX__
#define UINT64_MAX __UINT64_MAX__

#define INT_LEAST8_MAX  __INT_LEAST8_MAX__
#define INT_LEAST16_MAX __INT_LEAST16_MAX__
#define INT_LEAST32_MAX __INT_LEAST32_MAX__
#define INT_LEAST64_MAX __INT_LEAST64_MAX__

#define INT_LEAST8_MIN  (-__INT_LEAST8_MAX__ - 1)
#define INT_LEAST16_MIN (-__INT_LEAST16_MAX__ - 1)
#define INT_LEAST32_MIN (-__INT_LEAST32_MAX__ - 1)
#define INT_LEAST64_MIN (-__INT_LEAST64_MAX__ - 1)

#define UINT_LEAST8_MAX  __UINT_LEAST8_MAX__
#define UINT_LEAST16_MAX __UINT_LEAST16_MAX__
#define UINT_LEAST32_MAX __UINT_LEAST32_MAX__
#define UINT_LEAST64_MAX __UINT_LEAST64_MAX__

#define INT_FAST8_MAX  __INT_FAST8_MAX__
#define INT_FAST16_MAX __INT_FAST16_MAX__
#define INT_FAST32_MAX __INT_FAST32_MAX__
#define INT_FAST64_MAX __INT_FAST64_MAX__

#define INT_FAST8_MIN  (-__INT_FAST8_MAX__ - 1)
#define INT_FAST16_MIN (-__INT_FAST16_MAX__ - 1)
#define INT_FAST32_MIN (-__INT_FAST32_MAX__ - 1)
#define INT_FAST64_MIN (-__INT_FAST64_MAX__ - 1)

#define UINT_FAST8_MAX  __UINT_FAST8_MAX__
#define UINT_FAST16_MAX __UINT_FAST16_MAX__
#define UINT_FAST32_MAX __UINT_FAST32_MAX__
#define UINT_FAST64_MAX __UINT_FAST64_MAX__

#define INTPTR_MAX  __INTPTR_MAX__
#define INTPTR_MIN  (-__INTPTR_MAX__ - 1)
#define UINTPTR_MAX __UINTPTR_MAX__

#define INTMAX_MAX  __INTMAX_MAX__
#define INTMAX_MIN  (-__INTMAX_MAX__ - 1)
#define UINTMAX_MAX __UINTMAX_MAX__

#define SIZE_MAX    __SIZE_MAX__
#define PTRDIFF_MAX __PTRDIFF_MAX__
#define PTRDIFF_MIN (-__PTRDIFF_MAX__ - 1)

#define SIG_ATOMIC_MAX __SIG_ATOMIC_MAX__
#define SIG_ATOMIC_MIN __SIG_ATOMIC_MIN__

#define WCHAR_MAX __WCHAR_MAX__
#ifdef __WCHAR_UNSIGNED__
#  define WCHAR_MIN 0
#else
#  define WCHAR_MIN (-__WCHAR_MAX__ - 1)
#endif

#define WINT_MAX __WINT_MAX__
#ifdef __WINT_UNSIGNED__
#  define WINT_MIN 0
#else
#  define WINT_MIN (-__WINT_MAX__ - 1)
#endif


// ============================================================================
// 4. Function Macros for Literal Constants (Target Suffix Resolution Engine)
// ============================================================================
// Modern architectures use varying native sizes for int, long, and long long.
// This logic checks widths dynamically to append the proper L, LL, U, or ULL
// suffixes without hardcoding assumptions.

#define INT8_C(c)  c
#define INT16_C(c) c

#if __INT32_WIDTH__ <= __INT_WIDTH__
#  define INT32_C(c)  c
#  define UINT32_C(c) c##U
#elif __INT32_WIDTH__ <= __LONG_WIDTH__
#  define INT32_C(c)  c##L
#  define UINT32_C(c) c##UL
#else
#  define INT32_C(c)  c##LL
#  define UINT32_C(c) c##ULL
#endif

#if __INT64_WIDTH__ <= __LONG_WIDTH__
#  define INT64_C(c)  c##L
#  define UINT64_C(c) c##UL
#else
#  define INT64_C(c)  c##LL
#  define UINT64_C(c) c##ULL
#endif

#if __INTMAX_WIDTH__ == __LONG_LONG_WIDTH__
#  define INTMAX_C(c)  c##LL
#  define UINTMAX_C(c) c##ULL
#else
#  define INTMAX_C(c)  c##L
#  define UINTMAX_C(c) c##UL
#endif

#ifdef __cplusplus
}
#endif
