#pragma once

#include <__ranges/concat_view.hpp>
#include <__ranges/core.hpp>
#include <__ranges/single.hpp>

#include <iterator>
#include <optional>
#include <type_traits>
#include <utility>

namespace std::ranges {

template<class R> concept __bidirectional_common = bidirectional_range<R> && common_range<R>;

template<input_range V, forward_range Pattern> requires view<V> && input_range<range_reference_t<V>>
                                                     && view<Pattern>
                                                     && __concatable<range_reference_t<V>, Pattern>
class join_with_view: public view_interface<join_with_view<V, Pattern>> {
    using InnerRng = range_reference_t<V>;

    V base_ = V();

    [[no_unique_address]] conditional_t<
        forward_range<V>, __empty, __detail::non_propagating_cache<iterator_t<V>>>
        outer_it_;  //, present only if forward_range<V> is false

    [[no_unique_address]] conditional_t<
        is_reference_v<InnerRng>, __empty, __detail::non_propagating_cache<remove_cv_t<InnerRng>>>
        inner_;  //, present only if is_reference_v<InnerRng> is false

    Pattern pattern_ = Pattern();

    // [range.join.with.iterator], class template join_with_view​::​iterator
    template<bool Const> struct iterator;

    // [range.join.with.sentinel], class template join_with_view​::​sentinel
    template<bool Const> struct sentinel;

public:
    join_with_view() requires default_initializable<V> && default_initializable<Pattern> = default;

    constexpr explicit join_with_view(V base, Pattern pattern)
        : base_(std::move(base)), pattern_(std::move(pattern)) {}

    template<input_range R>
    requires constructible_from<V, views::all_t<R>>
              && constructible_from<Pattern, single_view<range_value_t<InnerRng>>>
    constexpr explicit join_with_view(R&& r, range_value_t<InnerRng> e)
        : base_(views::all(FWD(r))), pattern_(views::single(std::move(e))) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr auto begin() {
        if constexpr (forward_range<V>) {
            constexpr bool use_const = __detail::__simple_view<V> && is_reference_v<InnerRng>
                                    && __detail::__simple_view<Pattern>;
            return iterator<use_const>{*this, ranges::begin(base_)};
        } else {
            outer_it_ = ranges::begin(base_);
            return iterator<false>{*this};
        }
    }
    constexpr auto begin() const requires forward_range<V const> && forward_range<Pattern const>
                                       && is_reference_v<range_reference_t<V const>>
                                       && input_range<range_reference_t<V const>>
                                       && __concatable<range_reference_t<V const>, Pattern const> {
        return iterator<true>{*this, ranges::begin(base_)};
    }

    constexpr auto end() {
        if constexpr (
            forward_range<V> && is_reference_v<InnerRng> && forward_range<InnerRng>
            && common_range<V> && common_range<InnerRng>
        )
            return iterator<__detail::__simple_view<V> && __detail::__simple_view<Pattern>>{
                *this, ranges::end(base_)
            };
        else
            return sentinel<__detail::__simple_view<V> && __detail::__simple_view<Pattern>>{*this};
    }
    constexpr auto end() const requires forward_range<V const> && forward_range<Pattern const>
                                     && is_reference_v<range_reference_t<V const>>
                                     && input_range<range_reference_t<V const>>
                                     && __concatable<range_reference_t<V const>, Pattern const> {
        using InnerConstRng = range_reference_t<V const>;
        if constexpr (
            forward_range<InnerConstRng> && common_range<V const> && common_range<InnerConstRng>
        )
            return iterator<true>{*this, ranges::end(base_)};
        else
            return sentinel<true>{*this};
    }
};

template<class R, class P>
join_with_view(R&&, P&&) -> join_with_view<views::all_t<R>, views::all_t<P>>;

template<input_range R>
join_with_view(R&&, range_value_t<range_reference_t<R>>)
    -> join_with_view<views::all_t<R>, single_view<range_value_t<range_reference_t<R>>>>;

template<input_range V, forward_range Pattern>
requires view<V> && input_range<range_reference_t<V>> && view<Pattern>
      && __concatable<range_reference_t<V>, Pattern> template<bool Const>
struct join_with_view<V, Pattern>::iterator {
private:
    using Parent      = __maybe_const<Const, join_with_view>;
    using Base        = __maybe_const<Const, V>;
    using InnerBase   = range_reference_t<Base>;
    using PatternBase = __maybe_const<Const, Pattern>;

