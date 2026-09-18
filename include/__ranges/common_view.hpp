#pragma once

#include <__ranges/core.hpp>
#include <__iterator/common_iterator.hpp>

namespace std::ranges {

template<view V> requires(!common_range<V> && copyable<iterator_t<V>>)
class common_view: public view_interface<common_view<V>> {
private:
    V base_ = V();

public:
    common_view() requires default_initializable<V> = default;

    constexpr explicit common_view(V r): base_(std::move(r)) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr auto begin() requires(!__detail::__simple_view<V>) {
        if constexpr (random_access_range<V> && sized_range<V>)
            return ranges::begin(base_);
        else
            return common_iterator<iterator_t<V>, sentinel_t<V>>(ranges::begin(base_));
    }

    constexpr auto begin() const requires range<V const> {
        if constexpr (random_access_range<V const> && sized_range<V const>)
            return ranges::begin(base_);
        else
            return common_iterator<iterator_t<V const>, sentinel_t<V const>>(ranges::begin(base_));
    }

    constexpr auto end() requires(!__detail::__simple_view<V>) {
        if constexpr (random_access_range<V> && sized_range<V>)
            return ranges::begin(base_) + ranges::distance(base_);
        else
            return common_iterator<iterator_t<V>, sentinel_t<V>>(ranges::end(base_));
    }

    constexpr auto end() const requires range<V const> {
        if constexpr (random_access_range<V const> && sized_range<V const>)
            return ranges::begin(base_) + ranges::distance(base_);
        else
            return common_iterator<iterator_t<V const>, sentinel_t<V const>>(ranges::end(base_));
    }

    constexpr auto size() requires sized_range<V> { return ranges::size(base_); }
    constexpr auto size() const requires sized_range<V const> { return ranges::size(base_); }

    constexpr auto reserve_hint() requires approximately_sized_range<V> {
        return ranges::reserve_hint(base_);
    }
    constexpr auto reserve_hint() const requires approximately_sized_range<V const> {
        return ranges::reserve_hint(base_);
    }
};

template<class R> common_view(R&&) -> common_view<views::all_t<R>>;

template<class T> constexpr bool enable_borrowed_range<common_view<T>> = enable_borrowed_range<T>;

namespace views {

inline constexpr struct __common_fn: range_adaptor_closure<__common_fn> {
    static constexpr auto
    operator()(auto&& e) _STD_RETURN_REQ(common_range<decltype(e)>, views::all(FWD(e)));
    static constexpr auto operator()(auto&& e) _STD_RETURN(common_view{FWD(e)});
} common;

}  // namespace views

}  // namespace std::ranges
