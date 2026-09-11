#include <cstddef>
// #include <cstring> // Not included

#include <__algorithm/copy_move_fill_find.hpp>
#include <string.h>

namespace std {

template<class From, class To> struct copy_addr_space {
    using type = To;
};

template<class T, class U, size_t N> struct copy_addr_space<T [[clang::address_space(N)]], U> {
    using type = U [[clang::address_space(N)]];
};

template<class T, class U> inline constexpr bool in_same_addr_space = true;
template<class T, class U, size_t N, size_t M>
inline constexpr bool
    in_same_addr_space<T [[clang::address_space(N)]], U [[clang::address_space(M)]]> = true;

template<class> inline constexpr size_t get_address_space = 0;
template<class T, size_t N>
inline constexpr size_t get_address_space<T [[clang::address_space(N)]]> = N;

template<class From, class To> using copy_addr_space_t = copy_addr_space<From, To>::type;

template<class From, class To>
static void do_memcpy(To* __restrict to, From* __restrict from, size_t n) noexcept {
    auto d = to;
    auto s = from;

    while (n >= 8) {
        __builtin_memcpy_inline(d, s, 8);
        n -= 8;
        d += 8;
        s += 8;
    }

    if (n & 4) {
        __builtin_memcpy_inline(d, s, 4);
        d += 4;
        s += 4;
    }
    if (n & 2) {
        __builtin_memcpy_inline(d, s, 2);
        d += 2;
        s += 2;
    }
    if (n & 1) { __builtin_memcpy_inline(d, s, 1); }
}

template<class To> static void do_memset(To* to, int c, size_t n) noexcept {
    auto d = reinterpret_cast<copy_addr_space_t<To, byte>*>(to);

    while (n >= 8) {
        __builtin_memset_inline(d, c, 8);
        n -= 8;
        d += 8;
    }

    if (n & 4) {
        __builtin_memset_inline(d, c, 4);
        d += 4;
    }
    if (n & 2) {
        __builtin_memset_inline(d, c, 2);
        d += 2;
    }
    if (n & 1) { __builtin_memset_inline(d, c, 1); }
}

template<class From, class To> static void do_memmove(To* to, From const* from, size_t n) noexcept {
    auto d = reinterpret_cast<copy_addr_space_t<To, byte>*>(to);
    auto s = reinterpret_cast<copy_addr_space_t<From, byte const>*>(from);

    if constexpr (get_address_space<To> == get_address_space<From>) {
        if (d == s || n == 0) { return; }

        if (d < s) {
            // Destination is before source: Safe to copy forwards
            while (n--) { *d++ = *s++; }
        } else {
            // Destination is after source: MUST copy backwards
            d += n;
            s += n;
            while (n--) { *(--d) = *(--s); }
        }
    } else {
        do_memcpy(to, from, n);
    }
}

}  // namespace std

using std::byte;

