#pragma once

#include <__ranges/core.hpp>
#include <optional>

namespace std::ranges {

namespace __detail {

// Exposition-only as-lvalue helper [range.join.iterator]
template<class T> constexpr T& __as_lvalue(T&& t) noexcept {
    return static_cast<T&>(t);
}

// Zero-space conditional cache allocation for join_view
template<class T, bool Present> struct __join_cache_storage {
    using type = __empty;
};

template<class T> struct __join_cache_storage<T, true> {
    using type = non_propagating_cache<T>;
};

template<class T, bool Present>
using __join_cache_storage_t = typename __join_cache_storage<T, Present>::type;

// Conditionally declares iterator_category for join_view::iterator [range.join.iterator] p2
template<
    class Base, bool =
                    (is_reference_v<range_reference_t<Base>> && forward_range<Base>
                     && forward_range<range_reference_t<Base>>)>
struct __join_view_iterator_category_base {};

template<class Base> struct __join_view_iterator_category_base<Base, true> {
private:
    using _OuterC = typename iterator_traits<iterator_t<Base>>::iterator_category;
    using _InnerC =
        typename iterator_traits<iterator_t<range_reference_t<Base>>>::iterator_category;

public:
    using iterator_category = conditional_t<
        derived_from<_OuterC, bidirectional_iterator_tag>
            && derived_from<_InnerC, bidirectional_iterator_tag>
            && common_range<range_reference_t<Base>>,
        bidirectional_iterator_tag,
        conditional_t<
            derived_from<_OuterC, forward_iterator_tag>
                && derived_from<_InnerC, forward_iterator_tag>,
            forward_iterator_tag, input_iterator_tag>>;
};

// Conditionally declares outer_ storage inside join_view::iterator
template<class Base, bool = forward_range<Base>> struct __join_view_outer_storage {
    iterator_t<Base> outer_ = iterator_t<Base>();

    constexpr __join_view_outer_storage() = default;
    constexpr explicit __join_view_outer_storage(iterator_t<Base> outer)
        : outer_(std::move(outer)) {}
};

template<class Base> struct __join_view_outer_storage<Base, false> {
    constexpr __join_view_outer_storage() = default;
};

}  // namespace __detail

// =============================================================================
// Class template join_view [range.join.view]
// =============================================================================

template<input_range V> requires view<V> && input_range<range_reference_t<V>>
class join_view: public view_interface<join_view<V>> {
private:
    using InnerRng = range_reference_t<V>;

    template<bool Const> struct iterator;
    template<bool Const> struct sentinel;

    template<bool> friend struct iterator;
    template<bool> friend struct sentinel;

    [[no_unique_address]] V base_ = V();

    // Present only if V is not a forward_range
    [[no_unique_address]] __detail::__join_cache_storage_t<iterator_t<V>, !forward_range<V>>
        outer_{};

    // Present only if InnerRng is not a reference type
    [[no_unique_address]] __detail::__join_cache_storage_t<
        remove_cv_t<InnerRng>, !is_reference_v<InnerRng>> inner_{};

public:
    join_view() requires default_initializable<V> = default;

    constexpr explicit join_view(V base): base_(std::move(base)) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr auto begin() {
        if constexpr (forward_range<V>) {
            constexpr bool use_const = __detail::__simple_view<V> && is_reference_v<InnerRng>;
            return iterator<use_const>{*this, ranges::begin(base_)};
        } else {
            outer_.emplace(ranges::begin(base_));
            return iterator<false>{*this};
        }
    }

    constexpr auto begin() const requires forward_range<V const>
                                       && is_reference_v<range_reference_t<V const>>
                                       && input_range<range_reference_t<V const>> {
        return iterator<true>{*this, ranges::begin(base_)};
    }

    constexpr auto end() {
        if constexpr (
            forward_range<V> && is_reference_v<InnerRng> && forward_range<InnerRng>
            && common_range<V> && common_range<InnerRng>
        ) {
            return iterator<__detail::__simple_view<V>>{*this, ranges::end(base_)};
        } else {
            return sentinel<__detail::__simple_view<V>>{*this};
        }
    }

