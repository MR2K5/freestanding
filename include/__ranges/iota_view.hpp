#pragma once

#include <__iterator/concepts.hpp>
#include <__ranges/core.hpp>
#include <concepts>
#include <cstdint>
#include <type_traits>

namespace std::ranges {

namespace __detail {

template<class W>
using iota_diff_t = decltype([]<class = void> {
    if constexpr (!integral<W>) {
        return iter_difference_t<W>();
    } else if constexpr (sizeof(iter_difference_t<W>) > sizeof(W)) {
        return iter_difference_t<W>();
    } else if constexpr (is_signed_v<W> && sizeof(W) < sizeof(uint64_t)) {
        return int64_t();
    } else {
#ifdef __SIZEOF_INT128__
        return __int128_t{};
#else
        return int64_t();
#endif
    }
}());

template<class W>
concept decrementable = incrementable<W> && requires(W w) {
    { --w } -> same_as<W&>;
    { w-- } -> same_as<W>;
};

template<class I>
concept advanceable =
    decrementable<I> && totally_ordered<I> && requires(I i, I const j, iota_diff_t<I> const n) {
        { i += n } -> same_as<I&>;
        { i -= n } -> same_as<I&>;
        I(j + n);
        I(n + j);
        I(j - n);
        { j - j } -> convertible_to<iota_diff_t<I>>;
    };

}  // namespace __detail

template<weakly_incrementable W, semiregular Bound = unreachable_sentinel_t>
requires __detail::__weakly_equality_comparable_with<W, Bound> && copyable<W>
class iota_view: public view_interface<iota_view<W, Bound>> {
private:
    // [range.iota.iterator], class iota_view​::​iterator

    template<class = void> struct __iter_base {};
    template<class T> requires incrementable<W> && integral<__detail::iota_diff_t<W>>
    struct __iter_base<T> {
        using iterator_category = input_iterator_tag;
    };

    struct iterator: __iter_base<void> {
    private:
        W value_ = W();
        constexpr explicit iterator(W value): value_(value) {}

        friend iota_view;

    public:
        using iterator_concept = conditional_t<
            __detail::advanceable<W>, random_access_iterator_tag,
            conditional_t<
                __detail::decrementable<W>, bidirectional_iterator_tag,
                conditional_t<incrementable<W>, forward_iterator_tag, input_iterator_tag>>>;

        using value_type      = W;
        using difference_type = __detail::iota_diff_t<W>;

        iterator() requires default_initializable<W> = default;

        constexpr W operator*() const noexcept(is_nothrow_copy_constructible_v<W>) {
            return value_;
        }

