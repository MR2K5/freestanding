#pragma once

#include <__memory/allocator.hpp>
#include <__memory/base.hpp>

namespace std::__detail {

template<class T, class Allocator> struct __temp_value {
    using Atr = allocator_traits<Allocator>;

    Allocator& alloc_;
    union {
        T value_;
    };
    // bool has_value_ = false;

    template<class... Args>
    constexpr explicit __temp_value(Allocator& a, Args&&... args): alloc_(a) {
        Atr::construct(alloc_, std::addressof(value_), FWD(args)...);
        // has_value_ = true;
    }

    constexpr ~__temp_value() {
        // if (has_value_)
        Atr::destroy(alloc_, std::addressof(value_));
    }

    // Move and copy are disabled; this is purely a scoped RAII frame
    __temp_value(__temp_value const&)            = delete;
    __temp_value& operator=(__temp_value const&) = delete;

    [[nodiscard]] constexpr T& get() noexcept { return value_; }
    [[nodiscard]] constexpr T const& get() const noexcept { return value_; }
};

}  // namespace std::__detail
