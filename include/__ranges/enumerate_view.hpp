#pragma once

#include <__ranges/core.hpp>
#include <iterator>
#include <type_traits>

namespace std::ranges {

template<view V> requires __detail::__range_with_movable_references<V>
class enumerate_view: public view_interface<enumerate_view<V>> {
    V base_ = V();  // exposition only

    // [range.enumerate.iterator], class template enumerate_view​::​iterator
    template<bool Const> class iterator {
        using Base = __maybe_const<Const, V>;  // exposition only

    public:
        using iterator_category = input_iterator_tag;
        using iterator_concept  = conditional_t<
            random_access_range<Base>, random_access_iterator_tag,
            conditional_t<
                bidirectional_range<Base>, bidirectional_iterator_tag,
                conditional_t<forward_range<Base>, forward_iterator_tag, input_iterator_tag>>>;
        using difference_type = range_difference_t<Base>;
        using value_type      = tuple<difference_type, range_value_t<Base>>;

    private:
        using reference_type =  // exposition only
            tuple<difference_type, range_reference_t<Base>>;
        iterator_t<Base> current_ = iterator_t<Base>();  // exposition only
        difference_type pos_      = 0;                   // exposition only

        constexpr explicit iterator(iterator_t<Base> current, difference_type pos)
            : current_(std::move(current)), pos_(pos) {}

        friend enumerate_view;
        template<bool> friend class iterator;

    public:
        iterator() requires default_initializable<iterator_t<Base>> = default;
        constexpr iterator(iterator<!Const> i)
            requires Const && convertible_to<iterator_t<V>, iterator_t<Base>>
            : current_(std::move(i.current_)), pos_(i.pos_) {}

        constexpr iterator_t<Base> const& base() const& noexcept { return current_; }
        constexpr iterator_t<Base> base() && { return std::move(current_); }

        constexpr difference_type index() const noexcept { return pos_; }

        constexpr auto operator*() const { return reference_type(pos_, *current_); }

        constexpr iterator& operator++() {
            ++current_;
            ++pos_;
            return *this;
        }
        constexpr void operator++(int) { ++*this; }
        iterator operator++(int) requires forward_range<Base> {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        constexpr iterator& operator--() requires bidirectional_range<Base> {
            --current_;
            --pos_;
            return *this;
        }
        iterator operator--(int) {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        constexpr iterator& operator+=(difference_type x) requires random_access_range<Base> {
            current_ += x;
            pos_     += x;
            return *this;
        }
        constexpr iterator& operator-=(difference_type x) requires random_access_range<Base> {
            current_ -= x;
            pos_     -= x;
            return *this;
        }

        constexpr auto operator[](difference_type n) const requires random_access_range<Base> {
            return reference_type(pos_ + n, current_[n]);
        }

        friend constexpr bool operator==(iterator const& x, iterator const& y) noexcept {
            return x.pos_ == y.pos_;
        }
        friend constexpr strong_ordering
        operator<=>(iterator const& x, iterator const& y) noexcept {
            return x.pos_ <=> y.pos_;
        }

        friend constexpr iterator operator+(iterator const& x, difference_type y)
            requires random_access_range<Base> {
            return auto(x) += y;
        }
        friend constexpr iterator operator+(difference_type x, iterator const& y)
            requires random_access_range<Base> {
            return y + x;
        }
        friend constexpr iterator operator-(iterator const& x, difference_type y)
            requires random_access_range<Base> {
            return auto(x) -= y;
        }
        friend constexpr difference_type operator-(iterator const& x, iterator const& y) noexcept {
            return x.pos_ - y.pos_;
        }

        friend constexpr auto iter_move(iterator const& i) noexcept(
            noexcept(ranges::iter_move(i.current_))
            && is_nothrow_move_constructible_v<range_rvalue_reference_t<Base>>
        ) {
            return tuple<difference_type, range_rvalue_reference_t<Base>>(
                i.pos_, ranges::iter_move(i.current_)
            );
        }
    };

    // [range.enumerate.sentinel], class template enumerate_view​::​sentinel
    template<bool Const> class sentinel {
        using Base            = __maybe_const<Const, V>;
        sentinel_t<Base> end_ = sentinel_t<Base>();
        constexpr explicit sentinel(sentinel_t<Base> end): end_(std::move(end)) {}

        friend enumerate_view;
        template<bool> friend class sentinel;

    public:
        sentinel() = default;
        constexpr sentinel(sentinel<!Const> other)
            requires Const && convertible_to<sentinel_t<V>, sentinel_t<Base>>
            : end_(other.end_) {}

        constexpr sentinel_t<Base> base() const { return end_; }

        template<bool OtherConst>
        requires sentinel_for<sentinel_t<Base>, iterator_t<__maybe_const<OtherConst, V>>>
        friend constexpr bool operator==(iterator<OtherConst> const& x, sentinel const& y) {
            return x.base() == y.end_;
        }

        template<bool OtherConst>
        requires sized_sentinel_for<sentinel_t<Base>, iterator_t<__maybe_const<OtherConst, V>>>
        friend constexpr range_difference_t<__maybe_const<OtherConst, V>>
        operator-(iterator<OtherConst> const& x, sentinel const& y) {
            return x.base() - y.end_;
        }

        template<bool OtherConst>
        requires sized_sentinel_for<sentinel_t<Base>, iterator_t<__maybe_const<OtherConst, V>>>
        friend constexpr range_difference_t<__maybe_const<OtherConst, V>>
        operator-(sentinel const& x, iterator<OtherConst> const& y) {
            return x.end_ - y.base();
        }
    };

public:
    constexpr enumerate_view() requires default_initializable<V> = default;
    constexpr explicit enumerate_view(V base): base_(std::move(base)) {}

    constexpr auto begin() requires(!__detail::__simple_view<V>) {
        return iterator<false>(ranges::begin(base_), 0);
    }
    constexpr auto begin() const requires __detail::__range_with_movable_references<V const> {
        return iterator<true>(ranges::begin(base_), 0);
    }

    constexpr auto end() requires(!__detail::__simple_view<V>) {
        if constexpr (forward_range<V> && common_range<V> && sized_range<V>)
            return iterator<false>(ranges::end(base_), ranges::distance(base_));
        else
            return sentinel<false>(ranges::end(base_));
    }
    constexpr auto end() const requires __detail::__range_with_movable_references<V const> {
        if constexpr (forward_range<V const> && common_range<V const> && sized_range<V const>)
            return iterator<true>(ranges::end(base_), ranges::distance(base_));
        else
            return sentinel<true>(ranges::end(base_));
    }

    constexpr auto size() requires sized_range<V> { return ranges::size(base_); }
    constexpr auto size() const requires sized_range<V const> { return ranges::size(base_); }

    constexpr auto reserve_hint() requires approximately_sized_range<V> {
        return ranges::reserve_hint(base_);
    }
    constexpr auto reserve_hint() const requires approximately_sized_range<V const> {
        return ranges::reserve_hint(base_);
    }

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }
};

template<class View>
constexpr bool enable_borrowed_range<enumerate_view<View>> = enable_borrowed_range<View>;

template<class R> enumerate_view(R&&) -> enumerate_view<views::all_t<R>>;

namespace views {
inline constexpr struct __enumerate_fn: range_adaptor_closure<__enumerate_fn> {
    static constexpr auto
    operator()(auto&& e) _STD_RETURN(enumerate_view<all_t<decltype(e)>>(FWD(e)));
} enumerate;
}  // namespace views

}  // namespace std::ranges
