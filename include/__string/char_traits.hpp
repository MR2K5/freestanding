#pragma once

#include <compare>

#define EOF -1

namespace std {

template<class T> struct char_traits;

template<> struct char_traits<char> {
    using char_type = char;
    using int_type  = int;
    // using off_type            = streamoff;
    // using pos_type            = streampos;
    // using state_type          = mbstate_t;
    using comparison_category = strong_ordering;

    static constexpr void assign(char_type& c1, char_type const& c2) noexcept { c1 = c2; }
    static constexpr bool eq(char_type c1, char_type c2) noexcept {
        return (unsigned char)c1 == (unsigned char)c2;
    }
    static constexpr bool lt(char_type c1, char_type c2) noexcept {
        return (unsigned char)c1 < (unsigned char)c2;
    }

    static constexpr int compare(char_type const* s1, char_type const* s2, size_t n) noexcept {
        return __builtin_memcmp(s1, s2, n);
    }
    static constexpr size_t length(char_type const* s) noexcept { return __builtin_strlen(s); }
    static constexpr char_type const*
    find(char_type const* s, size_t n, char_type const& a) noexcept {
        return __builtin_char_memchr(s, (unsigned char)(a), n);
    }
    static constexpr char_type* move(char_type* s1, char_type const* s2, size_t n) noexcept {
        __builtin_memmove(s1, s2, n);
        return s1;
    }
    static constexpr char_type*
    copy(char_type* __restrict s1, char_type const* __restrict s2, size_t n) noexcept {
        __builtin_memcpy(s1, s2, n);
        return s1;
    }
    static constexpr char_type* assign(char_type* s, size_t n, char_type a) noexcept {
        __builtin_memset(s, (unsigned char)(a), n);
        return s;
    }

    static constexpr int_type not_eof(int_type c) noexcept { return eq_int_type(c, eof()) ? 0 : c; }
    static constexpr char_type to_char_type(int_type c) noexcept {
        return (char)(unsigned char)(c);
    }
    static constexpr int_type to_int_type(char_type c) noexcept { return int((unsigned char)c); }
    static constexpr bool eq_int_type(int_type c1, int_type c2) noexcept { return c1 == c2; }
    static constexpr int_type eof() noexcept { return EOF; }
};

}  // namespace std
