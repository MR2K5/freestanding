#pragma once
// code: language=c++
// IWYU pragma: private: include <iterator>

#include <__iterator/concepts.hpp>
#include <__iterator/iter_move.hpp>
#include <__memory/base.hpp>
#include <utility>

#include <compare>
#include <concepts>
#include <type_traits>

namespace std {

template<input_or_output_iterator I> class basic_const_iterator;

namespace __detail {

template<class T> struct __const_iter_base {};
template<class T> requires forward_iterator<T> && requires {
    typename iterator_traits<T>::iterator_category;
} struct __const_iter_base<T> {
    using iterator_category = iterator_traits<T>::iterator_category;
};

template<class T> inline constexpr bool __is_const_iterator                          = false;
template<class I> inline constexpr bool __is_const_iterator<basic_const_iterator<I>> = true;

template<class It>
concept constant_iterator =
    input_iterator<It> && same_as<iter_const_reference_t<It>, iter_reference_t<It>>;

}  // namespace __detail

template<input_or_output_iterator I> class basic_const_iterator: __detail::__const_iter_base<I> {
    using _reference = iter_const_reference_t<I>;
    I current_{};

public:
    using iterator_concept = conditional<
        contiguous_iterator<I>, contiguous_iterator_tag,
        conditional_t<
            random_access_iterator<I>, random_access_iterator_tag,
            conditional_t<
                bidirectional_iterator<I>, bidirectional_iterator_tag,
                conditional<forward_iterator<I>, forward_iterator_tag, input_iterator_tag>>>>;
    using value_type      = iter_value_t<I>;
    using difference_type = iter_difference_t<I>;

    basic_const_iterator() requires default_initializable<I> = default;
    constexpr basic_const_iterator(I x): current_(std::move(x)) {}
    template<convertible_to<I> U>
    constexpr basic_const_iterator(basic_const_iterator<U> o): current_(o.current_) {}
    template<convertible_to<I> T> requires(!same_as<T, basic_const_iterator>)
    constexpr basic_const_iterator(T&& t): current_(std::forward<T>(t)) {}

    constexpr I const& base() const& noexcept { return current_; }
    constexpr I base() && { return std::move(current_); }

    constexpr std::iter_const_reference_t<I> operator*() const {
        return static_cast<iter_const_reference_t<I>>(*current_);
    }
    constexpr auto const* operator->() const
        requires is_lvalue_reference_v<iter_reference_t<I>>
              && same_as<value_type, remove_cvref_t<iter_reference_t<I>>> {
        if constexpr (contiguous_iterator<I>) {
            return std::to_address(current_);
        } else {
            return std::addressof(*current_);
        }
    }

    constexpr iter_const_reference_t<I> operator[](difference_type n) const
        requires random_access_iterator<I> {
        return static_cast<iter_const_reference_t<I>>(current_[n]);
    }

    constexpr basic_const_iterator& operator++() {
        ++current_;
        return *this;
    }
    constexpr void operator++(int) { ++current_; }
    constexpr basic_const_iterator operator++(int) requires forward_iterator<I> {
        auto tmp(*this);
        ++current_;
        return tmp;
    }
    constexpr basic_const_iterator& operator--() requires bidirectional_iterator<I> { --current_; }
    constexpr basic_const_iterator operator--(int) requires bidirectional_iterator<I> {
        auto tmp(*this);
        --current_;
        return tmp;
    }

    constexpr basic_const_iterator& operator+=(difference_type n) requires random_access_iterator<I>
    {
        current_ += n;
        return *this;
    }

    constexpr basic_const_iterator& operator-=(difference_type n) requires random_access_iterator<I>
    {
        current_ -= n;
        return *this;
    }

    template<class CI> requires(!__detail::__is_const_iterator<CI>)
                            && __detail::constant_iterator<CI> && convertible_to<I const&, CI>
    constexpr operator CI() const& {
        return current_;
    }
    template<class CI> requires(!__detail::__is_const_iterator<CI>)
                            && __detail::constant_iterator<CI> && convertible_to<I, CI>
    constexpr operator CI() && {
        return std::move(current_);
    }

    template<sentinel_for<I> S> constexpr bool operator==(S const& s) const {
        return current_ == s;
    }
    constexpr bool operator<(basic_const_iterator const& o) const requires random_access_iterator<I>
    {
        return current_ < o.current_;
    }
    constexpr bool operator>(basic_const_iterator const& o) const requires random_access_iterator<I>
    {
        return current_ > o.current_;
    }
    constexpr bool operator<=(basic_const_iterator const& o) const
        requires random_access_iterator<I> {
        return current_ <= o.current_;
    }
    constexpr bool operator>=(basic_const_iterator const& o) const
        requires random_access_iterator<I> {
        return current_ >= o.current_;
    }
    constexpr auto operator<=>(basic_const_iterator const& o) const
        requires random_access_iterator<I> && three_way_comparable<I> {
        return current_ <=> o.current_;
    }

    template<class O> requires(!same_as<basic_const_iterator, O>)
    constexpr bool operator<(O const& y) const
        requires random_access_iterator<I> && totally_ordered_with<I, O> {
        return current_ < y;
    }
    template<class O> requires(!same_as<basic_const_iterator, O>)
    constexpr bool operator>(O const& y) const
        requires random_access_iterator<I> && totally_ordered_with<I, O> {
        return current_ > y;
    }
    template<class O> requires(!same_as<basic_const_iterator, O>)
    constexpr bool operator<=(O const& y) const
        requires random_access_iterator<I> && totally_ordered_with<I, O> {
        return current_ <= y;
    }
    template<class O> requires(!same_as<basic_const_iterator, O>)
    constexpr bool operator>=(O const& y) const
        requires random_access_iterator<I> && totally_ordered_with<I, O> {
        return current_ >= y;
    }
    template<class O> requires(!same_as<basic_const_iterator, O>)
    constexpr auto operator<=>(O const& y) const
        requires random_access_iterator<I> && totally_ordered_with<I, O>
              && three_way_comparable_with<I, O> {
        return current_ <=> y;
    }

    template<class O> requires(!__detail::__is_const_iterator<O>)
                           && random_access_iterator<I> && totally_ordered_with<I, O>
    friend constexpr bool operator<(O const& x, basic_const_iterator const& y) {
        return x < y.current_;
    }
    template<class O> requires(!__detail::__is_const_iterator<O>)
                           && random_access_iterator<I> && totally_ordered_with<I, O>
    friend constexpr bool operator>(O const& x, basic_const_iterator const& y) {
        return x > y.current_;
    }
    template<class O> requires(!__detail::__is_const_iterator<O>)
                           && random_access_iterator<I> && totally_ordered_with<I, O>
    friend constexpr bool operator<=(O const& x, basic_const_iterator const& y) {
        return x <= y.current_;
    }
    template<class O> requires(!__detail::__is_const_iterator<O>)
                           && random_access_iterator<I> && totally_ordered_with<I, O>
    friend constexpr bool operator>=(O const& x, basic_const_iterator const& y) {
        return x >= y.current_;
    }

    friend constexpr basic_const_iterator
    operator+(basic_const_iterator const& i, difference_type n) requires random_access_iterator<I> {
        return basic_const_iterator(i.current_ + n);
    }
    friend constexpr basic_const_iterator
    operator+(difference_type n, basic_const_iterator const& i) requires random_access_iterator<I> {
        return i + n;
    }

    friend constexpr basic_const_iterator
    operator-(basic_const_iterator const& i, difference_type n) requires random_access_iterator<I> {
        return basic_const_iterator(i.current_ - n);
    }

    template<sized_sentinel_for<I> S> constexpr difference_type operator-(S const& s) const {
        return current_ - s;
    }
    template<class S> requires(!__detail::__is_const_iterator<S>) && sized_sentinel_for<S, I>
    friend constexpr difference_type operator-(S const& s, basic_const_iterator const& i) {
        return s - i.current_;
    }

    using __get_rval_ref = common_reference_t<iter_value_t<I> const&&, iter_rvalue_reference_t<I>>;
    friend constexpr __get_rval_ref iter_move(basic_const_iterator const& i) noexcept(
        noexcept(static_cast<__get_rval_ref>(ranges::iter_move(i.current_)))
    ) {
        return static_cast<__get_rval_ref>(ranges::iter_move(i.current_));
    }
};

template<class T, common_with<T> U> requires input_iterator<common_type_t<T, U>>
struct common_type<basic_const_iterator<T>, U> {
    using type = basic_const_iterator<common_type_t<T, U>>;
};
template<class T, common_with<T> U> requires input_iterator<common_type_t<T, U>>
struct common_type<U, basic_const_iterator<T>> {
    using type = basic_const_iterator<common_type_t<T, U>>;
};
template<class T, common_with<T> U> requires input_iterator<common_type_t<T, U>>
struct common_type<basic_const_iterator<T>, basic_const_iterator<U>> {
    using type = basic_const_iterator<common_type_t<T, U>>;
};

}  // namespace std
