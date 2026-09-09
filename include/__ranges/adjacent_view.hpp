#pragma once

#include <__algorithm/minmax.hpp>
#include <__ranges/core.hpp>
#include <array>
#include <tuple>

namespace std::ranges {

// --- Exposition-only helpers and traits ---

// REPEAT(T, N) helper to construct a tuple with N identical types
template<class T, size_t N, class = std::make_index_sequence<N>> struct __repeat_tuple;

template<class T, size_t N, size_t... Is> struct __repeat_tuple<T, N, std::index_sequence<Is...>> {
    template<size_t> using __elem = T;
    using type                    = std::tuple<__elem<Is>...>;
};

template<class T, size_t N> using __repeat_tuple_t = typename __repeat_tuple<T, N>::type;

// =============================================================================
// 25.7.27.2 Class template adjacent_view [range.adjacent.view]
// =============================================================================

template<forward_range V, size_t N> requires view<V> && (N > 0)
class adjacent_view: public view_interface<adjacent_view<V, N>> {
    V base_ = V();

    template<bool> class iterator;
    template<bool> class sentinel;

    struct as_sentinel {};

public:
    adjacent_view() requires default_initializable<V> = default;

    constexpr explicit adjacent_view(V base): base_(std::move(base)) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr auto begin() requires(!__detail::__simple_view<V>) {
        return iterator<false>(ranges::begin(base_), ranges::end(base_));
    }

    constexpr auto begin() const requires range<V const> {
        return iterator<true>(ranges::begin(base_), ranges::end(base_));
    }

    constexpr auto end() requires(!__detail::__simple_view<V>) {
        if constexpr (common_range<V>) {
            return iterator<false>(as_sentinel{}, ranges::begin(base_), ranges::end(base_));
        } else {
            return sentinel<false>(ranges::end(base_));
        }
    }

    constexpr auto end() const requires range<V const> {
        if constexpr (common_range<V const>) {
            return iterator<true>(as_sentinel{}, ranges::begin(base_), ranges::end(base_));
        } else {
            return sentinel<true>(ranges::end(base_));
        }
    }

    constexpr auto size() requires sized_range<V> {
        using ST  = decltype(ranges::size(base_));
        using CT  = common_type_t<ST, size_t>;
        auto sz   = static_cast<CT>(ranges::size(base_));
        sz       -= std::min<CT>(sz, N - 1);
        return static_cast<ST>(sz);
    }

    constexpr auto size() const requires sized_range<V const> {
        using ST  = decltype(ranges::size(base_));
        using CT  = common_type_t<ST, size_t>;
        auto sz   = static_cast<CT>(ranges::size(base_));
        sz       -= std::min<CT>(sz, N - 1);
        return static_cast<ST>(sz);
    }

    constexpr auto reserve_hint() requires approximately_sized_range<V> {
        using DT  = range_difference_t<decltype((base_))>;
        using CT  = common_type_t<DT, size_t>;
        auto sz   = static_cast<CT>(ranges::reserve_hint(base_));
        sz       -= std::min<CT>(sz, N - 1);
        return __detail::__to_unsigned_like(sz);
    }

    constexpr auto reserve_hint() const requires approximately_sized_range<V const> {
        using DT  = range_difference_t<decltype((base_))>;
        using CT  = common_type_t<DT, size_t>;
        auto sz   = static_cast<CT>(ranges::reserve_hint(base_));
        sz       -= std::min<CT>(sz, N - 1);
        return __detail::__to_unsigned_like(sz);
    }
};

// =============================================================================
// 25.7.27.3 Class template adjacent_view::iterator [range.adjacent.iterator]
// =============================================================================

template<forward_range V, size_t N> requires view<V> && (N > 0) template<bool Const>
class adjacent_view<V, N>::iterator {
    using Base = __maybe_const<Const, V>;
    std::array<iterator_t<Base>, N> current_{};

    constexpr iterator(iterator_t<Base> first, sentinel_t<Base> last) {
        current_[0] = std::move(first);
        for (size_t i = 1; i < N; ++i) { current_[i] = ranges::next(current_[i - 1], 1, last); }
    }

    constexpr iterator(as_sentinel, iterator_t<Base> first, iterator_t<Base> last) {
        if constexpr (!bidirectional_range<Base>) {
            current_.fill(last);
        } else {
            current_[N - 1] = last;
            for (size_t i = N - 1; i > 0; --i) {
                current_[i - 1] = ranges::prev(current_[i], 1, first);
            }
        }
    }

    template<size_t... Is>
    constexpr iterator(iterator<!Const> i, std::index_sequence<Is...>)
        : current_(std::move(i.current_)) {}

    friend class adjacent_view;
    template<bool> friend class iterator;
    template<bool> friend class sentinel;

public:
    using iterator_category = input_iterator_tag;
    using iterator_concept  = std::conditional_t<
        random_access_range<Base>, random_access_iterator_tag,
        std::conditional_t<
            bidirectional_range<Base>, bidirectional_iterator_tag, forward_iterator_tag>>;
    using value_type      = __repeat_tuple_t<range_value_t<Base>, N>;
    using difference_type = range_difference_t<Base>;

    iterator() = default;

    constexpr iterator(iterator<!Const> i)
        requires Const && convertible_to<iterator_t<V>, iterator_t<Base>>
        : iterator(std::move(i), std::make_index_sequence<N>{}) {}

