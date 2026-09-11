#pragma once
// code: language=c++
// IWYU pragma: private: include <iterator>

#include <__iterator/concepts.hpp>
#include <__iterator/iter_move.hpp>
#include <__iterator/iter_swap.hpp>
#include <__iterator/operators.hpp>

#include <cassert>
#include <compare>
#include <concepts>
#include <type_traits>
#include <utility>

namespace std {

template<class It> class reverse_iterator {
    static_assert(
        bidirectional_iterator<It> || __detail::__LegacyBidirectionalIterator<It>,
        "reverse_iterator requires a bidirectinal iterator as base"
    );

protected:
    It current = {};

public:
    using iterator_type    = It;
    using iterator_concept = conditional_t<
        random_access_iterator<It>, random_access_iterator_tag, bidirectional_iterator_tag>;
    using iterator_category = conditional_t<
        derived_from<typename iterator_traits<It>::iterator_category, random_access_iterator_tag>,
        random_access_iterator_tag, bidirectional_iterator_tag>;
    using value_type      = iter_value_t<It>;
    using difference_type = iter_difference_t<It>;
    using pointer         = iterator_traits<It>::pointer;
    using reference       = iter_reference_t<It>;

    constexpr reverse_iterator() = default;
    explicit constexpr reverse_iterator(It const& x): current(x) {}
    template<class U> requires(!is_same_v<U, It>) && convertible_to<U const&, It>
    constexpr reverse_iterator(reverse_iterator<U> const& o): current(o.current) {}

    template<class U>
    requires(!is_same_v<U, It>) && convertible_to<U const&, It> && assignable_from<It&, U const&>
    constexpr reverse_iterator& operator=(reverse_iterator<U> const& other) {
        current = other.current;
        return *this;
    }

    constexpr It base() const { return current; }

    constexpr reference operator*() const {
        It tmp = current;
        return *--tmp;
    }
    constexpr pointer operator->() const
        requires is_pointer_v<It> || requires(It const i) { i.operator->(); } {
        if constexpr (is_pointer_v<It>) {
            return current - 1;
        } else {
            return std::prev(current).operator->();
        }
    }

    constexpr reference operator[](difference_type n) const { return current[-n - 1]; }

    constexpr reverse_iterator& operator++() {
        --current;
        return *this;
    }
    constexpr reverse_iterator& operator--() {
        ++current;
        return *this;
    }

    constexpr reverse_iterator operator++(int) {
        auto tmp = *this;
        --current;
        return tmp;
    }
    constexpr reverse_iterator operator--(int) {
        auto tmp = *this;
        ++current;
        return tmp;
    }
    constexpr reverse_iterator operator+(difference_type n) const {
        return reverse_iterator(current - n);
    }
    constexpr reverse_iterator operator-(difference_type n) const {
        return reverse_iterator(current + n);
    }
    constexpr reverse_iterator& operator+=(difference_type n) {
        current -= n;
        return *this;
    }
    constexpr reverse_iterator& operator-=(difference_type n) {
        current += n;
        return *this;
    }

    friend constexpr iter_rvalue_reference_t<It> iter_move(reverse_iterator const& it) noexcept(
        is_nothrow_copy_constructible_v<It> && noexcept(ranges::iter_move(--declval<It&>()))
    ) {
        auto tmp = it.base();
        return ranges::iter_move(--tmp);
    }

    template<indirectly_swappable<It> It2>
    friend constexpr void
    iter_swap(reverse_iterator const& x, reverse_iterator<It2> const& y) noexcept(
        is_nothrow_copy_constructible_v<It> && is_nothrow_copy_constructible_v<It2>
        && noexcept(ranges::iter_swap(--declval<It&>(), --declval<It2&>()))
    ) {
        auto tmp_x = x.base();
        auto tmp_y = y.base();
        return ranges::iter_swap(--tmp_x, --tmp_y);
    }
};

template<class It> constexpr reverse_iterator<It> make_reverse_iterator(It i) {
    return reverse_iterator<It>(i);
}

#define _DECL_REVIT_COMP(op, impl_op)                                                              \
    template<class I1, class I2>                                                                   \
    constexpr bool operator op(reverse_iterator<I1> const& x, reverse_iterator<I2> const& y)       \
        requires requires { x.base() impl_op y.base(); } {                                         \
        return x.base() impl_op y.base();                                                          \
    }

_DECL_REVIT_COMP(==, ==)
_DECL_REVIT_COMP(!=, !=)
_DECL_REVIT_COMP(<, >)
_DECL_REVIT_COMP(<=, >=)
_DECL_REVIT_COMP(>, <)
_DECL_REVIT_COMP(>=, <=)

#undef _DECL_REVIT_COMP

template<class It1, three_way_comparable_with<It1> It2>
constexpr compare_three_way_result_t<It1, It2>
operator<=>(reverse_iterator<It1> const& x, reverse_iterator<It2> const& y) {
    return y.base() <=> x.base();
}

template<class It>
constexpr reverse_iterator<It> operator+(iter_difference_t<It> n, reverse_iterator<It> const& it) {
    return reverse_iterator<It>(it.base() - n);
}

template<class It1, class It2>
constexpr auto operator-(reverse_iterator<It1> const& x, reverse_iterator<It2> const& y)
    -> decltype(y.base() - x.base()) {
    return y.base() - x.base();
}

template<class Iterator1, class Iterator2> requires(!sized_sentinel_for<Iterator1, Iterator2>)
inline constexpr bool
    disable_sized_sentinel_for<reverse_iterator<Iterator1>, reverse_iterator<Iterator2>> = true;

template<class C>
constexpr auto rbegin(C& c) noexcept(noexcept(c.rbegin())) -> decltype(c.rbegin()) {
    return c.rbegin();
}
template<class C>
constexpr auto rbegin(C const& c) noexcept(noexcept(c.rbegin())) -> decltype(c.rbegin()) {
    return c.rbegin();
}
template<class T, size_t N> constexpr reverse_iterator<T*> rbegin(T (&arr)[N]) noexcept {
    return reverse_iterator<T const*>(arr + N);
}
template<class E> constexpr reverse_iterator<E const*> rbegin(initializer_list<E> il) noexcept {
    return reverse_iterator<E const*>(il.end());
}
template<class C>
constexpr auto crbegin(C const& c) noexcept(noexcept(std::rbegin(c))) -> decltype(std::rbegin(c)) {
    return std::rbegin(c);
}

template<class C> constexpr auto rend(C& c) noexcept(noexcept(c.rend())) -> decltype(c.rend()) {
    return c.rend();
}
template<class C>
constexpr auto rend(C const& c) noexcept(noexcept(c.rend())) -> decltype(c.rend()) {
    return c.rend();
}
template<class T, size_t N> constexpr reverse_iterator<T*> rend(T (&arr)[N]) noexcept {
    return reverse_iterator<T const*>(arr);
}
template<class E> constexpr reverse_iterator<E const*> rend(initializer_list<E> il) noexcept {
    return reverse_iterator<E const*>(il.begin());
}
template<class C>
constexpr auto crend(C const& c) noexcept(noexcept(std::rend(c))) -> decltype(std::rend(c)) {
    return std::rend(c);
}

namespace __detail {
template<class It> struct __move_iter_base {};
template<class It> requires requires { typename iterator_traits<It>::iterator_category; }
struct __move_iter_base<It> {
    using __cat             = iterator_traits<It>::iterator_category;
    using iterator_category = conditional_t<
        derived_from<__cat, random_access_iterator_tag>, random_access_iterator_tag, __cat>;
};

}  // namespace __detail

template<semiregular S> class move_sentinel {
    S last_;

public:
    constexpr move_sentinel(): last_() {}
    constexpr explicit move_sentinel(S l): last_(l) {}
    template<class S2> requires convertible_to<S2 const&, S>
    constexpr move_sentinel(move_sentinel<S2> const& x): last_(x.last_) {}

    template<class S2> requires assignable_from<S&, S2 const&>
    constexpr move_sentinel& operator=(move_sentinel<S2> const& x) {
        last_ = x.last_;
        return *this;
    }
    constexpr S base() const { return last_; }
};

template<class It> class move_iterator: public __detail::__move_iter_base<It> {
    It cur_;

public:
    using iterator_type    = It;
    using value_type       = iter_value_t<It>;
    using difference_type  = iter_difference_t<It>;
    using pointer          = It;
    using reference        = iter_rvalue_reference_t<It>;
    using iterator_concept = conditional_t<
        random_access_iterator<It>, random_access_iterator_tag,
        conditional_t<
            bidirectional_iterator<It>, bidirectional_iterator_tag,
            conditional_t<forward_iterator<It>, forward_iterator_tag, input_iterator_tag>>>;

    constexpr move_iterator(): cur_() {}
    constexpr explicit move_iterator(It x): cur_(std::move(x)) {}
    template<class U> requires(!is_same_v<U, It> && convertible_to<U const&, It>)
    constexpr move_iterator(move_iterator<U> const& o): cur_(o.cur_) {}
    template<class U>
    requires(!is_same_v<U, It> && convertible_to<U const&, It> && assignable_from<It&, U const&>)
    constexpr move_iterator& operator=(move_iterator<U> const& o) {
        cur_ = o.cur_;
        return *this;
    }

    constexpr It const& base() const& noexcept { return cur_; }
    constexpr It base() && { return std::move(cur_); }
    constexpr reference operator*() const { return ranges::iter_move(cur_); }
    constexpr pointer operator->() const { return cur_; }
    constexpr reference operator[](difference_type n) const { return ranges::iter_move(cur_ + n); }

    constexpr move_iterator& operator++() {
        ++cur_;
        return *this;
    }
    constexpr move_iterator& operator--() {
        --cur_;
        return *this;
    }
    constexpr auto operator++(int) {
        if constexpr (forward_iterator<It>) {
            auto tmp = *this;
            ++cur_;
            return tmp;
        } else {
            ++cur_;
        }
    }
    constexpr move_iterator operator--(int) {
        auto tmp = *this;
        --cur_;
        return tmp;
    }
    constexpr move_iterator operator+(difference_type n) const { return move_iterator(cur_ + n); }
    constexpr move_iterator operator-(difference_type n) const { return move_iterator(cur_ - n); }
    constexpr move_iterator& operator+=(difference_type n) {
        cur_ += n;
        return *this;
    }
    constexpr move_iterator& operator-=(difference_type n) {
        cur_ -= n;
        return *this;
    }

    template<sentinel_for<It> S>
    friend constexpr bool operator==(move_iterator const& i, move_sentinel<S> const& s) {
        return i.base() == s.base();
    }

    template<sized_sentinel_for<It> S>
    friend constexpr iter_difference_t<It>
    operator-(move_sentinel<S> const& s, move_iterator const& i) {
        return s.base() - i.base();
    }
    template<sized_sentinel_for<It> S>
    friend constexpr iter_difference_t<It>
    operator-(move_iterator const& i, move_sentinel<S> const& s) {
        return i.base() - s.base();
    }

    friend constexpr move_iterator operator+(difference_type n, move_iterator const& x) {
        return x + n;
    }
    template<class It2>
    friend constexpr decltype(auto) operator-(move_iterator const& x, move_iterator<It2> const& y) {
        return x.base() - y.base();
    }
    friend constexpr iter_rvalue_reference_t<It>
    iter_move(move_iterator const& i) noexcept(noexcept(ranges::iter_move(i.cur_))) {
        return ranges::iter_move(i.cur_);
    }
    template<indirectly_swappable<It> It2>
    friend constexpr void iter_swap(move_iterator const& x, move_iterator<It2> const& y) noexcept(
        noexcept(ranges::iter_swap(x.base(), y.base()))
    ) {
        ranges::iter_swap(x.base(), y.base());
    }
};

template<class It> constexpr move_iterator<It> make_move_iterator(It i) {
    return move_iterator<It>(std::move(i));
}
template<class Iterator1, class Iterator2>

requires(!sized_sentinel_for<Iterator1, Iterator2>)
constexpr bool disable_sized_sentinel_for<move_iterator<Iterator1>, move_iterator<Iterator2>> =
    true;

template<class It1, class It2>
constexpr bool operator==(move_iterator<It1> const& x, move_iterator<It2> const& y)
    requires requires {
        { x.base() == y.base() } -> boolean_testable;
    } {
    return x.base() == y.base();
}
template<class It1, class It2>
constexpr bool operator<(move_iterator<It1> const& x, move_iterator<It2> const& y)
    requires requires {
        { x.base() < y.base() } -> boolean_testable;
    } {
    return x.base() < y.base();
}
template<class It1, class It2>
constexpr bool operator<=(move_iterator<It1> const& x, move_iterator<It2> const& y)
    requires requires {
        { x.base() <= y.base() } -> boolean_testable;
    } {
    return x.base() <= y.base();
}
template<class It1, class It2>
constexpr bool operator>(move_iterator<It1> const& x, move_iterator<It2> const& y)
    requires requires {
        { x.base() > y.base() } -> boolean_testable;
    } {
    return x.base() > y.base();
}
template<class It1, class It2>
constexpr bool operator>=(move_iterator<It1> const& x, move_iterator<It2> const& y)
    requires requires {
        { x.base() >= y.base() } -> boolean_testable;
    } {
    return x.base() >= y.base();
}
template<class It1, three_way_comparable_with<It1> It2>
constexpr compare_three_way_result_t<It1, It2>
operator<=>(move_iterator<It1> const& x, move_iterator<It2> const& y) {
    return x.base() <=> y.base();
}

namespace ranges {

namespace __detail {

struct __advance_fn {
    template<input_or_output_iterator I>
    static constexpr void operator()(I& i, iter_difference_t<I> n) {
        assert(n >= 0 || bidirectional_iterator<I>);
        if constexpr (random_access_iterator<I>) {
            i += n;
        } else {
            while (n > 0) {
                ++i;
                --n;
            }
            if constexpr (bidirectional_iterator<I>) {
                while (n < 0) {
                    --i;
                    ++n;
                }
            }
        }
    }

