#pragma once

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace std {

// -----------------------------------------------------------------------------
// 1. Primary Template (Disabled Hash)
// -----------------------------------------------------------------------------

template<class> struct hash {
    hash()                       = delete;
    ~hash()                      = delete;
    hash(hash const&)            = delete;
    hash& operator=(hash const&) = delete;
};

namespace __detail {

[[nodiscard]] constexpr uint16_t mix16(uint16_t x) noexcept {
    x ^= x >> 8;
    x *= 0x9E37U;
    x ^= x >> 7;
    return x;
}

[[nodiscard]] constexpr uint32_t mix32(uint32_t x) noexcept {
    x ^= x >> 16;
    x *= 0x85ebca6bU;
    x ^= x >> 13;
    x *= 0xc2b2ae35U;
    x ^= x >> 16;
    return x;
}

[[nodiscard]] constexpr uint64_t mix64(uint64_t x) noexcept {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

// -----------------------------------------------------------------------------
// 2. Universal Scalar Hash Dispatcher (Integers, Bools, Characters)
// -----------------------------------------------------------------------------

template<typename T> [[nodiscard]] constexpr size_t hash_scalar(T val) noexcept {
    using U         = std::conditional_t<std::is_same_v<T, bool>, uint8_t, std::make_unsigned_t<T>>;
    auto const uval = static_cast<U>(val);

    if constexpr (sizeof(size_t) == 8) {
        return static_cast<size_t>(mix64(static_cast<uint64_t>(uval)));
    } else if constexpr (sizeof(size_t) == 4) {
        if constexpr (sizeof(U) <= 4) {
            return static_cast<size_t>(mix32(static_cast<uint32_t>(uval)));
        } else {
            uint64_t const m = mix64(static_cast<uint64_t>(uval));
            return static_cast<size_t>(m ^ (m >> 32));
        }
    } else {
        if constexpr (sizeof(U) <= 2) {
            return static_cast<size_t>(mix16(static_cast<uint16_t>(uval)));
        } else if constexpr (sizeof(U) == 4) {
            uint32_t const m = mix32(static_cast<uint32_t>(uval));
            return static_cast<size_t>(m ^ (m >> 16));
        } else {
            uint64_t const m   = mix64(static_cast<uint64_t>(uval));
            uint32_t const m32 = static_cast<uint32_t>(m ^ (m >> 32));
            return static_cast<size_t>(m32 ^ (m32 >> 16));
        }
    }
}

// -----------------------------------------------------------------------------
// 3. Hash Combine
// -----------------------------------------------------------------------------

inline constexpr void hash_combine(size_t& seed, size_t next) noexcept {
    if constexpr (sizeof(size_t) == 8) {
        constexpr uint64_t m  = 0x9ddfea08eb382d69ULL;
        uint64_t a            = (next ^ seed) * m;
        a                    ^= (a >> 47);
        seed                  = (seed ^ a) * m;
    } else if constexpr (sizeof(size_t) == 4) {
        constexpr uint32_t m  = 0xcc9e2d51U;
        uint32_t a            = (next ^ seed) * m;
        a                    ^= (a >> 15);
        seed                  = (seed ^ a) * m;
    } else {
        constexpr uint16_t phi  = 0x9E37U;
        seed                   ^= next + phi + (seed << 3) + (seed >> 2);
    }
}

// -----------------------------------------------------------------------------
// 4. Float Normalization
// -----------------------------------------------------------------------------

template<std::floating_point T> [[nodiscard]] constexpr size_t hash_float(T val) noexcept {
    if (val == static_cast<T>(0)) { return 0; }
    if (__builtin_isnan(val)) { return hash_scalar(0x7FC00000U); }

    if constexpr (sizeof(T) == sizeof(uint32_t)) {
        return hash_scalar(std::bit_cast<uint32_t>(val));
    } else if constexpr (sizeof(T) == sizeof(uint64_t)) {
        return hash_scalar(std::bit_cast<uint64_t>(val));
    } else {
        size_t seed       = 0;
        auto const* bytes = reinterpret_cast<uint8_t const*>(&val);
        for (size_t i = 0; i < sizeof(T); ++i) { hash_combine(seed, hash_scalar(bytes[i])); }
        return seed;
    }
}

// -----------------------------------------------------------------------------
// 5. String / Buffer Hashing (Robust FNV-1a / Rapidhash)
// -----------------------------------------------------------------------------

template<typename SizeType = size_t> struct fnv1a_traits;

template<> struct fnv1a_traits<uint16_t> {
    static constexpr uint16_t offset_basis = 0x811cU;
    static constexpr uint16_t prime        = 0x0103U;
};

template<> struct fnv1a_traits<uint32_t> {
    static constexpr uint32_t offset_basis = 0x811c9dc5U;
    static constexpr uint32_t prime        = 0x01000193U;
};

template<> struct fnv1a_traits<uint64_t> {
    static constexpr uint64_t offset_basis = 0xcbf29ce484222325ULL;
    static constexpr uint64_t prime        = 0x00000100000001b3ULL;
};

[[nodiscard]] constexpr size_t fnv1a(void const* data, size_t count) noexcept {
    using traits  = fnv1a_traits<size_t>;
    auto const* p = static_cast<uint8_t const*>(data);
    size_t hash   = traits::offset_basis;
    for (size_t i = 0; i < count; ++i) {
        hash ^= static_cast<size_t>(p[i]);
        hash *= traits::prime;
    }
    return hash;
}

[[nodiscard]] inline size_t hash_bytes(void const* data, size_t len) noexcept {
#if defined(__SIZEOF_INT128__) && (defined(__x86_64__) || defined(__aarch64__))
    uint8_t const* p             = static_cast<uint8_t const*>(data);
    constexpr uint64_t secret[3] = {
        0x2d358dccaa6c78a5ULL, 0x8bb136861a9f559fULL, 0x9b9031c2a6ef8347ULL
    };

    uint64_t seed = secret[0] ^ len;
    uint64_t a = 0, b = 0;

    if (len <= 16) {
        if (len >= 8) {
            // Correctly load full 8..16 bytes via overlapping reads
            __builtin_memcpy(&a, p, 8);
            __builtin_memcpy(&b, p + len - 8, 8);
        } else if (len >= 4) {
            uint32_t low, high;
            __builtin_memcpy(&low, p, 4);
            __builtin_memcpy(&high, p + len - 4, 4);
            a = (static_cast<uint64_t>(low) << 32) | high;
        } else if (len > 0) {
            a = (static_cast<uint64_t>(p[0]) << 16) | (static_cast<uint64_t>(p[len >> 1]) << 8)
              | p[len - 1];
        }
    } else {
        // Stream all 16-byte blocks so no intermediate bytes are skipped
        size_t i = len;
        while (i > 16) {
            uint64_t v1, v2;
            __builtin_memcpy(&v1, p, 8);
            __builtin_memcpy(&v2, p + 8, 8);

            __uint128_t m = static_cast<__uint128_t>(v1 ^ secret[1]) * (v2 ^ seed);
            seed          = static_cast<uint64_t>(m) ^ static_cast<uint64_t>(m >> 64);

            p += 16;
            i -= 16;
        }
        // Overlapping tail read for the remaining 1..16 bytes
        __builtin_memcpy(&a, p + i - 16, 8);
        __builtin_memcpy(&b, p + i - 8, 8);
    }

    __uint128_t m = static_cast<__uint128_t>(a ^ secret[1]) * (b ^ seed);
    return static_cast<size_t>(static_cast<uint64_t>(m) ^ static_cast<uint64_t>(m >> 64));
#else
    return fnv1a(data, len);
#endif
}

template<class T> requires is_default_constructible_v<hash<T>>
constexpr size_t __do_hash(T const& t) noexcept {
    return hash<T>{}(t);
}

}  // namespace __detail

// -----------------------------------------------------------------------------
// 6. Standard Specializations
// -----------------------------------------------------------------------------

// Integral types (including bool and char types)
template<class T> requires is_integral_v<T> struct hash<T> {
    static constexpr size_t operator()(T t) noexcept { return __detail::hash_scalar(t); }
};

// Floating point types
template<class T> requires is_floating_point_v<T> struct hash<T> {
    static constexpr size_t operator()(T t) noexcept { return __detail::hash_float(t); }
};

// Enums
template<class T> requires is_enum_v<T> struct hash<T> {
    static constexpr size_t operator()(T t) noexcept {
        return __detail::__do_hash(std::to_underlying(t));
    }
};

// Raw pointers
template<class T> requires is_pointer_v<T> struct hash<T> {
    static size_t operator()(T t) noexcept {
        return __detail::hash_scalar(reinterpret_cast<uintptr_t>(t));
    }
};

// nullptr_t
template<> struct hash<nullptr_t> {
    static constexpr size_t operator()(nullptr_t) noexcept { return 0uz; }
};

template<> struct hash<monostate> {
    static constexpr size_t operator()(nullptr_t) noexcept { return size_t(66740831); }
};

}  // namespace std