        constexpr iterator& operator++() {
            ++value_;
            return *this;
        }
        constexpr void operator++(int) { ++*this; }
        constexpr iterator operator++(int) requires incrementable<W> {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        constexpr iterator& operator--() requires __detail::decrementable<W> {
            --value_;
            return *this;
        }
        constexpr iterator operator--(int) {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        constexpr iterator& operator+=(difference_type n) requires __detail::advanceable<W> {
            if constexpr (integral<W> && !signed_integral<W>) {
                if (n >= difference_type(0))
                    value_ += static_cast<W>(n);
                else
                    value_ -= static_cast<W>(-n);
            } else {
                value_ += n;
            }
            return *this;
        }
        constexpr iterator& operator-=(difference_type n) requires __detail::advanceable<W> {
            if constexpr (integral<W> && !signed_integral<W>) {
                if (n >= difference_type(0))
                    value_ -= static_cast<W>(n);
                else
                    value_ += static_cast<W>(-n);
            } else {
                value_ -= n;
            }
            return *this;
        }
        constexpr W operator[](difference_type n) const requires __detail::advanceable<W> {
            return W(value_ + n);
        }

        friend constexpr bool operator==(iterator const& x, iterator const& y)
            requires equality_comparable<W> {
            return x.value_ == y.value_;
        }

        friend constexpr bool operator<(iterator const& x, iterator const& y)
            requires totally_ordered<W> {
            return x.value_ < y.value_;
        }
        friend constexpr bool operator>(iterator const& x, iterator const& y)
            requires totally_ordered<W> {
            return y < x;
        }
        friend constexpr bool operator<=(iterator const& x, iterator const& y)
            requires totally_ordered<W> {
            return !(y < x);
        }
        friend constexpr bool operator>=(iterator const& x, iterator const& y)
            requires totally_ordered<W> {
            return !(x < y);
        }
        friend constexpr auto operator<=>(iterator const& x, iterator const& y)
            requires totally_ordered<W> && three_way_comparable<W> {
            return x.value_ <=> y.value_;
        }

        friend constexpr iterator operator+(iterator i, difference_type n)
            requires __detail::advanceable<W> {
            i += n;
            return i;
        }
        friend constexpr iterator operator+(difference_type n, iterator i)
            requires __detail::advanceable<W> {
            return i + n;
        }

        friend constexpr iterator operator-(iterator i, difference_type n)
            requires __detail::advanceable<W> {
            i -= n;
            return i;
        }
        friend constexpr difference_type operator-(iterator const& x, iterator const& y)
            requires __detail::advanceable<W> {
            using D = difference_type;
            if constexpr (integral<W>) {
                if constexpr (signed_integral<W>)
                    return D(D(x.value_) - D(y.value_));
                else
                    return (y.value_ > x.value_) ? D(-D(y.value_ - x.value_))
                                                 : D(x.value_ - y.value_);
            } else {
                return x.value_ - y.value_;
            }
        }
    };

    // [range.iota.sentinel], class iota_view​::​sentinel
    struct sentinel {
    private:
        Bound bound_ = Bound();
        constexpr explicit sentinel(Bound bound): bound_(bound) {}

        friend iota_view;

    public:
        sentinel() = default;

        friend constexpr bool operator==(iterator const& x, sentinel const& y) {
            return *x == y.bound_;
        }

        friend constexpr iter_difference_t<W> operator-(iterator const& x, sentinel const& y)
            requires sized_sentinel_for<Bound, W> {
            return *x - y.bound_;
        }
        friend constexpr iter_difference_t<W> operator-(sentinel const& x, iterator const& y)
            requires sized_sentinel_for<Bound, W> {
            return -(y - x);
        }
    };

    W value_     = W();
    Bound bound_ = Bound();

public:
    iota_view() requires default_initializable<W> = default;
    constexpr explicit iota_view(W value): value_(value) {
        if constexpr (totally_ordered_with<W, Bound>) assert(value_ <= Bound());
    }
    constexpr explicit iota_view(type_identity_t<W> value, type_identity_t<Bound> bound)
        : value_(value), bound_(bound) {
        if constexpr (totally_ordered_with<W, Bound>) assert(value_ <= bound_);
    }

    constexpr explicit iota_view(iterator first, iterator last) requires same_as<W, Bound>
        : iota_view(first.value_, last.value_) {}

    constexpr explicit iota_view(iterator first, Bound last)
        requires(!same_as<W, Bound>) && same_as<Bound, unreachable_sentinel_t>
        : iota_view(first.value_, last) {}
    constexpr explicit iota_view(iterator first, sentinel last)
        requires(!same_as<W, Bound>) && (!same_as<W, unreachable_sentinel_t>)
        : iota_view(first.value_, last.bound_) {}

    constexpr iterator begin() const { return iterator(value_); }
    constexpr auto end() const {
        if constexpr (same_as<Bound, unreachable_sentinel_t>) {
            return unreachable_sentinel;
        } else {
            return sentinel(bound_);
        }
    }
    constexpr iterator end() const requires same_as<W, Bound> { return iterator(bound_); }

    constexpr bool empty() const { return value_ == bound_; }
    constexpr auto size() const
        requires(same_as<W, Bound> && __detail::advanceable<W>)
             || (integral<W> && integral<Bound>) || sized_sentinel_for<Bound, W> {
        if constexpr (integral<W> && integral<Bound>)
            return (value_ < 0)
                     ? ((bound_ < 0) ? __detail::__to_unsigned_like(-value_)
                                           - __detail::__to_unsigned_like(-bound_)
                                     : __detail::__to_unsigned_like(bound_)
                                           + __detail::__to_unsigned_like(-value_))
                     : __detail::__to_unsigned_like(bound_) - __detail::__to_unsigned_like(value_);
        else
            return __detail::__to_unsigned_like(bound_ - value_);
    }
};

template<class W, class Bound>
requires(!integral<W> || !integral<Bound> || (signed_integral<W> == signed_integral<Bound>))
iota_view(W, Bound) -> iota_view<W, Bound>;

template<class W, class Bound>
    constexpr bool enable_borrowed_range<iota_view<W, Bound>> = true;

namespace views {

inline constexpr struct __iota_t {
    static constexpr auto operator()(auto&& e)
        requires requires { iota_view<decltype((e))>(FWD(e)); } {
        return iota_view<decltype((e))>(FWD(e));
    }
    static constexpr auto operator()(auto&& e, auto&& f)
        requires requires { iota_view(FWD(e), FWD(f)); } {
        return iota_view(FWD(e), FWD(f));
    }
} iota;

inline constexpr struct __indices_t {
    static constexpr auto operator()(integral auto&& e) {
        using T = remove_cvref_t<decltype((e))>;
        return iota(T(0), FWD(e));
    }
} indices;

}  // namespace views

template<class T> inline constexpr bool __is_iota_view_v                           = false;
template<class W, class B> inline constexpr bool __is_iota_view_v<iota_view<W, B>> = true;

}  // namespace std::ranges