    template<input_or_output_iterator I, sentinel_for<I> S>
    static constexpr void operator()(I& i, S end) {
        if constexpr (assignable_from<I&, S>) {
            i = std::move(end);
        } else if constexpr (sized_sentinel_for<S, I>) {
            operator()(i, end - i);
        } else {
            while (i != end) ++i;
        }
    }

    template<input_or_output_iterator I, sentinel_for<I> S>
    static constexpr iter_difference_t<I> operator()(I& i, iter_difference_t<I> n, S end) {
        assert(bidirectional_iterator<I> || n >= 0);
        if constexpr (sized_sentinel_for<S, I>) {
            auto dist = end - i;
            if (n >= 0) {
                if (n >= dist) {
                    operator()(i, end);
                    return n - dist;
                }
                operator()(i, n);
                return 0;
            } else {
                if (n <= dist) {
                    operator()(i, end);
                    return n - dist;
                }
                operator()(i, n);
                return 0;
            }
        } else {
            while (n > 0 && i != end) {
                --n;
                ++i;
            }
            if constexpr (bidirectional_iterator<I>) {
                while (n < 0 && i != end) {
                    ++n;
                    --i;
                }
            }
            return n;
        }
    }
};

}  // namespace __detail

inline namespace __cpo {
inline constexpr __detail::__advance_fn advance;
}

}  // namespace ranges


}  // namespace std
