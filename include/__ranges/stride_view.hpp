#pragma once

#include <__iterator/concepts.hpp>
#include <__ranges/core.hpp>
#include <type_traits>
#include <utility>

namespace std::ranges {

template<class T> constexpr T __div_ceil(T x, T y) noexcept {
    T result = x / y;
    if (x % y > 0) { ++result; }
    return result;
}

template<class Base> struct __stride_view_iter_cat {};

template<class Base> requires forward_range<Base> struct __stride_view_iter_cat<Base> {
private:
    static consteval auto __determine_cat() {
        using C = typename std::iterator_traits<iterator_t<Base>>::iterator_category;
        if constexpr (derived_from<C, random_access_iterator_tag>) {
            return random_access_iterator_tag{};
        } else {
            return C{};
        }
    }

public:
    using iterator_category = decltype(__determine_cat());
};

// =============================================================================
// 25.7.32.2 Class template stride_view [range.stride.view]
// =============================================================================

template<input_range V> requires view<V> class stride_view: public view_interface<stride_view<V>> {
    V base_                       = V();
    range_difference_t<V> stride_ = 0;

    template<bool> class iterator;

    template<bool Const> friend class iterator;

public:
    stride_view() requires default_initializable<V> = default;

    constexpr explicit stride_view(V base, range_difference_t<V> stride)
        : base_(std::move(base)), stride_(stride) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr range_difference_t<V> stride() const noexcept { return stride_; }

    constexpr auto begin() requires(!__detail::__simple_view<V>) {
        return iterator<false>(this, ranges::begin(base_));
    }

    constexpr auto begin() const requires range<V const> {
        return iterator<true>(this, ranges::begin(base_));
    }

    constexpr auto end() requires(!__detail::__simple_view<V>) {
        if constexpr (common_range<V> && sized_range<V> && forward_range<V>) {
            auto missing = (stride_ - ranges::distance(base_) % stride_) % stride_;
            return iterator<false>(this, ranges::end(base_), missing);
        } else if constexpr (common_range<V> && !bidirectional_range<V>) {
            return iterator<false>(this, ranges::end(base_));
        } else {
            return default_sentinel;
        }
    }

    constexpr auto end() const requires range<V const> {
        if constexpr (common_range<V const> && sized_range<V const> && forward_range<V const>) {
            auto missing = (stride_ - ranges::distance(base_) % stride_) % stride_;
            return iterator<true>(this, ranges::end(base_), missing);
        } else if constexpr (common_range<V const> && !bidirectional_range<V const>) {
            return iterator<true>(this, ranges::end(base_));
        } else {
            return default_sentinel;
        }
    }

    constexpr auto size() requires sized_range<V> {
        return __detail::__to_unsigned_like(__div_ceil(ranges::distance(base_), stride_));
    }

    constexpr auto size() const requires sized_range<V const> {
        return __detail::__to_unsigned_like(__div_ceil(ranges::distance(base_), stride_));
    }

    constexpr auto reserve_hint() requires approximately_sized_range<V> {
        auto s = static_cast<range_difference_t<decltype((base_))>>(ranges::reserve_hint(base_));
        return __detail::__to_unsigned_like(__div_ceil(s, stride_));
    }

    constexpr auto reserve_hint() const requires approximately_sized_range<V const> {
        auto s = static_cast<range_difference_t<decltype((base_))>>(ranges::reserve_hint(base_));
        return __detail::__to_unsigned_like(__div_ceil(s, stride_));
    }
};

template<class R> stride_view(R&&, range_difference_t<R>) -> stride_view<views::all_t<R>>;

// =============================================================================
// 25.7.32.3 Class template stride_view::iterator [range.stride.iterator]
// =============================================================================

template<input_range V> requires view<V> template<bool Const>
class stride_view<V>::iterator: public __stride_view_iter_cat<__maybe_const<Const, V>> {
    using Parent = __maybe_const<Const, stride_view>;
    using Base   = __maybe_const<Const, V>;

    iterator_t<Base> current_         = iterator_t<Base>();
    sentinel_t<Base> end_             = sentinel_t<Base>();
    range_difference_t<Base> stride_  = 0;
    range_difference_t<Base> missing_ = 0;

    constexpr iterator(
        Parent* parent, iterator_t<Base> current, range_difference_t<Base> missing = 0
    )
        : current_(std::move(current)), end_(ranges::end(parent->base_)), stride_(parent->stride_),
          missing_(missing) {}

    friend class stride_view;
    template<bool> friend class iterator;

public:
    using difference_type  = range_difference_t<Base>;
    using value_type       = range_value_t<Base>;
    using iterator_concept = std::conditional_t<
        random_access_range<Base>, random_access_iterator_tag,
        std::conditional_t<
            bidirectional_range<Base>, bidirectional_iterator_tag,
            std::conditional_t<forward_range<Base>, forward_iterator_tag, input_iterator_tag>>>;

    iterator() requires default_initializable<iterator_t<Base>> = default;

