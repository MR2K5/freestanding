#pragma once

#include <__config.hpp>
#include <compare>
#include <concepts>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace std {

namespace __counted_iter_detail {

// Exposition-only base templates to conditionally provide member typedefs
template<class I> struct __value_type_base {};

template<indirectly_readable I> struct __value_type_base<I> {
    using value_type = iter_value_t<I>;
};

template<class I> struct __concept_base {};

template<class I> requires requires { typename I::iterator_concept; } struct __concept_base<I> {
    using iterator_concept = typename I::iterator_concept;
};

template<class I> struct __category_base {};

template<class I> requires requires { typename I::iterator_category; } struct __category_base<I> {
    using iterator_category = typename I::iterator_category;
};

template<class I>
struct __iter_traits_base: __value_type_base<I>, __concept_base<I>, __category_base<I> {};

}  // namespace __counted_iter_detail

template<input_or_output_iterator I>
class counted_iterator: public __counted_iter_detail::__iter_traits_base<I> {
private:
    I current                   = I();
    iter_difference_t<I> length = 0;

    template<input_or_output_iterator I2> friend class counted_iterator;

public:
    using iterator_type   = I;
    using difference_type = iter_difference_t<I>;

    constexpr counted_iterator() requires default_initializable<I> = default;

    constexpr counted_iterator(I x, iter_difference_t<I> n): current(std::move(x)), length(n) {}

    template<class I2> requires convertible_to<const I2&, I>
    constexpr counted_iterator(counted_iterator<I2> const& x)
        : current(x.current), length(x.length) {}

    template<class I2> requires assignable_from<I&, const I2&>
    constexpr counted_iterator& operator=(counted_iterator<I2> const& x) {
        current = x.current;
        length  = x.length;
        return *this;
    }

    constexpr I const& base() const& noexcept { return current; }
    constexpr I base() && { return std::move(current); }
    constexpr iter_difference_t<I> count() const noexcept { return length; }

    constexpr decltype(auto) operator*() { return *current; }

    constexpr decltype(auto) operator*() const requires requires(I const& i) { *i; } {
        return *current;
    }

    constexpr auto operator->() const noexcept requires contiguous_iterator<I> {
        return std::to_address(current);
    }

    constexpr decltype(auto) operator[](iter_difference_t<I> n) const
        requires random_access_iterator<I> {
        return current[n];
    }

    constexpr counted_iterator& operator++() {
        ++current;
        --length;
        return *this;
    }

    constexpr decltype(auto) operator++(int) requires(!forward_iterator<I>) {
        --length;
        _TRY {
            return current++;
        }
        _CATCHALL {
            ++length;
            _THROW();
        }
    }

    constexpr counted_iterator operator++(int) requires forward_iterator<I> {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr counted_iterator& operator--() requires bidirectional_iterator<I> {
        --current;
        ++length;
        return *this;
    }

    constexpr counted_iterator operator--(int) requires bidirectional_iterator<I> {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    constexpr counted_iterator operator+(iter_difference_t<I> n) const
        requires random_access_iterator<I> {
        return counted_iterator(current + n, length - n);
    }

    friend constexpr counted_iterator operator+(iter_difference_t<I> n, counted_iterator const& x)
        requires random_access_iterator<I> {
        return x + n;
    }

    constexpr counted_iterator& operator+=(iter_difference_t<I> n)
        requires random_access_iterator<I> {
        current += n;
        length  -= n;
        return *this;
    }

    constexpr counted_iterator operator-(iter_difference_t<I> n) const
        requires random_access_iterator<I> {
        return counted_iterator(current - n, length + n);
    }

    template<common_with<I> I2>
    friend constexpr iter_difference_t<I2>
    operator-(counted_iterator const& x, counted_iterator<I2> const& y) {
        return y.length - x.length;
    }

    friend constexpr iter_difference_t<I>
    operator-(counted_iterator const& x, default_sentinel_t) noexcept {
        return -x.length;
    }

    friend constexpr iter_difference_t<I>
    operator-(default_sentinel_t, counted_iterator const& y) noexcept {
        return y.length;
    }

    constexpr counted_iterator& operator-=(iter_difference_t<I> n)
        requires random_access_iterator<I> {
        current -= n;
        length  += n;
        return *this;
    }

    template<common_with<I> I2>
    friend constexpr bool operator==(counted_iterator const& x, counted_iterator<I2> const& y) {
        return x.length == y.length;
    }

    friend constexpr bool operator==(counted_iterator const& x, default_sentinel_t) noexcept {
        return x.length == 0;
    }

    template<common_with<I> I2>
    friend constexpr strong_ordering
    operator<=>(counted_iterator const& x, counted_iterator<I2> const& y) {
        return y.length <=> x.length;
    }

    friend constexpr decltype(auto)
    iter_move(counted_iterator const& i) noexcept(noexcept(ranges::iter_move(i.current)))
        requires input_iterator<I> {
        return ranges::iter_move(i.current);
    }

    template<indirectly_swappable<I> I2>
    friend constexpr void iter_swap(
        counted_iterator const& x, counted_iterator<I2> const& y
    ) noexcept(noexcept(ranges::iter_swap(x.current, y.current))) {
        ranges::iter_swap(x.current, y.current);
    }
};

template<input_iterator I> struct iterator_traits<counted_iterator<I>>: iterator_traits<I> {
    using pointer = conditional_t<contiguous_iterator<I>, add_pointer_t<iter_reference_t<I>>, void>;
};

}  // namespace std
