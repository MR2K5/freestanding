#pragma once
// code: language=c++
// IWYU pragma: private: include <ranges>

#include <__iterator/concepts.hpp>
#include <__memory/base.hpp>
#include <__ranges/core.hpp>
#include <concepts>
#include <cstddef>
#include <optional>
#include <tuple>
#include <type_traits>

namespace std::ranges {

namespace __detail {

template<class T>
concept __int_like_with_diff_type =
    __detail::__signed_integer_like<T> || (__integer_like<T> && weakly_incrementable<T>);

}

template<move_constructible W, semiregular Bound = unreachable_sentinel_t> requires(
    is_object_v<W> && same_as<W, remove_cv_t<W>>
    && (__detail::__int_like_with_diff_type<Bound> || same_as<Bound, unreachable_sentinel_t>)
) class repeat_view: public view_interface<repeat_view<W, Bound>> {
    __detail::movable_box<W> value_;
    Bound bound_ = {};

    static constexpr bool __unreachable = same_as<unreachable_sentinel_t, Bound>;

    class iterator {
        using __index_t = conditional_t<same_as<Bound, unreachable_sentinel_t>, ptrdiff_t, Bound>;
        W const* value_ = nullptr;
        __index_t cur_  = {};

    public:
        using iterator_concept  = random_access_iterator_tag;
        using iterator_category = random_access_iterator_tag;
        using value_type        = W;
        using difference_type   = conditional_t<
              __detail::__signed_integer_like<__index_t>, __index_t,
              __detail::__iota_diff_t<__index_t>>;

        iterator() = default;
        constexpr explicit iterator(W const* v, __index_t b = __index_t()): value_(v), cur_(b) {}
        constexpr W const& operator*() const noexcept { return *value_; }
        constexpr W const& operator[](difference_type n) const noexcept { return *(*this + n); }
        constexpr iterator& operator++() {
            ++cur_;
            return *this;
        }
        constexpr iterator operator++(int) {
            auto tmp = *this;
            ++*this;
            return tmp;
        }
        constexpr iterator& operator--() {
            --cur_;
            return *this;
        }
        constexpr iterator operator--(int) {
            auto tmp = *this;
            --*this;
            return tmp;
        }
        constexpr iterator& operator+=(difference_type n) {
            cur_ += n;
            return *this;
        }
        constexpr iterator& operator-=(difference_type n) {
            cur_ -= n;
            return *this;
        }
        friend constexpr bool operator==(iterator const& x, iterator const& y) {
            return x.cur_ == y.cur_;
        }
        friend constexpr auto operator<=>(iterator const& x, iterator const& y) {
            return x.cur_ <=> y.cur_;
        }
        friend constexpr iterator operator+(iterator i, difference_type n) {
            i += n;
            return i;
        }
        friend constexpr iterator operator+(difference_type n, iterator i) { return i + n; }
        friend constexpr iterator operator-(iterator i, difference_type n) {
            i -= n;
            return i;
        }
        friend constexpr difference_type operator-(iterator const& x, iterator const& y) {
            return static_cast<difference_type>(x.cur_) - static_cast<difference_type>(y.cur_);
        }
    };

public:
    repeat_view() requires default_initializable<W> = default;
    constexpr explicit repeat_view(W const& val, Bound b = Bound()): value_(val), bound_(b) {
        if constexpr (!__unreachable) assert(b >= 0);
    }
    constexpr explicit repeat_view(W&& val, Bound b = Bound()): value_(std::move(val)), bound_(b) {
        if constexpr (!__unreachable) assert(b >= 0);
    }
    template<class... WArgs, class... BoundArgs>
    requires constructible_from<W, WArgs...> && constructible_from<Bound, BoundArgs...>
    constexpr explicit repeat_view(
        piecewise_construct_t, tuple<WArgs...> value_args,
        tuple<BoundArgs...> bound_args = tuple<>{}
    )
        : value_(make_from_tuple<W>(std::move(value_args))),
          bound_(std::make_from_tuple<Bound>(std::move(bound_args))) {}

    constexpr iterator begin() const { return iterator(std::addressof(*value_)); }
    constexpr iterator end() const requires(!__unreachable) {
        return iterator(std::addressof(*value_), bound_);
    }
    constexpr unreachable_sentinel_t end() const { return unreachable_sentinel; }
    constexpr auto size() const requires(!__unreachable) {
        return __detail::__to_unsigned_like(bound_);
    }

    constexpr auto __value() const {
        return value_;
    }
};
template<class W, class Bound = unreachable_sentinel_t>
repeat_view(W, Bound = Bound()) -> repeat_view<W, Bound>;

namespace views {

inline constexpr struct __repeat_fn {
    template<class W, class Bound>
    static constexpr auto operator()(W&& val, Bound&& b)
        requires requires { repeat_view(std::forward<W>(val), std::forward<Bound>(b)); } {
        return repeat_view(std::forward<W>(val), std::forward<Bound>(b));
    }
    template<class W>
    static constexpr auto operator()(W&& val)
        requires requires { repeat_view<decay_t<decltype((val))>>(std::forward<W>(val)); } {
        return repeat_view<decay_t<decltype((val))>>(std::forward<W>(val));
    }
} repeat;

}  // namespace views

}  // namespace std::ranges