    using OuterIter   = iterator_t<Base>;
    using InnerIter   = iterator_t<InnerBase>;
    using PatternIter = iterator_t<PatternBase>;

    friend join_with_view;
    template<bool> friend struct iterator;
    template<bool> friend struct sentinel;

    static constexpr bool ref_is_glvalue = is_reference_v<InnerBase>;

    Parent* parent_ = nullptr;
    [[no_unique_address]] conditional_t<forward_range<Base>, OuterIter, __empty>
        outer_it_{};  // , present only if Base models forward_range
    variant<PatternIter, InnerIter> inner_it_;

    constexpr iterator(Parent& parent, OuterIter outer) requires forward_range<Base>
        : parent_(&parent), outer_it_(std::move(outer)) {
        if (outer() != ranges::end(parent_->base_)) {
            inner_it_.template emplace<1>(ranges::begin(update_inner()));
            satisfy();
        }
    }
    constexpr explicit iterator(Parent& parent) requires(!forward_range<Base>): parent_(&parent) {
        if (outer() != ranges::end(parent_->base_)) {
            inner_it_.template emplace<1>(ranges::begin(update_inner()));
            satisfy();
        }
    }

    constexpr OuterIter& outer() {
        if constexpr (forward_range<Base>) {
            return outer_it_;
        } else {
            return *parent_->outer_it_;
        }
    }
    constexpr OuterIter const& outer() const {
        if constexpr (forward_range<Base>) {
            return outer_it_;
        } else {
            return *parent_->outer_it_;
        }
    }
    constexpr auto& update_inner() {
        if constexpr (ref_is_glvalue) {
            return __as_lvalue(*outer());
        } else {
            return parent_->inner_.__emplace_deref(outer());
        }
    }
    constexpr auto& get_inner() {
        if constexpr (ref_is_glvalue) {
            return __as_lvalue(*outer());
        } else {
            return *parent_->inner_;
        }
    }
    constexpr void satisfy() {
        while (true) {
            if (inner_it_.index() == 0) {
                if (std::get<0>(inner_it_) != ranges::end(parent_->pattern_)) break;
                inner_it_.template emplace<1>(ranges::begin(update_inner()));
            } else {
                if (std::get<1>(inner_it_) != ranges::end(get_inner())) break;
                if (++outer() == ranges::end(parent_->base_)) {
                    if constexpr (ref_is_glvalue) inner_it_.template emplace<0>();
                    break;
                }
                inner_it_.template emplace<0>(ranges::begin(parent_->pattern_));
            }
        }
    }

public:
    using iterator_concept = conditional_t<
        ref_is_glvalue && bidirectional_range<Base> && __bidirectional_common<InnerBase>
            && __bidirectional_common<PatternBase>,
        bidirectional_iterator_tag,
        conditional_t<
            ref_is_glvalue && forward_range<InnerBase> && forward_range<PatternBase>,
            forward_iterator_tag, input_iterator_tag>>;
    // using iterator_category = see below;  // not always present
    using value_type      = common_type_t<iter_value_t<InnerBase>, iter_value_t<PatternBase>>;
    using difference_type = common_type_t<
        iter_difference_t<OuterIter>, iter_difference_t<InnerIter>, iter_difference_t<PatternIter>>;

    iterator() = default;