    constexpr iterator(iterator<!Const> other)
        requires Const && convertible_to<iterator_t<V>, iterator_t<Base>>
                  && convertible_to<sentinel_t<V>, sentinel_t<Base>>
        : current_(std::move(other.current_)), end_(std::move(other.end_)), stride_(other.stride_),
          missing_(other.missing_) {}

    constexpr iterator_t<Base> base() && { return std::move(current_); }

    constexpr iterator_t<Base> const& base() const& noexcept { return current_; }

    constexpr decltype(auto) operator*() const { return *current_; }

    constexpr iterator& operator++() {
        missing_ = ranges::advance(current_, stride_, end_);
        return *this;
    }

    constexpr void operator++(int) { ++*this; }

    constexpr iterator operator++(int) requires forward_range<Base> {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr iterator& operator--() requires bidirectional_range<Base> {
        ranges::advance(current_, missing_ - stride_);
        missing_ = 0;
        return *this;
    }

    constexpr iterator operator--(int) requires bidirectional_range<Base> {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    constexpr iterator& operator+=(difference_type n) requires random_access_range<Base> {
        if (n > 0) {
            ranges::advance(current_, stride_ * (n - 1));
            missing_ = ranges::advance(current_, stride_, end_);
        } else if (n < 0) {
            ranges::advance(current_, stride_ * n + missing_);
            missing_ = 0;
        }
        return *this;
    }

    constexpr iterator& operator-=(difference_type n) requires random_access_range<Base> {
        return *this += -n;
    }

    constexpr decltype(auto) operator[](difference_type n) const requires random_access_range<Base>
    {
        return *(*this + n);
    }

    friend constexpr bool operator==(iterator const& x, default_sentinel_t) {
        return x.current_ == x.end_;
    }

    friend constexpr bool operator==(iterator const& x, iterator const& y)
        requires equality_comparable<iterator_t<Base>> {
        return x.current_ == y.current_;
    }

    friend constexpr bool operator<(iterator const& x, iterator const& y)
        requires random_access_range<Base> {
        return x.current_ < y.current_;
    }

    friend constexpr bool operator>(iterator const& x, iterator const& y)
        requires random_access_range<Base> {
        return y < x;
    }

    friend constexpr bool operator<=(iterator const& x, iterator const& y)
        requires random_access_range<Base> {
        return !(y < x);
    }

    friend constexpr bool operator>=(iterator const& x, iterator const& y)
        requires random_access_range<Base> {
        return !(x < y);
    }

    friend constexpr auto operator<=>(iterator const& x, iterator const& y)
        requires random_access_range<Base> && three_way_comparable<iterator_t<Base>> {
        return x.current_ <=> y.current_;
    }

    friend constexpr iterator operator+(iterator const& x, difference_type n)
        requires random_access_range<Base> {
        auto r  = x;
        r      += n;
        return r;
    }

    friend constexpr iterator operator+(difference_type n, iterator const& x)
        requires random_access_range<Base> {
        return x + n;
    }

    friend constexpr iterator operator-(iterator const& x, difference_type n)
        requires random_access_range<Base> {
        auto r  = x;
        r      -= n;
        return r;
    }

    friend constexpr difference_type operator-(iterator const& x, iterator const& y)
        requires sized_sentinel_for<iterator_t<Base>, iterator_t<Base>> {
        auto n = x.current_ - y.current_;
        if constexpr (forward_range<Base>) {
            return (n + x.missing_ - y.missing_) / x.stride_;
        } else if (n < 0) {
            return -__div_ceil(-n, x.stride_);
        } else {
            return __div_ceil(n, x.stride_);
        }
    }

    friend constexpr difference_type operator-(default_sentinel_t, iterator const& x)
        requires sized_sentinel_for<sentinel_t<Base>, iterator_t<Base>> {
        return __div_ceil(x.end_ - x.current_, x.stride_);
    }

    friend constexpr difference_type operator-(iterator const& x, default_sentinel_t y)
        requires sized_sentinel_for<sentinel_t<Base>, iterator_t<Base>> {
        return -(y - x);
    }

    friend constexpr range_rvalue_reference_t<Base>
    iter_move(iterator const& i) noexcept(noexcept(ranges::iter_move(i.current_))) {
        return ranges::iter_move(i.current_);
    }

    friend constexpr void iter_swap(iterator const& x, iterator const& y) noexcept(
        noexcept(ranges::iter_swap(x.current_, y.current_))
    ) requires indirectly_swappable<iterator_t<Base>> {
        ranges::iter_swap(x.current_, y.current_);
    }
};

template<class V> constexpr bool enable_borrowed_range<stride_view<V>> = enable_borrowed_range<V>;

// =============================================================================
// 25.7.32.1 Overview [range.stride.overview] - Range Adaptor Object
// =============================================================================

namespace views {

inline constexpr struct __stride_fn {
    static constexpr auto operator()(auto&& n) {
        return __range_adaptor_closure_fn(bind_back<__stride_fn>(FWD(n)));
    }

    static constexpr auto operator()(auto&& E, auto&& N) _STD_RETURN(stride_view(FWD(E), FWD(N)));
} stride;

}  // namespace views

}  // namespace std::ranges