    constexpr auto operator*() const {
        return __detail::__tuple_transform([](auto& i) -> decltype(auto) { return *i; }, current_);
    }

    constexpr iterator& operator++() {
        for (auto& it: current_) { ++it; }
        return *this;
    }

    constexpr iterator operator++(int) {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr iterator& operator--() requires bidirectional_range<Base> {
        for (auto& it: current_) { --it; }
        return *this;
    }

    constexpr iterator operator--(int) requires bidirectional_range<Base> {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    constexpr iterator& operator+=(difference_type x) requires random_access_range<Base> {
        for (auto& it: current_) { it += x; }
        return *this;
    }

    constexpr iterator& operator-=(difference_type x) requires random_access_range<Base> {
        for (auto& it: current_) { it -= x; }
        return *this;
    }

    constexpr auto operator[](difference_type n) const requires random_access_range<Base> {
        return __tuple_transform([&](auto& i) -> decltype(auto) { return i[n]; }, current_);
    }

    friend constexpr bool operator==(iterator const& x, iterator const& y) {
        return x.current_.back() == y.current_.back();
    }

    friend constexpr bool operator<(iterator const& x, iterator const& y)
        requires random_access_range<Base> {
        return x.current_.back() < y.current_.back();
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
        return x.current_.back() <=> y.current_.back();
    }

    friend constexpr iterator operator+(iterator const& i, difference_type n)
        requires random_access_range<Base> {
        auto r  = i;
        r      += n;
        return r;
    }

    friend constexpr iterator operator+(difference_type n, iterator const& i)
        requires random_access_range<Base> {
        auto r  = i;
        r      += n;
        return r;
    }

    friend constexpr iterator operator-(iterator const& i, difference_type n)
        requires random_access_range<Base> {
        auto r  = i;
        r      -= n;
        return r;
    }

    friend constexpr difference_type operator-(iterator const& x, iterator const& y)
        requires sized_sentinel_for<iterator_t<Base>, iterator_t<Base>> {
        return x.current_.back() - y.current_.back();
    }

    friend constexpr auto iter_move(iterator const& i) noexcept(
        noexcept(ranges::iter_move(std::declval<iterator_t<Base> const&>()))
        && is_nothrow_move_constructible_v<range_rvalue_reference_t<Base>>
    ) {
        return __tuple_transform(ranges::iter_move, i.current_);
    }

    friend constexpr void iter_swap(iterator const& l, iterator const& r) noexcept(noexcept(
        ranges::iter_swap(std::declval<iterator_t<Base>>(), std::declval<iterator_t<Base>>())
    )) requires indirectly_swappable<iterator_t<Base>> {
        for (size_t i = 0; i < N; ++i) { ranges::iter_swap(l.current_[i], r.current_[i]); }
    }
};

// =============================================================================
// 25.7.27.4 Class template adjacent_view::sentinel [range.adjacent.sentinel]
// =============================================================================

template<forward_range V, size_t N> requires view<V> && (N > 0) template<bool Const>
class adjacent_view<V, N>::sentinel {
    using Base            = __maybe_const<Const, V>;
    sentinel_t<Base> end_ = sentinel_t<Base>();

    constexpr explicit sentinel(sentinel_t<Base> end): end_(std::move(end)) {}

    friend class adjacent_view;
    template<bool> friend class sentinel;

public:
    sentinel() = default;

    constexpr sentinel(sentinel<!Const> i)
        requires Const && convertible_to<sentinel_t<V>, sentinel_t<Base>>
        : end_(std::move(i.end_)) {}

    template<bool OtherConst>
    requires sentinel_for<sentinel_t<Base>, iterator_t<__maybe_const<OtherConst, V>>>
    friend constexpr bool operator==(iterator<OtherConst> const& x, sentinel const& y) {
        return x.current_.back() == y.end_;
    }

    template<bool OtherConst>
    requires sized_sentinel_for<sentinel_t<Base>, iterator_t<__maybe_const<OtherConst, V>>>
    friend constexpr range_difference_t<__maybe_const<OtherConst, V>>
    operator-(iterator<OtherConst> const& x, sentinel const& y) {
        return x.current_.back() - y.end_;
    }

    template<bool OtherConst>
    requires sized_sentinel_for<sentinel_t<Base>, iterator_t<__maybe_const<OtherConst, V>>>
    friend constexpr range_difference_t<__maybe_const<OtherConst, V>>
    operator-(sentinel const& y, iterator<OtherConst> const& x) {
        return y.end_ - x.current_.back();
    }
};

template<class V, size_t N>
constexpr bool enable_borrowed_range<adjacent_view<V, N>> = enable_borrowed_range<V>;

// =============================================================================
// 25.7.27.1 Overview [range.adjacent.overview] - Range Adaptor Object
// =============================================================================

namespace views {

template<size_t N> struct __adjacent_fn: range_adaptor_closure<__adjacent_fn<N>> {
    static constexpr auto
    operator()(auto&& r) _STD_RETURN(adjacent_view<all_t<decltype(r)>, N>(FWD(r)));
};

template<> struct __adjacent_fn<0>: range_adaptor_closure<__adjacent_fn<0>> {
    static constexpr empty_view<tuple<>> operator()(auto&&) noexcept { return empty<tuple<>>; }
};

template<size_t N> inline constexpr __adjacent_fn<N> adjacent{};

}  // namespace views

}  // namespace std::ranges
