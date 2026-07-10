#pragma once
// code: language=c++

#include <__iterator/concepts.hpp>
#include <__memory/base.hpp>

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace std::__detail {

template<class T, class Base = void> class __pointer_wrapper {
    T* ptr_ = nullptr;
    template<class U, class B> friend class __pointer_wrapper;

public:
    using value_type        = remove_cv_t<T>;
    using difference_type   = ptrdiff_t;
    using reference         = T&;
    using pointer           = T*;
    using iterator_category = random_access_iterator_tag;
    using iterator_concept  = contiguous_iterator_tag;

    constexpr __pointer_wrapper()                           = default;
    constexpr __pointer_wrapper(__pointer_wrapper const& o) = default;

    template<class U> requires convertible_to<U*, T*>
    constexpr __pointer_wrapper(__pointer_wrapper<U, Base> const& o) noexcept: ptr_(o.ptr_) {}
    constexpr __pointer_wrapper(T* ptr) noexcept: ptr_(ptr) {}

    constexpr T* base() const noexcept { return ptr_; }
    constexpr reference operator[](integral auto n) const noexcept { return ptr_[n]; }
    constexpr reference operator*() const noexcept { return *ptr_; }
    constexpr pointer operator->() const noexcept { return ptr_; }

    constexpr __pointer_wrapper& operator++() noexcept {
        ++ptr_;
        return *this;
    }
    constexpr __pointer_wrapper& operator--() noexcept {
        --ptr_;
        return *this;
    }
    constexpr __pointer_wrapper operator++(int) noexcept {
        auto tmp = *this;
        ++ptr_;
        return tmp;
    }
    constexpr __pointer_wrapper operator--(int) noexcept {
        auto tmp = *this;
        --ptr_;
        return tmp;
    }
    constexpr __pointer_wrapper& operator+=(integral auto n) noexcept {
        ptr_ += n;
        return *this;
    }
    constexpr __pointer_wrapper& operator-=(integral auto n) noexcept {
        ptr_ -= n;
        return *this;
    }

    friend constexpr bool operator==(__pointer_wrapper x, __pointer_wrapper y) noexcept  = default;
    friend constexpr auto operator<=>(__pointer_wrapper x, __pointer_wrapper y) noexcept = default;

    friend constexpr __pointer_wrapper operator+(__pointer_wrapper x, integral auto n) noexcept {
        return x += n;
    }
    friend constexpr __pointer_wrapper operator+(integral auto n, __pointer_wrapper x) noexcept {
        return x += n;
    }
    friend constexpr __pointer_wrapper operator-(__pointer_wrapper x, integral auto n) noexcept {
        return x -= n;
    }
    friend constexpr difference_type operator-(__pointer_wrapper x, __pointer_wrapper y) noexcept {
        return x.ptr_ - y.ptr_;
    }

    friend constexpr T&& iter_move(__pointer_wrapper const& x) noexcept { return std::move(*x); }
};

}  // namespace std::__detail
