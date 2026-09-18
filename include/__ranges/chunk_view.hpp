#pragma once

#include <__algorithm/minmax.hpp>
#include <__ranges/core.hpp>
#include <__ranges/take_view.hpp>
#include <optional>
#include <type_traits>

namespace std::ranges {

template<view V> requires input_range<V> class chunk_view: public view_interface<chunk_view<V>> {
    V base_;
    range_difference_t<V> n_;
    range_difference_t<V> remainder_ = 0;

    __detail::non_propagating_cache<iterator_t<V>> current_;

    // [range.chunk.inner.iter], class chunk_view​::​inner_iterator
    class inner_iterator {
        chunk_view* parent_;

        constexpr explicit inner_iterator(chunk_view& parent) noexcept: parent_(&parent) {}
        friend chunk_view;

    public:
        using iterator_concept = input_iterator_tag;
        using difference_type  = range_difference_t<V>;
        using value_type       = range_value_t<V>;

        inner_iterator(inner_iterator&&)            = default;
        inner_iterator& operator=(inner_iterator&&) = default;

        constexpr iterator_t<V> const& base() const& { return *parent_->current_; }

        constexpr range_reference_t<V> operator*() const { return **parent_->current_; }
        constexpr inner_iterator& operator++() {
            ++*parent_->current_;
            if (*parent_->current_ == ranges::end(parent_->base_))
                parent_->remainder_ = 0;
            else
                --parent_->remainder_;
            return *this;
        }
        constexpr void operator++(int) { ++*this; }

        friend constexpr bool operator==(inner_iterator const& x, default_sentinel_t) {
            return x.parent_->remainder_ == 0;
        }

        friend constexpr difference_type operator-(default_sentinel_t y, inner_iterator const& x)
            requires sized_sentinel_for<sentinel_t<V>, iterator_t<V>> {
            return ranges::min(
                x.parent_->remainder_, ranges::end(x.parent_->base_) - *x.parent_->current_
            );
        }
        friend constexpr difference_type operator-(inner_iterator const& x, default_sentinel_t y)
            requires sized_sentinel_for<sentinel_t<V>, iterator_t<V>> {
            return -(y - x);
        }

        friend constexpr range_rvalue_reference_t<V> iter_move(inner_iterator const& i) noexcept(
            noexcept(ranges::iter_move(*i.parent_->current_))
        ) {
            return ranges::iter_move(*i.parent_->current_);
        }

        friend constexpr void iter_swap(inner_iterator const& x, inner_iterator const& y) noexcept(
            noexcept(ranges::iter_swap(*x.parent_->current_, *y.parent_->current_))
        ) requires indirectly_swappable<iterator_t<V>> {
            ranges::iter_swap(*x.parent_->current_, *y.parent_->current_);
        }
    };

    // [range.chunk.outer.iter], class chunk_view​::​outer_iterator
    class outer_iterator {
        chunk_view* parent_;

        constexpr explicit outer_iterator(chunk_view& parent): parent_(&parent) {}

        friend chunk_view;

    public:
        using iterator_concept = input_iterator_tag;
        using difference_type  = range_difference_t<V>;

        // [range.chunk.outer.value], class chunk_view​::​outer_iterator​::​value_type
        struct value_type: view_interface<value_type> {
        private:
            chunk_view* parent_;

            constexpr explicit value_type(chunk_view& parent): parent_(&parent) {}
            friend outer_iterator;

        public:
            constexpr inner_iterator begin() const noexcept { return inner_iterator(*parent_); }
            constexpr default_sentinel_t end() const noexcept { return default_sentinel; }

            constexpr auto size() const requires sized_sentinel_for<sentinel_t<V>, iterator_t<V>> {
                return __detail::__to_unsigned_like(
                    ranges::min(
                        parent_->remainder_, ranges::end(parent_->base_) - *parent_->current_
                    )
                );
            }

            constexpr auto reserve_hint() const noexcept {
                return __detail::__to_unsigned_like(parent_->remainder_);
            }
        };

        outer_iterator(outer_iterator&&)            = default;
        outer_iterator& operator=(outer_iterator&&) = default;

        constexpr value_type operator*() const { return value_type(*parent_); }
        constexpr outer_iterator& operator++() {
            ranges::advance(*parent_->current_, parent_->remainder_, ranges::end(parent_->base_));
            parent_->remainder_ = parent_->n_;
            return *this;
        }
        constexpr void operator++(int) { ++*this; }

        friend constexpr bool operator==(outer_iterator const& x, default_sentinel_t) {
            return *x.parent_->current_ == ranges::end(x.parent_->base_)
                && x.parent_->remainder_ != 0;
        }

        friend constexpr difference_type operator-(default_sentinel_t y, outer_iterator const& x)
            requires sized_sentinel_for<sentinel_t<V>, iterator_t<V>> {
            auto const dist = ranges::end(x.parent_->base_) - *x.parent_->current_;
            if (dist < x.parent_->remainder_) { return dist == 0 ? 0 : 1; }
            return __div_ceil(dist - x.parent_->remainder_, x.parent_->n_) + 1;
        }
        friend constexpr difference_type operator-(outer_iterator const& x, default_sentinel_t y)
            requires sized_sentinel_for<sentinel_t<V>, iterator_t<V>> {
            return -(y - x);
        }
    };

public:
    constexpr explicit chunk_view(V base, range_difference_t<V> n): base_(std::move(base)), n_(n) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr outer_iterator begin() {
        current_   = ranges::begin(base_);
        remainder_ = n_;
        return outer_iterator(*this);
    }
    constexpr default_sentinel_t end() const noexcept { return default_sentinel; }

    constexpr auto size() requires sized_range<V> {
        return __detail::__to_unsigned_like(__div_ceil(ranges::distance(base_), n_));
    }
    constexpr auto size() const requires sized_range<V const> {
        return __detail::__to_unsigned_like(__div_ceil(ranges::distance(base_), n_));
    }

    constexpr auto reserve_hint() requires approximately_sized_range<V> {
        auto s = static_cast<range_difference_t<decltype((base_))>>(ranges::reserve_hint(base_));
        return __detail::__to_unsigned_like(__div_ceil(s, n_));
    }
    constexpr auto reserve_hint() const requires approximately_sized_range<V const> {
        auto s = static_cast<range_difference_t<decltype((base_))>>(ranges::reserve_hint(base_));
        return __detail::__to_unsigned_like(__div_ceil(s, n_));
    }
};

template<class R> chunk_view(R&&, range_difference_t<R>) -> chunk_view<views::all_t<R>>;

template<view V> requires forward_range<V>
class chunk_view<V>: public view_interface<chunk_view<V>> {
    V base_;
    range_difference_t<V> n_;

