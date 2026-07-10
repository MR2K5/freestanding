#include <cstddef>
// #include <cstring> // Not included

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
static void do_memcpy(To* __restrict to, From* __restrict from, size_t n) {
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

template<class To> static void do_memset(To* to, int c, size_t n) {
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

template<class From, class To> static void do_memmove(To* to, From const* from, size_t n) {
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

byte* memcpy(byte* __restrict to, byte const* __restrict from, size_t sz) noexcept {
    std::do_memcpy(to, from, sz);
    return to;
}

byte* memmove(byte* to, byte const* from, size_t sz) noexcept {
    std::do_memmove(to, from, sz);
    return to;
}

byte* memset(byte* to, int c, size_t sz) noexcept {
    std::do_memset(to, c, sz);
    return to;
}

byte* memcpy_p(byte* __restrict to, byte const __flash* __restrict from, size_t sz) noexcept {
    std::do_memcpy(to, from, sz);
    return to;
}

byte* memmove_p(byte* to, byte const __flash* from, size_t sz) noexcept {
    std::do_memmove(to, from, sz);
    return to;
}
}
