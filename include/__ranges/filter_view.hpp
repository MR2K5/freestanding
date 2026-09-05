#pragma once

#include <__algorithm/copy_move_fill_find.hpp>
#include <__ranges/core.hpp>
#include <concepts>
#include <iterator>
#include <optional>
#include <type_traits>

namespace std::ranges {

template<input_range V, indirect_unary_predicate<iterator_t<V>> Pred>
requires view<V> && is_object_v<Pred>
class filter_view: public view_interface<filter_view<V, Pred>> {
    [[no_unique_address]] V base_ = {};
    [[no_unique_address]] __detail::movable_box<Pred> pred_;
    [[no_unique_address]] conditional_t<
        forward_range<V>, __detail::non_propagating_cache<iterator_t<V>>, __empty> cache_;

    template<bool B> struct __iter_base {};
    template<bool B> requires forward_range<__maybe_const<B, V>> struct __iter_base<B> {
        using Base              = __maybe_const<B, V>;
        using C                 = iterator_traits<iterator_t<Base>>::iterator_category;
        using iterator_category = conditional_t<
            derived_from<C, bidirectional_iterator_tag>, bidirectional_iterator_tag,
            conditional_t<derived_from<C, forward_iterator_tag>, forward_iterator_tag, C>>;
    };

    template<bool Const> class iterator: public __iter_base<Const> {
        using Parent          = __maybe_const<Const, filter_view>;
        using Base            = __maybe_const<Const, V>;
        iterator_t<Base> cur_ = iterator_t<Base>();
        Parent* parent_       = nullptr;
        constexpr iterator(Parent& parent, iterator_t<Base> current)
            : cur_(std::move(current)), parent_(std::addressof(parent)) {}

        template<bool> friend class iterator;
        friend filter_view;

    public:
        using value_type       = range_value_t<Base>;
        using difference_type  = range_difference_t<Base>;
        using iterator_concept = conditional_t<
            Const, input_iterator_tag,
            conditional_t<
                bidirectional_range<V>, bidirectional_iterator_tag,
                conditional_t<forward_range<V>, forward_iterator_tag, input_iterator_tag>>>;

        constexpr iterator(iterator<!Const> i)
            requires Const && convertible_to<iterator_t<V>, iterator_t<Base>>
            : parent_(i.parent_), cur_(std::move(i.cur_)) {}

        iterator() requires default_initializable<iterator_t<Base>> = default;

        constexpr iterator_t<Base> const& base() const& noexcept { return cur_; }
        constexpr iterator_t<Base> base() && { return std::move(cur_); }

        constexpr range_reference_t<Base> operator*() const { return *cur_; }
        constexpr iterator_t<Base> operator->() const
            requires __detail::__has_arrow<iterator_t<Base>> && copyable<iterator_t<Base>> {
            return cur_;
        }

        constexpr iterator& operator++() {
            cur_ =
                find_if(std::move(++cur_), ranges::end(parent_->base_), std::ref(*parent_->pred_));
            return *this;
        }
        constexpr void operator++(int) { ++*this; }
        constexpr iterator operator++(int) requires forward_range<Base> {
            auto tmp = *this;
            ++*this;
            return tmp;
        }
        constexpr iterator& operator--() requires bidirectional_range<Base> {
            do { --cur_; } while (!std::invoke(*parent_->pred_, *cur_));
            return *this;
        }
        constexpr iterator operator--(int) requires bidirectional_range<Base> {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        friend constexpr bool operator==(iterator const& x, iterator const& y)
            requires equality_comparable<iterator_t<Base>> {
            return x.cur_ == y.cur_;
        }
        friend constexpr range_rvalue_reference_t<Base>
        iter_move(iterator const& i) noexcept(noexcept(ranges::iter_move(i.cur_))) {
            return ranges::iter_move(i.cur_);
        }
        friend constexpr void iter_swap(iterator const& x, iterator const& y) noexcept(
            noexcept(ranges::iter_swap(x.cur_, y.cur_))
        ) requires indirectly_swappable<iterator_t<Base>> {
            return ranges::iter_swap(x.cur_, y.cur_);
        }
    };
    template<bool Const> struct sentinel {
    private:
        using Base            = __maybe_const<Const, V>;
        using Parent          = __maybe_const<Const, filter_view>;
        sentinel_t<Base> end_ = sentinel_t<Base>();
        constexpr explicit sentinel(Parent& parent): end_(ranges::end(parent.base_)) {}

        template<bool> friend struct sentinel;
        friend filter_view;

    public:
        sentinel() = default;
        constexpr sentinel(sentinel<!Const> other)
            requires Const && convertible_to<sentinel_t<V>, sentinel_t<Base>>
            : end_(std::move(other.end_)) {}

        constexpr sentinel_t<Base> base() const { return end_; }

        template<bool OtherConst>
        requires sentinel_for<sentinel_t<Base>, iterator_t<__maybe_const<OtherConst, V>>>
        friend constexpr bool operator==(iterator<OtherConst> const& x, sentinel const& y) {
            return x.base() == y.end_;
        }
    };

public:
    filter_view() requires default_initializable<V> && default_initializable<Pred> = default;
    constexpr explicit filter_view(V base, Pred pred)
        : base_(std::move(base)), pred_(std::move(pred)) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr Pred const& pred() const { return *pred_; }

    constexpr iterator<false> begin() {
        if constexpr (!forward_range<V>) {
            return iterator<false>{*this, find_if(base_, std::ref(*pred_))};
        } else {
            if (!cache_.has_value()) cache_ = find_if(base_, std::ref(*pred_));
            return {*this, *cache_};
        }
    }
    constexpr iterator<true> begin() const requires(
        input_range<V const> && !forward_range<V const>
        && indirect_unary_predicate<Pred const, iterator_t<V const>>
    ) {
        return iterator<true>{*this, find_if(base_, std::ref(*pred_))};
    }

    constexpr auto end() {
        if constexpr (common_range<V>)
            return iterator<false>{*this, ranges::end(base_)};
        else
            return sentinel<false>{*this};
    }
    constexpr sentinel<true> end() const requires(
        input_range<V const> && !forward_range<V const>
        && indirect_unary_predicate<Pred const, iterator_t<V const>>
    ) {
        return sentinel<true>{*this};
    }
};

template<class R, class Pred> filter_view(R&&, Pred) -> filter_view<views::all_t<R>, Pred>;

namespace views {

inline constexpr struct __filter_fn {
    static constexpr decltype(auto) operator()(auto&& E, auto&& P)
        requires requires { filter_view(FWD(E), FWD(P)); } {
        return filter_view(FWD(E), FWD(P));
    }

    static constexpr auto operator()(auto&& pred) {
        return __range_adaptor_closure_fn(std::bind_back<__filter_fn>(FWD(pred)));
    }
} filter;

}  // namespace views

}  // namespace std::ranges