extern "C" {

void* memcpy(void* __restrict to, void const* __restrict from, size_t sz) noexcept {
    std::do_memcpy((byte*)to, (byte const*)from, sz);
    return to;
}

void* memmove(void* to, void const* from, size_t sz) noexcept {
    std::do_memmove((byte*)to, (byte const*)from, sz);
    return to;
}

void* memset(void* to, int c, size_t sz) noexcept {
    std::do_memset((byte*)to, c, sz);
    return to;
}

byte* memcpy_p(
    byte* __restrict to, byte const [[clang::address_space(1)]]* __restrict from, size_t sz
) noexcept {
    std::do_memcpy(to, from, sz);
    return to;
}

byte* memmove_p(byte* to, byte const [[clang::address_space(1)]] * from, size_t sz) noexcept {
    std::do_memmove(to, from, sz);
    return to;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const auto* p1 = static_cast<const unsigned char*>(s1);
    const auto* p2 = static_cast<const unsigned char*>(s2);

    for (size_t i = 0; i < n; ++i) {
        if (p1[i] != p2[i]) {
            return static_cast<int>(p1[i]) - static_cast<int>(p2[i]);
        }
    }

    return 0;
}

void* memccpy(void* __restrict dest, void const* __restrict src, int c, size_t count) noexcept {
    auto d = reinterpret_cast<byte*>(dest);
    auto s = reinterpret_cast<byte const*>(src);
    for (; count != 0; --count) {
        *d++ = *s++;
        if (d[-1] == byte(c)) return d;
    }
    return nullptr;
}

char* strcpy(char* __restrict dest, char const* __restrict src) noexcept {
    auto d = dest;
    while (*src != '\0') *dest++ = *src++;
    *dest++ = *src++;  // Copy null
    return d;
}

char* strncpy(char* __restrict s1, char const* __restrict s2, size_t n) noexcept {
    auto d = s1;
    while (n != 0 && *s2 != '\0') {
        --n;
        *s1++ = *s2++;
    }
    for (; n != 0; --n) *s1++ = '\0';
    return d;
}

char* strcat(char* __restrict dest, char const* __restrict src) noexcept {
    auto len = strlen(dest);
    strcpy(dest + len, src);
    return dest;
}

char* strncat(char* __restrict dest, char const* __restrict src, size_t count) noexcept {
    char* d = dest + strlen(dest);
    while (count > 0 && *src != '\0') {
        *d++ = *src++;
        --count;
    }
    *d = '\0';
    return dest;
}

size_t strlen(char const* str) noexcept {
    size_t res = 0;
    for (; *str != '\0'; ++str) ++res;
    return res;
}

int strcmp(char const* lhs, char const* rhs) noexcept {
    unsigned char const* p1 = (unsigned char const*)lhs;
    unsigned char const* p2 = (unsigned char const*)rhs;

    while (*p1 && (*p1 == *p2)) {
        p1++;
        p2++;
    }

    return (int)*p1 - (int)*p2;
}

int strncmp(char const* s1, char const* s2, size_t n) noexcept {
    unsigned char const* p1 = (unsigned char const*)s1;
    unsigned char const* p2 = (unsigned char const*)s2;

    while (n > 0 && *p1 && (*p1 == *p2)) {
        p1++;
        p2++;
        n--;
    }

    if (n == 0) { return 0; }

    return (int)*p1 - (int)*p2;
}

char* strchr(char const* str, int ch) noexcept {
    char c = static_cast<char>(ch);
    for (; *str != '\0'; ++str) {
        if (*str == c) { return const_cast<char*>(str); }
    }
    return (c == '\0') ? const_cast<char*>(str) : nullptr;
}

char* strrchr(char const* str, int ch) noexcept {
    char c          = static_cast<char>(ch);
    char const* end = str + strlen(str);

    while (end >= str) {
        if (*end == c) { return const_cast<char*>(end); }
        --end;
    }
    return nullptr;
}

size_t strspn(char const* s, char const* accept) noexcept {
    char const* p = s;

    for (; *p; p++) {
        char const* a = accept;
        while (*a && *a != *p) { a++; }
        // If we reached the end of accept, *p was not found
        if (*a == '\0') { break; }
    }

    return (size_t)(p - s);
}

size_t strcspn(char const* s, char const* reject) noexcept {
    char const* p = s;

    for (; *p; p++) {
        for (char const* r = reject; *r; r++) {
            // Stop at the first character that matches reject
            if (*p == *r) { return (size_t)(p - s); }
        }
    }
    return (size_t)(p - s);
}

char* strpbrk(char const* s, char const* accept) noexcept {
    for (; *s != '\0'; s++) {
        for (char const* a = accept; *a != '\0'; a++) {
            if (*s == *a) { return (char*)s; }
        }
    }
    return NULL;
}

char* strstr(char const* haystack, char const* needle) noexcept {
    if (*needle == '\0') { return (char*)haystack; }

    for (; *haystack != '\0'; haystack++) {
        char const* h = haystack;
        char const* n = needle;

        while (*h != '\0' && *n != '\0' && *h == *n) {
            h++;
            n++;
        }

        if (*n == '\0') { return (char*)haystack; }
    }

    return NULL;
}

char* strtok_r(char* str, char const* delim, char** saveptr) noexcept {
    if (str == NULL) { str = *saveptr; }

    if (str == NULL) { return NULL; }

    // 1. Skip leading delimiters
    while (*str != '\0') {
        int is_delim = 0;
        for (char const* d = delim; *d != '\0'; d++) {
            if (*str == *d) {
                is_delim = 1;
                break;
            }
        }
        if (!is_delim) { break; }
        str++;
    }

    // If string contained only delimiters
    if (*str == '\0') {
        *saveptr = NULL;
        return NULL;
    }

    char* token_start = str;

    // 2. Find the end of the token
    while (*str != '\0') {
        for (char const* d = delim; *d != '\0'; d++) {
            if (*str == *d) {
                *str     = '\0';     // Terminate the token
                *saveptr = str + 1;  // Save position for next call
                return token_start;
            }
        }
        str++;
    }

    // End of string reached without trailing delimiter
    *saveptr = NULL;
    return token_start;
}

char* strtok(char* str, char const* delim) noexcept {
    static char* last_pos = NULL;
    return strtok_r(str, delim, &last_pos);
}

}  // extern "C"
