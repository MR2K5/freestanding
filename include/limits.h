#pragma once

#define CHAR_BIT            __CHAR_BIT__

#ifdef __MB_LEN_MAX__
    #define MB_LEN_MAX      __MB_LEN_MAX__
#else
    #define MB_LEN_MAX      1 // Standard freestanding fallback for pure ASCII/Basic execution sets
#endif

#define BOOL_WIDTH          __BOOL_WIDTH__
#define CHAR_WIDTH          __CHAR_WIDTH__

#define SCHAR_WIDTH         __SCHAR_WIDTH__
#define SHRT_WIDTH          __SHRT_WIDTH__
#define INT_WIDTH           __INT_WIDTH__
#define LONG_WIDTH          __LONG_WIDTH__
#define LLONG_WIDTH         __LONG_LONG_WIDTH__

#define UCHAR_WIDTH         __SCHAR_WIDTH__
#define USHRT_WIDTH         __SHRT_WIDTH__
#define UINT_WIDTH          __INT_WIDTH__
#define ULONG_WIDTH         __LONG_WIDTH__
#define ULLONG_WIDTH        __LONG_LONG_WIDTH__

#define SCHAR_MAX           __SCHAR_MAX__
#define SCHAR_MIN           (-__SCHAR_MAX__ - 1)

#define SHRT_MAX            __SHRT_MAX__
#define SHRT_MIN            (-__SHRT_MAX__ - 1)

#define INT_MAX             __INT_MAX__
#define INT_MIN             (-__INT_MAX__ - 1)

#define LONG_MAX            __LONG_MAX__
#define LONG_MIN            (-__LONG_MAX__ - 1)

#define LLONG_MAX           __LONG_LONG_MAX__
#define LLONG_MIN           (-__LONG_LONG_MAX__ - 1)

#define UCHAR_MAX           (unsigned char)(-1)
#define USHRT_MAX           (unsigned short)(-1)
#define UINT_MAX            (unsigned)(-1)
#define ULONG_MAX           (unsigned long)(-1)
#define ULLONG_MAX          (unsigned long long)(-1)

#ifdef __CHAR_UNSIGNED__
    #define CHAR_MIN        0
    #define CHAR_MAX        (__SCHAR_MAX__ * 2 + 1)
#else
    #define CHAR_MIN        (-__SCHAR_MAX__ - 1)
    #define CHAR_MAX        __SCHAR_MAX__
#endif