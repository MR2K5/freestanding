#pragma once
// code: language=c++
// IWYU pragma: private: include <ranges>

#include <__ranges/core.hpp>
#include <concepts>
#include <cstddef>

namespace std::ranges {

template<view V> requires input_range<V> class as_rvalue_view: public view_interface<as_rvalue_view<V>> {
    V base_{};

public:
    as_rvalue_view() requires default_initializable<V> = default;
    constexpr explicit as_rvalue_view(V base): base_(std::move(base)) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }
    constexpr auto begin() requires(!__detail::__simple_view<V>) {
        return move_iterator(ranges::begin(base_));
    }
    constexpr auto begin() const requires range<V const> {
        return move_iterator(ranges::begin(base_));
    }
    constexpr auto end() requires(!__detail::__simple_view<V>) {
        if constexpr (common_range<V>)
            return move_iterator(ranges::end(base_));
        else
            return move_sentinel(ranges::end(base_));
    }
    constexpr auto end() const requires range<V const> {
        if constexpr (common_range<V>)
            return move_iterator(ranges::end(base_));
        else
            return move_sentinel(ranges::end(base_));
    }
    constexpr auto size() requires sized_range<V> { return ranges::size(base_); }
    constexpr auto size() const requires sized_range<V const> { return ranges::size(base_); }
};

template<class R> as_rvalue_view(R&&) -> as_rvalue_view<views::all_t<R>>;
template<class T>
inline constexpr bool enable_borrowed_range<as_rvalue_view<T>> = enable_borrowed_range<T>;

namespace views {

template<class R>
concept __as_rvalue_req = requires { views::all(declval<R>()); } && input_range<R&>
                       && same_as<range_rvalue_reference_t<R&>, range_reference_t<R&>>;
inline constexpr struct __as_rvalue_fn: range_adaptor_closure<__as_rvalue_fn> {
    template<viewable_range R> requires __as_rvalue_req<R>
    static constexpr view auto operator()(R&& r) {
        return views::all(std::forward<R>(r));
    }

    template<viewable_range R> requires(!__as_rvalue_req<R>) && requires {
        as_rvalue_view(declval<R>());
    } static constexpr view auto operator()(R&& r) {
        return as_rvalue_view{std::forward<R>(r)};
    }
} as_rvalue;
}  // namespace views

}  // namespace std::ranges