    // [range.chunk.fwd.iter], class template chunk_view​::​iterator
    template<bool Const> class iterator {
        using Parent = __maybe_const<Const, chunk_view>;
        using Base   = __maybe_const<Const, V>;

        iterator_t<Base> current_         = iterator_t<Base>();
        sentinel_t<Base> end_             = sentinel_t<Base>();
        range_difference_t<Base> n_       = 0;
        range_difference_t<Base> missing_ = 0;

        constexpr iterator(
            Parent* parent, iterator_t<Base> current, range_difference_t<Base> missing = 0
        )
            : current_(std::move(current)), end_(ranges::end(parent->base_)), n_(parent->n_),
              missing_(missing) {}

        friend chunk_view;
        template<bool> friend class iterator;

    public:
        using iterator_category = input_iterator_tag;
        using iterator_concept  = conditional_t<
            random_access_range<Base>, random_access_iterator_tag,
            conditional_t<
                bidirectional_range<Base>, bidirectional_iterator_tag, forward_iterator_tag>>;
        using value_type      = decltype(views::take(subrange(current_, end_), n_));
        using difference_type = range_difference_t<Base>;

        iterator() = default;
        constexpr iterator(iterator<!Const> i)
            requires Const && convertible_to<iterator_t<V>, iterator_t<Base>>
                      && convertible_to<sentinel_t<V>, sentinel_t<Base>>
            : current_(std::move(i.current_)), end_(std::move(i.end_)), n_(i.n_),
              missing_(i.missing_) {}

        constexpr iterator_t<Base> base() const { return current_; }

        constexpr value_type operator*() const { return views::take(subrange(current_, end_), n_); }
        constexpr iterator& operator++() {
            missing_ = ranges::advance(current_, n_, end_);
            return *this;
        }
        iterator operator++(int) {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        constexpr iterator& operator--() requires bidirectional_range<Base> {
            ranges::advance(current_, missing_ - n_);
            missing_ = 0;
            return *this;
        }
        iterator operator--(int) requires bidirectional_range<Base> {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        constexpr iterator& operator+=(difference_type x) requires random_access_range<Base> {
            if (x > 0) {
                ranges::advance(current_, n_ * (x - 1));
                missing_ = ranges::advance(current_, n_, end_);
            } else if (x < 0) {
                ranges::advance(current_, n_ * x + missing_);
                missing_ = 0;
            }
            return *this;
        }
        constexpr iterator& operator-=(difference_type x) requires random_access_range<Base> {
            return *this += -x;
        }

        constexpr value_type operator[](difference_type n) const requires random_access_range<Base>
        {
            return *(*this + n);
        }

        friend constexpr bool operator==(iterator const& x, iterator const& y) {
            return x.current_ == y.current_;
        }
        friend constexpr bool operator==(iterator const& x, default_sentinel_t) {
            return x.current_ == x.end_;
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

        friend constexpr iterator operator+(iterator const& i, difference_type n)
            requires random_access_range<Base> {
            return auto(i) += n;
        }
        friend constexpr iterator operator+(difference_type n, iterator const& i)
            requires random_access_range<Base> {
            return auto(i) += n;
        }
        friend constexpr iterator operator-(iterator const& i, difference_type n)
            requires random_access_range<Base> {
            return auto(i) -= n;
        }
        friend constexpr difference_type operator-(iterator const& x, iterator const& y)
            requires sized_sentinel_for<iterator_t<Base>, iterator_t<Base>> {
            return (x.current_ - y.current_ + x.missing_ - y.missing_) / x.n_;
        }

        friend constexpr difference_type operator-(default_sentinel_t y, iterator const& x)
            requires sized_sentinel_for<sentinel_t<Base>, iterator_t<Base>> {
            return __div_ceil(x.end_ - x.current_, x.n_);
        }
        friend constexpr difference_type operator-(iterator const& x, default_sentinel_t y)
            requires sized_sentinel_for<sentinel_t<Base>, iterator_t<Base>> {
            return -(y - x);
        }
    };

public:
    constexpr explicit chunk_view(V base, range_difference_t<V> n): base_(std::move(base)), n_(n) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr auto begin() requires(!__detail::__simple_view<V>) {
        return iterator<false>(this, ranges::begin(base_));
    }

    constexpr auto begin() const requires forward_range<V const> {
        return iterator<true>(this, ranges::begin(base_));
    }

    constexpr auto end() requires(!__detail::__simple_view<V>) {
        if constexpr (common_range<V> && sized_range<V>) {
            auto missing = (n_ - ranges::distance(base_) % n_) % n_;
            return iterator<false>(this, ranges::end(base_), missing);
        } else if constexpr (common_range<V> && !bidirectional_range<V>) {
            return iterator<false>(this, ranges::end(base_));
        } else {
            return default_sentinel;
        }
    }

    constexpr auto end() const requires forward_range<V const> {
        if constexpr (common_range<V const> && sized_range<V const>) {
            auto missing = (n_ - ranges::distance(base_) % n_) % n_;
            return iterator<true>(this, ranges::end(base_), missing);
        } else if constexpr (common_range<V const> && !bidirectional_range<V const>) {
            return iterator<true>(this, ranges::end(base_));
        } else {
            return default_sentinel;
        }
    }

    constexpr auto size() requires sized_range<V> {
        return __detail::__to_unsigned_like(__div_ceil(ranges::distance(base_), n_));
    }
    constexpr auto size() const requires sized_range<V const> {
        return __detail::__to_unsigned_like(__div_ceil(ranges::distance(base_), n_));
    }

    constexpr auto reserve_hint() requires approximately_sized_range<V> {
        auto s = static_cast<range_difference_t<decltype((base_))>>(ranges::reserve_hint(base_));
        return __detail::__to_unsigned_like(__div_ceil(s, n_));
    }
    constexpr auto reserve_hint() const requires approximately_sized_range<V const> {
        auto s = static_cast<range_difference_t<decltype((base_))>>(ranges::reserve_hint(base_));
        return __detail::__to_unsigned_like(__div_ceil(s, n_));
    }
};

template<class V>
constexpr bool enable_borrowed_range<chunk_view<V>> = forward_range<V> && enable_borrowed_range<V>;

namespace views {

inline constexpr struct __chunk_fn {
    static constexpr auto operator()(auto&& e, auto&& f) _STD_RETURN(chunk_view(FWD(e), FWD(f)));

    static constexpr auto operator()(auto&& f) noexcept {
        return __range_adaptor_closure_fn(std::bind_back<__chunk_fn>(FWD(f)));
    }
} chunk;

}  // namespace views

}  // namespace std::ranges
