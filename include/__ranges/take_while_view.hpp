#pragma once

#include <__ranges/core.hpp>
#include <concepts>
#include <optional>

namespace std::ranges {

// =============================================================================
// Class template take_while_view [range.take.while.view]
// =============================================================================

template<view V, class Pred>
requires input_range<V> && is_object_v<Pred> && indirect_unary_predicate<Pred const, iterator_t<V>>
class take_while_view: public view_interface<take_while_view<V, Pred>> {
private:
    template<bool Const> class sentinel;

    [[no_unique_address]] V base_ = V();
    [[no_unique_address]] __detail::movable_box<Pred> pred_;

public:
    take_while_view() requires default_initializable<V> && default_initializable<Pred> = default;

    constexpr explicit take_while_view(V base, Pred pred)
        : base_(std::move(base)), pred_(std::move(pred)) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr Pred const& pred() const { return *pred_; }

    constexpr auto begin() requires(!__detail::__simple_view<V>) { return ranges::begin(base_); }

    constexpr auto begin() const
        requires range<V const> && indirect_unary_predicate<Pred const, iterator_t<V const>> {
        return ranges::begin(base_);
    }

    constexpr auto end() requires(!__detail::__simple_view<V>) {
        return sentinel<false>(ranges::end(base_), std::addressof(*pred_));
    }

    constexpr auto end() const
        requires range<V const> && indirect_unary_predicate<Pred const, iterator_t<V const>> {
        return sentinel<true>(ranges::end(base_), std::addressof(*pred_));
    }
};

// Deduction Guide [range.take.while.view]
template<class R, class Pred> take_while_view(R&&, Pred) -> take_while_view<views::all_t<R>, Pred>;

// =============================================================================
// Class template take_while_view::sentinel [range.take.while.sentinel]
// =============================================================================

template<view V, class Pred>
requires input_range<V> && is_object_v<Pred> && indirect_unary_predicate<Pred const, iterator_t<V>>
template<bool Const>
class take_while_view<V, Pred>::sentinel {
private:
    using Base = __maybe_const<Const, V>;

    template<bool> friend class sentinel;

    sentinel_t<Base> end_ = sentinel_t<Base>();
    Pred const* pred_     = nullptr;

    constexpr explicit sentinel(sentinel_t<Base> end, Pred const* pred)
        : end_(std::move(end)), pred_(pred) {}

    friend class take_while_view;

public:
    sentinel() = default;

    constexpr sentinel(sentinel<!Const> s)
        requires Const && convertible_to<sentinel_t<V>, sentinel_t<Base>>
        : end_(std::move(s.end_)), pred_(s.pred_) {}

    constexpr sentinel_t<Base> base() const { return end_; }

    friend constexpr bool operator==(iterator_t<Base> const& x, sentinel const& y) {
        return y.end_ == x || !std::invoke(*y.pred_, *x);
    }

    template<bool OtherConst = !Const>
    requires sentinel_for<sentinel_t<Base>, iterator_t<__maybe_const<OtherConst, V>>>
    friend constexpr bool
    operator==(iterator_t<__maybe_const<OtherConst, V>> const& x, sentinel const& y) {
        return y.end_ == x || !std::invoke(*y.pred_, *x);
    }
};

// =============================================================================
// views::take_while Range Adaptor Object [range.take.while.overview]
// =============================================================================

namespace views {

inline constexpr struct __take_while_fn {
    template<class R, class Pred> requires requires {
        take_while_view(declval<R>(), declval<Pred>());
    } constexpr auto operator()(R&& r, Pred&& pred) const {
        return take_while_view(FWD(r), FWD(pred));
    }

    template<class Pred>
    requires is_object_v<decay_t<Pred>> && constructible_from<decay_t<Pred>, Pred>
    constexpr auto operator()(Pred&& pred) const {
        return __range_adaptor_closure_fn(bind_back<__take_while_fn>(FWD(pred)));
    }
} take_while;

}  // namespace views

}  // namespace std::ranges
