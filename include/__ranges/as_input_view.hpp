#pragma once

#include <__ranges/core.hpp>
#include <iterator>

namespace std::ranges {

template<input_range V> requires view<V> class as_input_view {
    V base_ = V();

    template<bool Const> class iterator {
        using Base = __maybe_const<Const, V>;
        iterator_t<Base> current_{};

        constexpr explicit iterator(iterator_t<Base> c): current_(std::move(c)) {}

        friend as_input_view;
        template<bool> friend class iterator;

    public:
        using difference_type  = range_difference_t<Base>;
        using value_type       = range_value_t<Base>;
        using iterator_concept = input_iterator_tag;

        iterator() requires default_initializable<iterator_t<Base>> = default;

        iterator(iterator&&)            = default;
        iterator& operator=(iterator&&) = default;

        constexpr iterator(iterator<!Const> i)
            requires Const && convertible_to<iterator_t<V>, iterator_t<Base>>
            : current_(std::move(i.current_)) {}

        constexpr iterator_t<Base> base() && { return std::move(current_); }
        constexpr iterator_t<Base> const& base() const& noexcept { return current_; }

        constexpr decltype(auto) operator*() const { return *current_; }

        constexpr iterator& operator++() {
            ++current_;
            return *this;
        }
        constexpr void operator++(int) { ++*this; }

        friend constexpr bool operator==(iterator const& x, sentinel_t<Base> const& y) {
            return x.current_ == y;
        }

        friend constexpr difference_type operator-(sentinel_t<Base> const& y, iterator const& x)
            requires sized_sentinel_for<sentinel_t<Base>, iterator_t<Base>> {
            return y - x.current_;
        }
        friend constexpr difference_type operator-(iterator const& x, sentinel_t<Base> const& y)
            requires sized_sentinel_for<sentinel_t<Base>, iterator_t<Base>> {
            return x.current_ - y;
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

public:
    as_input_view() requires default_initializable<V> = default;
    constexpr explicit as_input_view(V base): base_(std::move(base)) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr auto begin() requires(!__detail::__simple_view<V>) {
        return iterator<false>(ranges::begin(base_));
    }
    constexpr auto begin() const requires range<V const> {
        return iterator<true>(ranges::begin(base_));
    }

    constexpr auto end() requires(!__detail::__simple_view<V>) { return ranges::end(base_); }
    constexpr auto end() const requires range<V const> { return ranges::end(base_); }

    constexpr auto size() requires sized_range<V> { return ranges::size(base_); }
    constexpr auto size() const requires sized_range<V const> { return ranges::size(base_); }

    constexpr auto reserve_hint() requires approximately_sized_range<V> {
        return ranges::reserve_hint(base_);
    }
    constexpr auto reserve_hint() const requires approximately_sized_range<V const> {
        return ranges::reserve_hint(base_);
    }
};

template<class R> as_input_view(R&&) -> as_input_view<views::all_t<R>>;

template<class V> constexpr bool enable_borrowed_range<as_input_view<V>> = enable_borrowed_range<V>;

namespace views {
inline constexpr struct __as_input_fn: range_adaptor_closure<__as_input_fn> {

    template<class E>
    requires input_range<E> && (!common_range<E> && !forward_range<E>)static constexpr auto
    operator()(E&& e) _STD_RETURN(all(FWD(e)));

    template<class E> static constexpr auto operator()(E&& e) _STD_RETURN(as_input_view(FWD(e)));

} as_input;
}  // namespace views

}  // namespace std::ranges