    constexpr auto end() const requires forward_range<V const>
                                     && is_reference_v<range_reference_t<V const>>
                                     && input_range<range_reference_t<V const>> {
        if constexpr (
            forward_range<range_reference_t<V const>> && common_range<V const>
            && common_range<range_reference_t<V const>>
        ) {
            return iterator<true>{*this, ranges::end(base_)};
        } else {
            return sentinel<true>{*this};
        }
    }
};

// Deduction Guide [range.join.view]
template<class R> explicit join_view(R&&) -> join_view<views::all_t<R>>;

// =============================================================================
// Class template join_view::iterator [range.join.iterator]
// =============================================================================

template<input_range V> requires view<V> && input_range<range_reference_t<V>> template<bool Const>
struct join_view<V>::iterator
    : __detail::__join_view_iterator_category_base<__maybe_const<Const, V>>,
      __detail::__join_view_outer_storage<__maybe_const<Const, V>> {
private:
    using Parent    = __maybe_const<Const, join_view>;
    using Base      = __maybe_const<Const, V>;
    using OuterIter = iterator_t<Base>;
    using InnerIter = iterator_t<range_reference_t<Base>>;

    static constexpr bool ref_is_glvalue = is_reference_v<range_reference_t<Base>>;

    template<bool> friend struct iterator;
    template<bool> friend struct sentinel;

    std::optional<InnerIter> inner_;
    Parent* parent_ = nullptr;

    constexpr OuterIter& outer() {
        if constexpr (forward_range<Base>) {
            return this->outer_;
        } else {
            return *parent_->outer_;
        }
    }

    constexpr OuterIter const& outer() const {
        if constexpr (forward_range<Base>) {
            return this->outer_;
        } else {
            return *parent_->outer_;
        }
    }

    constexpr void satisfy() {
        auto update_inner = [this](iterator_t<Base> const& x) -> auto&& {
            if constexpr (ref_is_glvalue) {
                return *x;
            } else {
                parent_->inner_.emplace(*x);
                return *parent_->inner_;
            }
        };

        for (; outer() != ranges::end(parent_->base_); ++outer()) {
            auto&& inner = update_inner(outer());
            inner_       = ranges::begin(inner);
            if (*inner_ != ranges::end(inner)) { return; }
        }

        if constexpr (ref_is_glvalue) { inner_.reset(); }
    }

    constexpr iterator(Parent& parent, OuterIter outer) requires forward_range<Base>
        : __detail::__join_view_outer_storage<Base>(std::move(outer)),
          parent_(std::addressof(parent)) {
        satisfy();
    }

    constexpr explicit iterator(Parent& parent) requires(!forward_range<Base>)
        : parent_(std::addressof(parent)) {
        satisfy();
    }

    friend class join_view;

public:
    using iterator_concept = conditional_t<
        ref_is_glvalue && bidirectional_range<Base> && bidirectional_range<range_reference_t<Base>>
            && common_range<range_reference_t<Base>>,
        bidirectional_iterator_tag,
        conditional_t<
            ref_is_glvalue && forward_range<Base> && forward_range<range_reference_t<Base>>,
            forward_iterator_tag, input_iterator_tag>>;

    using value_type = range_value_t<range_reference_t<Base>>;
    using difference_type =
        common_type_t<range_difference_t<Base>, range_difference_t<range_reference_t<Base>>>;

    iterator() = default;

    constexpr iterator(iterator<!Const> i)
        requires Const && convertible_to<iterator_t<V>, OuterIter>
                  && convertible_to<iterator_t<InnerRng>, InnerIter>
        : __detail::__join_view_outer_storage<Base>(std::move(i.outer_)),
          inner_(std::move(i.inner_)), parent_(i.parent_) {}

    constexpr decltype(auto) operator*() const { return **inner_; }

    constexpr InnerIter operator->() const
        requires __detail::__has_arrow<InnerIter> && copyable<InnerIter> {
        return *inner_;
    }

    constexpr iterator& operator++() {
        auto&& inner_range = [this]() -> auto&& {
            if constexpr (ref_is_glvalue) {
                return *outer();
            } else {
                return *parent_->inner_;
            }
        }();

        if (++*inner_ == ranges::end(__detail::__as_lvalue(inner_range))) {
            ++outer();
            satisfy();
        }
        return *this;
    }

    constexpr void operator++(int) { ++*this; }

    constexpr iterator operator++(int)
        requires ref_is_glvalue && forward_range<Base> && forward_range<range_reference_t<Base>> {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr iterator& operator--() requires ref_is_glvalue && bidirectional_range<Base>
                                           && bidirectional_range<range_reference_t<Base>>
                                           && common_range<range_reference_t<Base>> {
        if (outer() == ranges::end(parent_->base_)) {
            inner_ = ranges::end(__detail::__as_lvalue(*--outer()));
        }
        while (*inner_ == ranges::begin(__detail::__as_lvalue(*outer()))) {
            *inner_ = ranges::end(__detail::__as_lvalue(*--outer()));
        }
        --*inner_;
        return *this;
    }

    constexpr iterator operator--(int) requires ref_is_glvalue && bidirectional_range<Base>
                                             && bidirectional_range<range_reference_t<Base>>
                                             && common_range<range_reference_t<Base>> {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    friend constexpr bool operator==(iterator const& x, iterator const& y)
        requires ref_is_glvalue
              && forward_range<Base> && equality_comparable<iterator_t<range_reference_t<Base>>> {
        return x.outer() == y.outer() && x.inner_ == y.inner_;
    }

    friend constexpr decltype(auto)
    iter_move(iterator const& i) noexcept(noexcept(ranges::iter_move(*i.inner_))) {
        return ranges::iter_move(*i.inner_);
    }

    friend constexpr void iter_swap(iterator const& x, iterator const& y) noexcept(
        noexcept(ranges::iter_swap(*x.inner_, *y.inner_))
    ) requires indirectly_swappable<InnerIter> {
        ranges::iter_swap(*x.inner_, *y.inner_);
    }
};

// =============================================================================
// Class template join_view::sentinel [range.join.sentinel]
// =============================================================================

template<input_range V> requires view<V> && input_range<range_reference_t<V>> template<bool Const>
struct join_view<V>::sentinel {
private:
    using Parent = __maybe_const<Const, join_view>;
    using Base   = __maybe_const<Const, V>;

    template<bool> friend struct sentinel;

    sentinel_t<Base> end_ = sentinel_t<Base>();

    constexpr explicit sentinel(Parent& parent): end_(ranges::end(parent.base_)) {}

    friend class join_view;

public:
    sentinel() = default;

    constexpr sentinel(sentinel<!Const> s)
        requires Const && convertible_to<sentinel_t<V>, sentinel_t<Base>>
        : end_(std::move(s.end_)) {}

    template<bool OtherConst>
    requires sentinel_for<sentinel_t<Base>, iterator_t<__maybe_const<OtherConst, V>>>
    friend constexpr bool operator==(iterator<OtherConst> const& x, sentinel const& y) {
        return x.outer() == y.end_;
    }
};

// =============================================================================
// views::join Range Adaptor Object [range.join.overview]
// =============================================================================

namespace views {

inline constexpr struct __join_fn {
    template<class R> requires requires(R&& r) { join_view<views::all_t<R>>{std::forward<R>(r)}; }
    constexpr auto
    operator()(R&& r) const noexcept(noexcept(join_view<views::all_t<R>>{std::forward<R>(r)}))
        -> join_view<views::all_t<R>> {
        return join_view<views::all_t<R>>{std::forward<R>(r)};
    }

    template<class R> requires requires(R&& r, __join_fn const& self) { self(std::forward<R>(r)); }
    friend constexpr auto
    operator|(R&& r, __join_fn const& self) noexcept(noexcept(self(std::forward<R>(r))))
        -> decltype(self(std::forward<R>(r))) {
        return self(std::forward<R>(r));
    }
} join;

}  // namespace views

}  // namespace std::ranges