    constexpr iterator(iterator<!Const> i)
        requires Const && convertible_to<iterator_t<V>, OuterIter>
                  && convertible_to<iterator_t<InnerRng>, InnerIter>
                  && convertible_to<iterator_t<Pattern>, PatternIter>
        : outer_it_(std::move(outer_it_)), parent_(i.parent_) {
        if (i.inner_it_.index() == 0)
            inner_it_.template emplace<0>(std::get<0>(std::move(i.inner_it_)));
        else
            inner_it_.template emplace<1>(std::get<1>(std::move(i.inner_it_)));
    }

    constexpr decltype(auto) operator*() const {
        using ref = common_reference_t<iter_reference_t<InnerIter>, iter_reference_t<PatternIter>>;
        return std::visit([](auto& it) -> ref { return *it; }, inner_it_);
    }

    constexpr iterator& operator++() {
        std::visit([](auto& it) { ++it; }, inner_it_);
        satisfy();
        return *this;
    }
    constexpr void operator++(int) { ++*this; }
    iterator operator++(int)
        requires ref_is_glvalue && forward_iterator<OuterIter> && forward_iterator<InnerIter> {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr iterator& operator--() requires ref_is_glvalue && bidirectional_range<Base>
                                           && __bidirectional_common<InnerBase>
                                           && __bidirectional_common<PatternBase> {
        if (outer_it_ == ranges::end(parent_->base_)) {
            auto&& inner = *--outer_it_;
            inner_it_.template emplace<1>(ranges::end(inner));
        }

        while (true) {
            if (inner_it_.index() == 0) {
                auto& it = std::get<0>(inner_it_);
                if (it == ranges::begin(parent_->pattern_)) {
                    auto&& inner = *--outer_it_;
                    inner_it_.template emplace<1>(ranges::end(inner));
                } else {
                    break;
                }
            } else {
                auto& it     = std::get<1>(inner_it_);
                auto&& inner = *outer_it_;
                if (it == ranges::begin(inner)) {
                    inner_it_.template emplace<0>(ranges::end(parent_->pattern_));
                } else {
                    break;
                }
            }
        }
        visit([](auto& it) { --it; }, inner_it_);
        return *this;
    }
    iterator operator--(int) requires ref_is_glvalue
                                   && bidirectional_range<Base> && __bidirectional_common<InnerBase>
                                   && __bidirectional_common<PatternBase> {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    friend constexpr bool operator==(iterator const& x, iterator const& y)
        requires ref_is_glvalue && forward_range<Base> && equality_comparable<InnerIter> {
        return x.outer_it_ == y.outer_it_ && x.inner_it_ == y.inner_it_;
    }

    friend constexpr decltype(auto) iter_move(iterator const& x) {
        using rvalue_reference = common_reference_t<
            iter_rvalue_reference_t<InnerIter>, iter_rvalue_reference_t<PatternIter>>;
        return visit<rvalue_reference>(ranges::iter_move, x.inner_it_);
    }

    friend constexpr void iter_swap(iterator const& x, iterator const& y)
        requires indirectly_swappable<InnerIter, PatternIter> {
        visit(ranges::iter_swap, x.inner_it_, y.inner_it_);
    }
};

template<input_range V, forward_range Pattern>
requires view<V> && input_range<range_reference_t<V>> && view<Pattern>
      && __concatable<range_reference_t<V>, Pattern> template<bool Const>
struct join_with_view<V, Pattern>::sentinel {
private:
    using Parent          = __maybe_const<Const, join_with_view>;
    using Base            = __maybe_const<Const, V>;
    sentinel_t<Base> end_ = sentinel_t<Base>();

    constexpr explicit sentinel(Parent& parent): end_(ranges::end(parent.base_)) {}

    friend join_with_view;
    template<bool> friend struct sentinel;

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

inline constexpr struct __join_with_fn {
    static constexpr auto
    operator()(auto&& E, auto&& F) _STD_RETURN(join_with_view(FWD(E), FWD(F)));

    static constexpr auto operator()(auto&& F) noexcept {
        return __range_adaptor_closure_fn(std::bind_back<__join_with_fn>(FWD(F)));
    }

} join_with;

}  // namespace std::ranges
